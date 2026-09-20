# Linux 6.12.80 first KMI5 test strategy

The repository owner's selected first post-6.12.52 physical checkpoint is
Linux 6.12.80 while retaining the Android 16 generation-5 KMI contract.  This
supersedes the earlier plan to make 6.12.110 the first physical test.  Work
must stop at 6.12.80 until its static and physical result is known.

The source history retains every applicable non-merge Linux stable commit from
v6.12.52 through v6.12.80 individually and in dependency order.  Android
Common is used only for reviewed KMI repairs; Android's generation-6 thaw is
not imported.  OnePlus/OxygenOS compatibility changes remain separate from
the stable commits.

The 6.12.80 artifact may be produced only after the final source passes the
symtypes and strict-KMI build, enforced common ABI comparison, Canoe ABI and
normal distribution builds, retained stock-provider replay, private-ABI and
namespace review, SHA-512 signing verification, FBE/storage review, and the
established Android-v4 boot-container checks.  An ABI-reference update is not
authorized by this checkpoint.

The physical test is boot-only.  It must preserve stock `system_dlkm`,
`vendor_dlkm`, `vendor_boot`, DTBO, VBMeta, metadata, and userdata.  The first
oracle is TWRP metadata/PIN decryption and existing-userdata F2FS/inlinecrypt;
Android testing follows only after that passes.
