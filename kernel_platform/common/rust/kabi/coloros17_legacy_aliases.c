// SPDX-License-Identifier: GPL-2.0
/*
 * ColorOS .102's retained Rust Binder versions were generated with LLVM
 * MergeFunctions enabled. The .117 KCFI fix (upstream 29b03d1de967) must keep
 * merging disabled: some folded Default bodies had the wrong KCFI prefix.
 *
 * Preserve only the six old alias *descriptions*, recovered by rebuilding
 * the qualified pre-.117 compiler policy offline. Do not restore its code or
 * KCFI hashes. These are normal Android KABI rules, not numeric CRC overrides.
 * Kbuild extracts only this discarded annotation section into the new Rust
 * objects; all allocated runtime sections, symbols and relocations stay intact.
 *
 * The integer aliases have identical ARM64 width, signedness and alignment;
 * Formatter/Result layouts and the four indirect-call KCFI prefixes match.
 * Both Default bodies remain byte-identical zero initializers. Their old
 * alias return storage has the same size/alignment/return convention, and
 * retained Binder uses only direct CALL26 relocations for these two exports.
 * Keep the corrected .117 KCFI hashes, not the old merged hashes.
 *
 * Revalidate source types, return lowering, KCFI and the actual consumer set
 * before changing Rust, these types or the firmware contract. Do not extend
 * this list on the strength of a CRC match alone.
 */
#include <linux/alloc_tag.h>
#include <linux/android_kabi.h>
#include <linux/irqflags.h>
#include <linux/rbtree.h>
#include <linux/stddef.h>

static_assert(sizeof(long) == 8 && sizeof(void *) == 8);
static_assert(sizeof(struct rb_node) == 24);
static_assert(sizeof(struct alloc_tag_module_section) == 24);
static_assert(__alignof__(struct rb_node) == 8);
static_assert(__alignof__(struct alloc_tag_module_section) == 8);
static_assert(offsetof(struct rb_node, __rb_parent_color) == 0);
static_assert(offsetof(struct rb_node, rb_right) == 8);
static_assert(offsetof(struct rb_node, rb_left) == 16);
static_assert(offsetof(struct alloc_tag_module_section, start_addr) == 0);
static_assert(offsetof(struct alloc_tag_module_section, end_addr) == 8);
static_assert(offsetof(struct alloc_tag_module_section, size) == 16);
static_assert(sizeof(struct rb_root) == 8);
static_assert(sizeof(class_irq_t) == 8);
static_assert(__alignof__(struct rb_root) == 8);
static_assert(__alignof__(class_irq_t) == 8);
static_assert(offsetof(struct rb_root, rb_node) == 0);

ANDROID_KABI_TYPE_STRING(
 "_RNvXs14_NtNtCs3o2tGsuHyou_4core3fmt3numyNtB8_8LowerHex3fmt",
 "subprogram ( formal_parameter pointer_type &usize { base_type usize "
 "byte_size(8) encoding(7) } self , formal_parameter pointer_type &mut "
 "core::fmt::Formatter { s#core::fmt::Formatter } f ) -> "
 "s#'core::result::Result<(), core::fmt::Error>'");
ANDROID_KABI_TYPE_STRING(
 "_RNvXs4_NtNtNtCs3o2tGsuHyou_4core3fmt3num3impxNtB9_7Display3fmt",
 "subprogram ( formal_parameter pointer_type &isize { base_type isize "
 "byte_size(8) encoding(5) } self , formal_parameter pointer_type &mut "
 "core::fmt::Formatter { s#core::fmt::Formatter } f ) -> "
 "s#'core::result::Result<(), core::fmt::Error>'");
ANDROID_KABI_TYPE_STRING(
 "_RNvXs6_NtNtNtCs3o2tGsuHyou_4core3fmt3num3impjNtB9_7Display3fmt",
 "subprogram ( formal_parameter pointer_type &u64 { base_type u64 "
 "byte_size(8) encoding(7) } self , formal_parameter pointer_type &mut "
 "core::fmt::Formatter { s#core::fmt::Formatter } f ) -> "
 "s#'core::result::Result<(), core::fmt::Error>'");
ANDROID_KABI_TYPE_STRING(
 "_RNvXsz_NtNtCs3o2tGsuHyou_4core3fmt3numjNtB7_8UpperHex3fmt",
 "subprogram ( formal_parameter pointer_type &u64 { base_type u64 "
 "byte_size(8) encoding(7) } self , formal_parameter pointer_type &mut "
 "core::fmt::Formatter { s#core::fmt::Formatter } f ) -> "
 "s#'core::result::Result<(), core::fmt::Error>'");
ANDROID_KABI_TYPE_STRING(
 "_RNvXs1P_NtCsg7kyWky7wBq_8bindings12bindings_rawNtB6_7rb_nodeNtNtCs3o2tGsuHyou_4core7default7Default7default",
 "subprogram ( ) -> s#bindings::bindings_raw::alloc_tag_module_section");
ANDROID_KABI_TYPE_STRING(
 "_RNvXs1Q_NtCsg7kyWky7wBq_8bindings12bindings_rawNtB6_7rb_rootNtNtCs3o2tGsuHyou_4core7default7Default7default",
 "subprogram ( ) -> s#bindings::bindings_raw::class_irq_t");
