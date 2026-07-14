load("@bazel_skylib//rules:write_file.bzl", "write_file")

def _fragment_config_impl(ctx):
    executable = ctx.actions.declare_file("{}/config.sh".format(ctx.label.name))

    script = """
        set -e

        convert() {{
            echo "$(basename -s .bzl $2 | sed -e 's/\\./_/g')_config = {{"
            echo "    # keep sorted"
            # expr 1 converts CONFIG_FOO="something"
            # expr 2 converts CONFIG_FOO=y
            # expr 3 converts # CONFIG_FOO is not set
            sed -e 's/\\(CONFIG_.*\\)="\\([^"]*\\)"/    "\\1": "\\\\"\\2\\\\"",/' \
                -e 's/\\(CONFIG_.*\\)=\\([^"]*\\)/    "\\1": "\\2",/' \
                -e 's/# \\(CONFIG_.*\\) is not set/    "\\1": "n",/' \
                $1
            echo "}}"
        }}

        temp=$(mktemp)
        trap "rm $temp" EXIT
        {config} "${{@:-menuconfig}}" --file ${{temp}}

        dest=$BUILD_WORKSPACE_DIRECTORY/{dest}
        if which buildifier > /dev/null ; then
            convert ${{temp}} ${{dest}} | buildifier -lint=fix -warnings=all > ${{dest}}
        else
            convert ${{temp}} ${{dest}} > ${{dest}}
        fi

        echo Updated $(realpath ${{dest}})
    """.format(
        config = ctx.executable.ddk_config.short_path,
        dest = ctx.file.dest.short_path,
    )

    ctx.actions.write(executable, script, is_executable = True)

    return [
        DefaultInfo(
            executable = executable,
            runfiles = ctx.runfiles(transitive_files = ctx.attr.ddk_config[DefaultInfo].files).merge(ctx.attr.ddk_config[DefaultInfo].default_runfiles),
        ),
    ]

fragment_menuconfig = rule(
    implementation = _fragment_config_impl,
    executable = True,
    attrs = {
        "ddk_config": attr.label(
            mandatory = True,
            executable = True,
            cfg = "target",
            doc = "The ddk_config to run menuconfig against",
        ),
        "dest": attr.label(
            mandatory = True,
            allow_single_file = True,
            doc = "The destination source file.",
        ),
    },
    doc = "Wraps ddk_config executable to generate configs/*.bzl files",
)

def define_defconfig_fragment(name, out, config):
    """Generate a defconfig fragment from the given configuration.

    This rule generates a defconfig fragment from a target/variant's
    configuration. Bazel needs to be mostly aware of the target/variant
    configuration in order to know how to generate module dependency graph
    as some modules will not exist in certain configurations. Thus, we let
    Bazel be the source of truth for the defconfig fragment which is passed
    to Kbuild during compilation.

    Note that this rule does not actually compile anything; it only writes
    files.

    Args:
      name: A unique name for this rule.
      out: The file to generate.
      config: A dictionary of key/value pairs that will be written as lines in the output file.
    """

    content = []
    for k, v in config.items():
        if v == "n":
            content.append("# {} is not set".format(k))
        else:
            content.append("{}={}".format(k, v))

    write_file(
        name,
        out,
        content,
    )
