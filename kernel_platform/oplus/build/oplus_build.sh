#!/bin/sh
set -e
#set -x
function usage() {
    echo -e "
Description:
    这个脚本用于编译高通SM8850平台 kernel相关的目标
    请确保在vnd全编译过的环境使用
    vendor下高通原生KO请直接使用./mk_android.sh -t user -n xxx编译模块

OPTIONS:
    -t, --type
        Build type(user/userdebug/eng) (Default: userdebug)
    -p, --platform
        Specifies platform(SM8850)
    -b, --build
        Specifies build targets,can be one or a combination of the following
        vmlinux: 目标kernel
        abl: 目标abl
        ko: 统一传递格式为"//target_bazel_path:target_name",分为三类：
            1.目标名称在soc-repo下面modules.bzl中的各name处定义,例://soc-repo:{}/drivers/soc/qcom/smem
            2.目标在kernel_platform/oplus/bazel/oplus_modules.bzl的oplus_ddk_targets中,
               例://vendor/oplus/hardware/radio/kernel:oplus_mdmfeature 、//vendor/oplus/kernel/charger/bazel:{}_oplus_cfg
            3.目标在device/qcom/canoe/techpack_modules.bzl的techpack_targets中, 这个表中是高通原生vendor下的KO
               例：//vendor/qcom/opensource/fingerprint:{}_qbt_handler 、 //vendor/qcom/opensource/spu-kernel:{}_spss_utils
            传递的目标加{}是为了脚本自动format目标名称,{}根据编译的是user、userdebug会自动替换成canoe_perf、canoe_consolidate
            oplus DDK除了充电、camera等少数模块区分perf、consolidate外其余均未区分, 故不区分的模块传递目标时不需加{}
        dtb: 目标dtb
            -> base dtb的修改,生成dtb.img,最终打包在vendor_boot.img
        dtbo: 目标dtbo
            -> dtbo/dtso的修改,生成dtbo.img.如果已按项目打包dtbo则为xxxx-dtbo.img
        techpack dtbs:以devicetree的路径为目标,例:vendor/qcom/proprietary/display-devicetree vendor/qcom/opensource/camera-devicetree
            -> 编译vendor/qcom/下xxxx-devicetree的dtb及dtbo
Usage:
    1 ./kernel_platform/oplus/build/oplus_build.sh -t user -p SM8850 -b kernel
    仅修改kernel代码只需要vmlinux更新
    
    2 ./kernel_platform/oplus/build/oplus_build.sh -t user -p SM8850 -b abl
    仅修改kernel代码只需要vmlinux更新
    
    3 ./kernel_platform/oplus/build/oplus_build.sh -t user -p SM8850 -b //soc-repo:{}/drivers/soc/qcom/smem //vendor/oplus/kernel/boot:oplus_bsp_boot_projectinfo
      ./kernel_platform/oplus/build/oplus_build.sh -t user -p SM8850 -b //vendor/qcom/opensource/spu-kernel:{}_spss_utils //soc-repo:{}/drivers/soc/qcom/smem
    不需要更新vmlinux,只有KO的修改。
    
    4 ./kernel_platform/oplus/build/oplus_build.sh -t user -p SM8850 -b dtbo/dtb
    编译kernel下面的dtb dtbo，如果要编译vendor/qcom下面xxxx-devicetree请参考5
    
    5. ./kernel_platform/oplus/build/oplus_build.sh -t user -p SM8850 -b vendor/qcom/opensource/camera-devicetree
    针对只修改了模块自己dts的场景
"
}

