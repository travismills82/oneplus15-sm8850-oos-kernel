def _ddk_unpacked_headers_impl(ctx):
    out_dir = ctx.actions.declare_directory(ctx.label.name)

    tarball = ctx.files.hdr_archive[0]

    cmd = """
        mkdir -p "{out_dir}"
        tar --strip-components=2 -C "{out_dir}" -xzf "{tar_file}"
    """.format(out_dir = out_dir.path, tar_file = tarball.path)

    ctx.actions.run_shell(
        mnemonic = "DDKUnpackedUapiHeaders",
        inputs = [tarball],
        outputs = [out_dir],
        progress_message = "Unpacking DDK UAPI headers {}".format(ctx.label),
        command = cmd,
    )

    return [
        DefaultInfo(files = depset([out_dir])),
    ]

_ddk_unpacked_headers = rule(
    implementation = _ddk_unpacked_headers_impl,
    doc = """Unpack `ddk_uapi_headers`'s uapi header tarball'""",
    attrs = {
        "hdr_archive": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
    },
)

def ddk_uapi_headers_cc_library(name, header_archive):
    unpacked_header_name = name + "_unpacked_headers"
    _ddk_unpacked_headers(
        name = unpacked_header_name,
        hdr_archive = header_archive,
    )

    native.cc_library(
        name = name,
        hdrs = [":" + unpacked_header_name],
        includes = [unpacked_header_name],
        visibility = ["//visibility:public"],
    )
