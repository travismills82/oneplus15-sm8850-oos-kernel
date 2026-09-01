# OOS 16.0.10.500 ACK 6.12.26 SHA-512 module-signing toolchain

## Scope

Linux 6.12.26 commit `e62c31802dcc76f89df73f4b18cffedb8d4a3274`
selects SHA-512 as the default module-signing digest.  This record identifies
and pins the Android generation-5 host tool used to satisfy that requirement.
It does not change the module-signing key, trusted certificates,
`MODULE_SIG_FORCE`, MODVERSIONS, protected exports, or any target-kernel
runtime policy.

## Official Android prebuilt

| Field | Value |
|---|---|
| Repository | `https://android.googlesource.com/platform/prebuilts/build-tools` |
| Branch | `main-kernel-2025` |
| Commit | `574765b494f226284968a8d156437e4c932aad61` |
| Commit date | `2025-04-29` |
| Subject | `Update build-tools to ab/13424132` |
| Binary | `linux_musl-x86/bin/openssl` |
| Git blob | `01ef6b3b6e2dc31f4dc7bcb56a292d9842cc217f` |
| SHA-256 | `44618139874abf4a78a3d36ddfe42918b12995d1a69f3a526e78b063ef36e91f` |
| Version | `OpenSSL 1.1.1n 15 Mar 2022` |

The OnePlus/CLO build-tools project pinned in this workspace is commit
`951b0e9b947327fe485b2553faef2ed34f8e148d`.  Its
`linux_musl-x86/bin/openssl` is byte-identical to the official Android blob
above.  Kleaf's `prebuilt_tool` transition selects this musl variant for the
Linux execution platform.  The distinct `linux-x86/bin/openssl` has SHA-256
`82eb57bd583315a3cba93a15a8ea476b7d40f8a1053586f1615a41645649b30a`
and is not the artifact selected by this build graph.  No broad prebuilt-tree
replacement is required.

The separately pinned
`prebuilts/kernel-build-tools/linux-x86/lib64/libcrypto-host.so` has SHA-256
`6b10191f02f6b8451db7f94218cd252dc0ec7180400f86e8448de44874cb45c1`.
It is the BoringSSL-compatible library used to link Kbuild's `sign-file`; it
can append a supplied raw CMS object but cannot itself create a SHA-512 CMS
signature.

## Disposable capability proof

A disposable RSA key, self-signed certificate, and 4 KiB payload were created
under `/tmp/op15-sha512-musl-probe.RoZnCW`.  No project or production private
key was used.  The pinned Android musl OpenSSL generated a detached, binary,
certificate-free CMS signature with:

```text
cms -sign -binary -noattr -nocerts -md sha512
```

The same pinned executable verified that signature successfully, and its CMS
printer reported `sha512` (`2.16.840.1.101.3.4.2.3`).  The detached signature
SHA-256 was
`645a3d0697fea2b1797d48c35fce890e7bc1c9f4f685d9f706b2e2282078e003`.

The repository wrapper then passed the detached CMS object to the exact
compiled Kbuild `scripts/sign-file` helper in raw-signature mode.  The result
was 4,543 bytes: the original 4,096-byte payload, a 407-byte CMS object, and
the standard 40-byte Linux module-signature metadata/trailer.  It ended in:

```text
~Module signature appended~
```

The complete wrapper-produced probe SHA-256 was
`e3bd92c659be495a3b4cdf4deaa1f68d5a2e363824aa3b3b194b18c77cc130e2`.

Capability result:

```text
signature generation: PASS
SHA-512: PASS
verification: PASS
kernel trailer append: PASS
```

Kbuild supplies `certs/signing_key.x509` in DER form.  A second disposable
probe passed a DER copy of the same test certificate to the wrapper.  The
wrapper used the pinned OpenSSL to create a temporary PEM representation for
`cms -sign`, retained the original DER certificate for Kbuild key-ID
derivation, and appended the same valid SHA-512 signature and Linux trailer.
The DER-input wrapper probe passed with result SHA-256
`e3bd92c659be495a3b4cdf4deaa1f68d5a2e363824aa3b3b194b18c77cc130e2`.

## Kleaf integration

Kleaf's `//build/kernel:hermetic-tools` already declares
`//prebuilts/build-tools:openssl` in `_PREBUILT_TOOLS`.  The kernel action
therefore receives the executable as a declared input and places only the
hermetic tool directory at the front of its build `PATH`.

`scripts/Makefile.modinst` invokes `scripts/sign-file-cms`, which resolves the
Kleaf-provided `openssl` and refuses it unless the executable SHA-256 exactly
matches the pinned Android blob above.  Consequently a mutable host OpenSSL
cannot be accepted even if a non-Kleaf caller provides an unexpected `PATH`.
The wrapper:

1. refuses any digest other than SHA-512;
2. uses the pinned OpenSSL only to generate the detached CMS signature;
3. normalizes Kbuild's DER public certificate to temporary PEM using that
   same pinned tool, without changing the certificate identity;
4. uses the normal compiled Kbuild `scripts/sign-file` only to append that CMS
   object and the standard module-signature trailer;
5. preserves `KBUILD_SIGN_PIN` handling; and
6. uses the existing Kbuild signing key and generated X.509 certificate.

The small `sign-file.c` change permits SHA-512 only when an already-created
raw signature is supplied.  Its original BoringSSL signing path remains
restricted to SHA-1, so it cannot falsely claim native SHA-512 support.

## Build gate

The final clean `//common:kernel_aarch64` and Canoe distribution results, plus
the signer and `sig_hashalgo` inventory for every generated module, must be
recorded with the complete 6.12.26 compatibility validation.  A standalone
capability pass does not by itself qualify a kernel image.