function build_ddk_ko () {
    cd ${KERNEL_ROOT}
    mapfile -t build_flags < "${ANDROID_KP_OUT_DIR}/dist/build_opts.txt"
    temp_file=$(mktemp)
    for ko_target in $KO_TARGETS; do
        ko_out=$(echo "$ko_target" | sed 's#^//##; s#:#/#')
        if ./tools/bazel query "$ko_target"
        then
            ./tools/bazel build "${build_flags[@]}" "$ko_target"
        else
            echo -e "\033[0;31m ${ko_path} is not in tree ko\033[0m"
            exit 1
        fi

        # 查找所有的 *.ko 文件并保存到临时文件
        find "${BAZEL_BIN_OUT}/${ko_out}"  -maxdepth 1 -name "*.ko" > "$temp_file"
        while IFS= read -r ko_file; do
            # 获取文件名
            ko_filename=$(basename "$ko_file")

            # 检查 *.ko 文件是否在 modules.load中
            if grep -q "$ko_filename" "${ANDROID_KERNEL_OUT}/modules.load"; then
                cp "$ko_file" "${ANDROID_KERNEL_OUT}/"
                VENDORBOOTIMAGE=1
                vendorboot_targets="$vendorboot_targets $ko_filename"
            else
                cp "$ko_file" "${ANDROID_KERNEL_OUT}/vendor_dlkm/"
                if ! grep -q "$ko_filename" "${ANDROID_PRODUCT_OUT}/vendor_ramdisk/lib/modules/modules.load"; then
                    vendordlkm_targets="$vendordlkm_targets $ko_filename"
                fi
            fi
            if echo "$ko_target" | grep -q "//vendor/oplus"; then
                cp "$ko_file"  ${ANDROID_KP_OUT_DIR}/oplus_ddk
            elif echo "$ko_target" | grep -q "//vendor/"; then
                cp "$ko_file"  ${ANDROID_KP_OUT_DIR}/dist 
                #在mk中通过BOARD_VENDOR_RAMDISK_KERNEL_MODULES_LOAD指定的高通vendor下的KO
                if grep -q "$ko_filename" "${ANDROID_PRODUCT_OUT}/vendor_ramdisk/lib/modules/modules.load"; then
                    vendorboot_targets="$vendorboot_targets $ko_filename"
                    IAMGES="$IAMGES $ko_filename"
                fi
            else
                cp "$ko_file"  ${ANDROID_KP_OUT_DIR}/dist
            fi

        done < "$temp_file"
    done
    rm -f "$temp_file"
    cd -
}

