load("//build/kernel/kleaf:hermetic_tools.bzl", "hermetic_genrule")

def define_top_level_config(target):
    """Define common top-level variables in build.config"""
    rule_name = "{}_top_level_config".format(target)
    hermetic_genrule(
        name = rule_name,
        srcs = [],
        outs = ["build.config.bazel.top.level.{}".format(target)],
        cmd = """
          cat << 'EOF' > "$@"
# === define_top_level_config ===
BUILDING_WITH_BAZEL=true
# === end define_top_level_config ===
EOF
        """,
    )

    return ":{}".format(rule_name)

def gen_config_without_source_lines(build_config, target):
    """Replace "." or "source" lines in build.config files with shell null operator"""
    rule_name = "{}.{}".format(target, build_config)
    out_file_name = rule_name + ".generated"
    hermetic_genrule(
        name = rule_name,
        srcs = [build_config],
        outs = [out_file_name],
        cmd = """
          sed -e 's/^ *\\. /: # &/' \
              -e 's/^ *source /: # &/' \
              $(location {}) > "$@"
        """.format(build_config),
    )

    return ":" + rule_name

def get_out_dir(msm_target, variant):
    if "allyes" in msm_target:
        return "out/msm-kernel-{}-{}".format(msm_target.replace("_", "-"), variant.replace("-", "_"))
    return "out/msm-kernel-{}_{}".format(msm_target.replace("-", "_"), variant.replace("-", "_"))

def define_signing_keys():
    hermetic_genrule(
        name = "signing_key",
        srcs = ["//soc-repo:certs/qcom_x509.genkey"],
        outs = ["signing_key.pem"],
        tools = ["//prebuilts/build-tools:openssl"],
        cmd = """
          $(location //prebuilts/build-tools:openssl) req -new -nodes -utf8 -sha256 -days 36500 \
            -batch -x509 -config $(location //msm-kernel:certs/qcom_x509.genkey) \
            -outform PEM -out "$@" -keyout "$@"
        """,
    )

    hermetic_genrule(
        name = "verity_key",
        srcs = ["//soc-repo:certs/qcom_x509.genkey"],
        outs = ["verity_cert.pem", "verity_key.pem"],
        tools = ["//prebuilts/build-tools:openssl"],
        cmd = """
          $(location //prebuilts/build-tools:openssl) req -new -nodes -utf8 -newkey rsa:1024 -days 36500 \
            -batch -x509 -config $(location //msm-kernel:certs/qcom_x509.genkey) \
            -outform PEM -out $(location verity_cert.pem) -keyout $(location verity_key.pem)
        """,
    )