function build_dtbs () {
    cd ${KERNEL_ROOT}
    mapfile -t build_flags < "${ANDROID_KP_OUT_DIR}/dist/build_opts.txt"
    ./tools/bazel build "${build_flags[@]}" //soc-repo:${KERNEL_TARGET}_${KERNEL_VARIANT}_dtb_build
    rm -rf ${ANDROID_KERNEL_OUT}/kp-dtbs
    mkdir ${ANDROID_KERNEL_OUT}/kp-dtbs
    cp ${BAZEL_BIN_OUT}/soc-repo/${KERNEL_TARGET}_${KERNEL_VARIANT}_dtb_build/*.dtb* ${ANDROID_KERNEL_OUT}/kp-dtbs/
    cp ${BAZEL_BIN_OUT}/soc-repo/${KERNEL_TARGET}_${KERNEL_VARIANT}_dtb_build/*.dtb* ${ANDROID_KP_OUT_DIR}/dist
    cd -
}

function build_techpack_dtbo () {
    cd ${KERNEL_ROOT}
    for DT in $OPLUS_TECKPACK_DTBO; do
        OUT_DIR=${ANDROID_EXT_MODULES_OUT} \
        EXT_MODULES="../${DT}" \
        KERNEL_KIT=${ANDROID_KERNEL_OUT} \
        ./build/build_module.sh dtbs < /dev/null
    done
    cd -
}

function build_merge_dtbs () {
    rm -rf ${ANDROID_KERNEL_OUT}/dtbs
    mkdir ${ANDROID_KERNEL_OUT}/dtbs

    cd ${KERNEL_ROOT}
    OUT_DIR=${ANDROID_EXT_MODULES_OUT} ./build/android/merge_dtbs.sh ${ANDROID_KERNEL_OUT}/kp-dtbs ${ANDROID_EXT_MODULES_COMMON_OUT} ${ANDROID_KERNEL_OUT}/dtbs
    cd -
}

function build_abl () {
    cd ${KERNEL_ROOT}
    ./tools/bazel run --//bootable/bootloader/edk2:target_build_variant=$O_BUILD_TYPE \
		--//bootable/bootloader/edk2:oplus_vnd_build_platform=$O_BUILD_PLATFORM //soc-repo:${KERNEL_TARGET}_${KERNEL_VARIANT}_abl_dist -- --dist_dir ${ANDROID_KP_OUT_DIR}/abl
    for file in LinuxLoader_${O_BUILD_TYPE}.debug unsigned_abl_${O_BUILD_TYPE}.elf ; do
      if [ -e ${ANDROID_KP_OUT_DIR}/abl/${file} ]; then
        if [ ! -e "${ANDROID_ABL_OUT_DIR}/abl-${O_BUILD_TYPE}" ]; then
          mkdir -p ${ANDROID_ABL_OUT_DIR}/abl-${O_BUILD_TYPE}
        fi
        FILE_NAME=$(echo ${file} | sed 's/_'${O_BUILD_TYPE}'//g')
        cp ${ANDROID_KP_OUT_DIR}/abl/${file} ${ANDROID_ABL_OUT_DIR}/abl-${O_BUILD_TYPE}/${FILE_NAME}
      fi
    done
    cd -
}

function get_opt() {
    # 使用getopt解析参数
    OPTS=`getopt -o t:p:b:h --long type:,platform:,build:,help -- "$@"`
    if [[ $? != 0 ]]; then
      usage
      exit 1
    fi
    eval set -- "$OPTS"

    # 提取参数
    while true; do
      case "$1" in
        -t|--type)
          O_BUILD_TYPE=$2
          shift 2
          ;;
        -p|--platform)
          O_BUILD_PLATFORM=$2
          shift 2
          ;;
        -b|--build)
          O_BUILD_TARGETS=$2
          shift 2
          ;;
        -h|--help)
          usage
          exit 0
          ;;
        --)
          shift
          break
          ;;
        *)
          usage
          exit 0
          ;;
      esac
    done
    O_BUILD_TARGETS+=" $@"
}

get_opt "$@"

echo "O_BUILD_TYPE=$O_BUILD_TYPE"
echo "O_BUILD_PLATFORM=$O_BUILD_PLATFORM"
echo "O_BUILD_TARGETS=$O_BUILD_TARGETS"
if [ -z "$O_BUILD_TYPE" ] || [ -z "$O_BUILD_PLATFORM" ] || [ -z "$O_BUILD_TARGETS" ];then
    usage
    exit 1
fi

TOPDIR=$(readlink -f $(cd $(dirname "$0");cd ../../../;pwd))
pushd $TOPDIR

#使能bazel cache
source  vendor/oplus/build/ci/functions.sh
fn_try_speedup_build

#获取kernel编译KERNEL_TARGET、KERNEL_VARIANT
source vendor/oplus/build/platform/vnd/$O_BUILD_PLATFORM/envsetup.sh
KERNEL_TARGET=${OPLUS_KERNEL_PARAM:-canoe}
if [ "$O_BUILD_TYPE" == "userdebug" ];then
    KERNEL_VARIANT=consolidate
else
    KERNEL_VARIANT=${OPLUS_USER_KERNEL_VARIANT:-perf}
fi

#set env
export TARGET_BUILD_VARIANT=$O_BUILD_TYPE
export ANDROID_BUILD_TOP=${TOPDIR}
export ANDROID_KERNEL_OUT=${ANDROID_BUILD_TOP}/device/qcom/${KERNEL_TARGET}-kernel
export ANDROID_ABL_OUT_DIR=${ANDROID_KERNEL_OUT}/kernel-abl
export ANDROID_KP_OUT_DIR="out/msm-kernel-${KERNEL_TARGET}-${KERNEL_VARIANT}"
export ANDROID_PRODUCT_OUT=${TOPDIR}/out/target/product/$COMPILE_PLATFORM
export TARGET_BOARD_PLATFORM=$KERNEL_TARGET
export JAVA_HOME=${TOPDIR}/prebuilts/jdk/jdk21/linux-x86
export KERNEL_ROOT=${TOPDIR}/kernel_platform
export BAZEL_BIN_OUT=${KERNEL_ROOT}/bazel-bin
export ANDROID_EXT_MODULES_COMMON_OUT=${ANDROID_PRODUCT_OUT}/obj/DLKM_OBJ
export ANDROID_EXT_MODULES_OUT=${ANDROID_EXT_MODULES_COMMON_OUT}/kernel_platform
#是否全编译环境检测
if ! cat "$ANDROID_PRODUCT_OUT/previous_build_config.mk" | grep -qw "$O_BUILD_TYPE"; then
    echo -e "
\033[0;31m当前编译类型跟全编译类型不同，请跟全编译选择相同编译类型，当前:$O_BUILD_TYPE，全编译:$(cat "$ANDROID_PRODUCT_OUT/previous_build_config.mk")
    \033[0m"
    exit 1
fi
if [ ! -e $ANDROID_KERNEL_OUT ];then
    echo -e "
\033[0;31mit's new downloaded code, kernel has not yet been compiled, please build kpl with:
OPLUS_BUILD_JSON=build.json ./mk_android.sh -t user/userdebug -b kpl
    \033[0m"
    exit 1
fi

#设置编译控制宏
RECOMPILE_KERNEL=0
RECOMPILE_ABL=0
RECOMPILE_TECHPACK_DTBO=0
RECOMPILE_MERGE_DTBS=0
KO_TARGETS=""
OPLUS_TECKPACK_DTBO=""
vendorboot_targets=""
vendordlkm_targets=""
IAMGES=""
for target in $O_BUILD_TARGETS; do
    if [ "$target" = "kernel" ]; then
        RECOMPILE_KERNEL=1
        BOOTIMAGE=1
        SYSTEM_DLKMIMAGE=1
    elif [ "$target" = "dtbo" ];then
        RECOMPILE_DTBS=1
        RECOMPILE_MERGE_DTBS=1
        DTBOIMAGE=1
    elif [ "$target" = "dtb" ];then
        RECOMPILE_DTBS=1
        RECOMPILE_MERGE_DTBS=1
        VENDORBOOTIMAGE=1
    elif [ "$target" = "abl" ]; then
        RECOMPILE_ABL=1
        ABOOT=1
    elif echo "$target" | grep -Eq "//vendor/|//soc-repo"; then
        target=$(echo "$target" | sed "s/{}/${KERNEL_TARGET}_${KERNEL_VARIANT}/")
        KO_TARGETS="$KO_TARGETS $target"
    elif [ "${target%-devicetree}" != "$target" ]; then
        if [ -d "$target" ];then
            OPLUS_TECKPACK_DTBO="$OPLUS_TECKPACK_DTBO $target"
            RECOMPILE_MERGE_DTBS=1
            DTBOIMAGE=1
        else
            echo -e "\033[0;31m $target 路径不存在\033[0m"
        fi
    elif echo "$target" | grep -q "vendor"; then
        echo -e "\033[0;31m veddor下Android.mk维护的KO直接使用./mk_android.sh -t user/userdebug -m/-n xxxx\033[0m"
        echo -e "\033[0;31m 不支持$target\033[0m"
        exit 1
    else
        echo  -e "\033[0;31mERROR:无法处理 $target\033[0m"
        WRONG_TARGET=1
    fi
done

if [ "$WRONG_TARGET" == "1" ];then
    usage
    exit 1
fi

#去除开头空格
KO_TARGETS="${KO_TARGETS# }"

echo "RECOMPILE_KERNEL=$RECOMPILE_KERNEL"
echo "RECOMPILE_ABL=$RECOMPILE_ABL"
echo "RECOMPILE_TECHPACK_DTBO=$RECOMPILE_TECHPACK_DTBO"
echo "RECOMPILE_MERGE_DTBS=$RECOMPILE_MERGE_DTBS"
echo "KO_TARGETS=$KO_TARGETS"
echo "OPLUS_TECKPACK_DTBO=$OPLUS_TECKPACK_DTBO"
echo "KERNEL_TARGET=$KERNEL_TARGET"
echo "KERNEL_VARIANT=$KERNEL_VARIANT"

#备份 $ANDROID_KERNEL_OUT/.config  $ANDROID_KERNEL_OUT/Module.symvers
#防止编译vendor_boot、vendor_dlkm时会触发高通原生KO编译
tmpdir=$(mktemp -d)
rsync -a $ANDROID_KERNEL_OUT/.config $tmpdir
rsync -a $ANDROID_KERNEL_OUT/Module.symvers $tmpdir

#处理 ko 编译
if [ -n "$KO_TARGETS" ];then
    echo -e "\033[0;32mbuild ddk ko $KO_TARGETS\033[0m"
    build_ddk_ko
fi

if [ "$RECOMPILE_DTBS" == "1" ];then
    echo -e "\033[0;32mbuild base dtb/dtbo\033[0m"
    build_dtbs
fi

if [ "$RECOMPILE_ABL" == "1" ];then
    echo -e "\033[0;32mbuild abl\033[0m"
    build_abl
fi

if [ -n "$OPLUS_TECKPACK_DTBO" ];then
    echo -e "\033[0;32mbuild techpack dtbo\033[0m"
    build_techpack_dtbo
fi

if [ "$RECOMPILE_MERGE_DTBS" == "1" ];then
    echo -e "\033[0;32mMerging vendor devicetree overlay\033[0m"
    build_merge_dtbs
fi

#整体编译kernel，不需要编译的部分已经加宏控制
if [ "$RECOMPILE_KERNEL" == "1" ];then
    RECOMPILE_KERNEL=$RECOMPILE_KERNEL RECOMPILE_ABL=0  RECOMPILE_TECHPACK_DTBO=0 RECOMPILE_MERGE_DTBS=0 \
    ./kernel_platform/build/android/prepare_vendor.sh $KERNEL_TARGET $KERNEL_VARIANT < /dev/null
fi

#如果内容一样就带时间戳还原.config Module.symvers
if diff -q $ANDROID_KERNEL_OUT/.config $tmpdir/.config && diff -q $ANDROID_KERNEL_OUT/Module.symvers $tmpdir/Module.symvers;then
    rsync -a $tmpdir/.config $ANDROID_KERNEL_OUT/
    rsync -a $tmpdir/Module.symvers $ANDROID_KERNEL_OUT/
fi
rm -rf $tmpdir

#生成对应img
if [ "$SYSTEM_DLKMIMAGE" == "1" ]; then
    IAMGES="$IAMGES system_dlkmimage"
fi

if [ "$BOOTIMAGE" == "1" ]; then
    IAMGES="$IAMGES bootimage"
fi

if [ "$RECOMPILE_DTBS" == "1" ];then
    IAMGES="$IAMGES out/target/product/$COMPILE_PLATFORM/dtb.img"
fi

if [ "$VENDORBOOTIMAGE" == "1" ]; then
    IAMGES="$IAMGES vendorbootimage"
fi

if [ "$VENDOR_DLKMIMAGE" == "1" ]; then
    IAMGES="$IAMGES vendor_dlkmimage"
fi

if [ "$ABOOT" == "1" ];then
    IAMGES="$IAMGES aboot"
fi

if [ "$DTBOIMAGE" == "1" ];then
    IAMGES="$IAMGES dtboimage"
fi
#去除开头空格
IAMGES="${IAMGES# }"

#编译img
if [ -n "$IAMGES" ];then
    echo -e "\033[0;32m./mk_android.sh -t $O_BUILD_TYPE -n "$IAMGES"\033[0m"
    ./mk_android.sh -t $O_BUILD_TYPE -n "$IAMGES"
fi

#输出验证建议信息
tput setaf 2
echo
if [ -n "$vendorboot_targets" ];then
    echo "$vendorboot_targets 打包在vendor_boot,需要编译vendor_boot.img验证修改"
    echo
fi
if [ -n "$vendordlkm_targets" ];then
    echo "$vendordlkm_targets 打包在vendor_dlkm,可以直接push或者编译vendor_dlkm.img验证修改"
    echo
fi
echo "直接KO产物在device/qcom/canoe-kernel下,直接编出的KO未strip,push验证前需要先做strip
kernel_platform/prebuilts/clang/host/linux-x86/llvm-binutils-stable/llvm-strip -S *.ko -o yourpath/*.ko"
echo
if [ -n "$IAMGES" ];then
    echo "根据您指定的target，已为您编译出$ANDROID_PRODUCT_OUT"
    tput setaf 1
    echo ">>> $IAMGES <<<"
    tput setaf 2
    echo "请确保您没有修改过Android.mk Android.bp否则-N生成的image会失效"
    echo
fi
echo "如果发现本脚本使用-n生成的image有问题，可以尝试使用./mk_android.sh -t user -m xxx"
tput sgr0
popd
