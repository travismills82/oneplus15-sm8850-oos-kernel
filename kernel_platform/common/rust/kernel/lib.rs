// SPDX-License-Identifier: GPL-2.0

//! The `kernel` crate.
//!
//! This crate contains the kernel APIs that have been ported or wrapped for
//! usage by Rust code in the kernel and is shared by all of them.
//!
//! In other words, all the rest of the Rust code in the kernel (e.g. kernel
//! modules written in Rust) depends on [`core`] and this crate.
//!
//! If you need a kernel C API that is not ported or wrapped yet here, then
//! do so first instead of bypassing this crate.

#![no_std]
#![feature(arbitrary_self_types)]
#![feature(asm_goto)]
#![feature(coerce_unsized)]
#![feature(dispatch_from_dyn)]
#![feature(inline_const)]
#![feature(linkage)]
#![feature(lint_reasons)]
#![feature(unsize)]
#![feature(used_with_arg)]

// Ensure conditional compilation based on the kernel configuration works;
// otherwise we may silently break things like initcall handling.
#[cfg(not(CONFIG_RUST))]
compile_error!("Missing kernel configuration for conditional compilation");

// Allow proc-macros to refer to `::kernel` inside the `kernel` crate (this crate).
extern crate self as kernel;

pub use ffi;

pub mod alloc;
#[cfg(CONFIG_BLOCK)]
pub mod block;
mod build_assert;
pub mod cred;
pub mod device;
pub mod error;
#[cfg(CONFIG_RUST_FW_LOADER_ABSTRACTIONS)]
pub mod firmware;
pub mod fs;
pub mod init;
pub mod ioctl;
pub mod jump_label;
#[cfg(CONFIG_KUNIT)]
pub mod kunit;
pub mod list;
pub mod miscdevice;
pub mod mm;
#[cfg(CONFIG_NET)]
pub mod net;
pub mod page;
pub mod page_size_compat;
pub mod pid_namespace;
pub mod prelude;
pub mod print;
pub mod rbtree;
pub mod security;
pub mod seq_file;
pub mod sizes;
mod static_assert;
#[doc(hidden)]
pub mod std_vendor;
pub mod str;
pub mod sync;
pub mod task;
pub mod time;
pub mod tracepoint;
pub mod types;
pub mod uaccess;
pub mod workqueue;

#[doc(hidden)]
pub use bindings;
pub use macros;
pub use uapi;

#[doc(hidden)]
pub use build_error::build_error;

/// Prefix to appear before log messages printed from within the `kernel` crate.
const __LOG_PREFIX: &[u8] = b"rust_kernel\0";

/*
 * ANDROID: The cgroup freeze-time backport consumes a reserved slot through
 * ANDROID_BACKPORT_USE(). Rust bindgen gives that otherwise anonymous union a
 * generated field name, which changes gendwarfksyms CRCs for exports that
 * transitively reference task_struct even though the cgroup layout is
 * unchanged. Preserve the generation-5 Rust module type string while keeping
 * the official ACK C and bindgen layouts visible to the ABI tooling.
 */
#[cfg(CONFIG_MODVERSIONS)]
#[used]
#[link_section = ".discard.gendwarfksyms.kabi_rules"]
#[linkage = "internal"]
static __GENDWARFKSYMS_RULE_CGROUP: [u8; 5457] =
    *b"1\0type_string\0s#bindings::bindings_raw::cgroup\0structure_type bindings::bindings_raw::cgroup { member s#bindings::bindings_raw::cgroup_subsys_state self_ accessibility(1) data_member_location(0) , member base_type usize byte_size(8) encoding(7) flags accessibility(1) data_member_location(216) , member base_type i32 byte_size(4) encoding(5) level accessibility(1) data_member_location(224) , member base_type i32 byte_size(4) encoding(5) max_depth accessibility(1) data_member_location(228) , member base_type i32 byte_size(4) encoding(5) nr_descendants accessibility(1) data_member_location(232) , member base_type i32 byte_size(4) encoding(5) nr_dying_descendants accessibility(1) data_member_location(236) , member base_type i32 byte_size(4) encoding(5) max_descendants accessibility(1) data_member_location(240) , member base_type i32 byte_size(4) encoding(5) nr_populated_csets accessibility(1) data_member_location(244) , member base_type i32 byte_size(4) encoding(5) nr_populated_domain_children accessibility(1) data_member_location(248) , member base_type i32 byte_size(4) encoding(5) nr_populated_threaded_children accessibility(1) data_member_location(252) , member base_type i32 byte_size(4) encoding(5) nr_threaded_children accessibility(1) data_member_location(256) , member base_type u32 byte_size(4) encoding(7) kill_seq accessibility(1) data_member_location(260) , member pointer_type *mut bindings::bindings_raw::kernfs_node { s#bindings::bindings_raw::kernfs_node } kn accessibility(1) data_member_location(264) , member s#bindings::bindings_raw::cgroup_file procs_file accessibility(1) data_member_location(272) , member s#bindings::bindings_raw::cgroup_file events_file accessibility(1) data_member_location(328) , member array_type[4] { s#bindings::bindings_raw::cgroup_file } psi_files accessibility(1) data_member_location(384) , member base_type u16 byte_size(2) encoding(7) subtree_control accessibility(1) data_member_location(608) , member base_type u16 byte_size(2) encoding(7) subtree_ss_mask accessibility(1) data_member_location(610) , member base_type u16 byte_size(2) encoding(7) old_subtree_control accessibility(1) data_member_location(612) , member base_type u16 byte_size(2) encoding(7) old_subtree_ss_mask accessibility(1) data_member_location(614) , member array_type[8] { pointer_type *mut bindings::bindings_raw::cgroup_subsys_state { s#bindings::bindings_raw::cgroup_subsys_state } } subsys accessibility(1) data_member_location(616) , member array_type[8] { base_type i32 byte_size(4) encoding(5) } nr_dying_subsys accessibility(1) data_member_location(680) , member pointer_type *mut bindings::bindings_raw::cgroup_root { s#bindings::bindings_raw::cgroup_root } root accessibility(1) data_member_location(712) , member s#bindings::bindings_raw::list_head cset_links accessibility(1) data_member_location(720) , member array_type[8] { s#bindings::bindings_raw::list_head } e_csets accessibility(1) data_member_location(736) , member pointer_type *mut bindings::bindings_raw::cgroup { s#bindings::bindings_raw::cgroup } dom_cgrp accessibility(1) data_member_location(864) , member pointer_type *mut bindings::bindings_raw::cgroup { s#bindings::bindings_raw::cgroup } old_dom_cgrp accessibility(1) data_member_location(872) , member pointer_type *mut bindings::bindings_raw::cgroup_rstat_cpu { s#bindings::bindings_raw::cgroup_rstat_cpu } rstat_cpu accessibility(1) data_member_location(880) , member s#bindings::bindings_raw::list_head rstat_css_list accessibility(1) data_member_location(888) , member array_type[7] { base_type u64 byte_size(8) encoding(7) } __bindgen_padding_0 accessibility(1) data_member_location(904) , member s#bindings::bindings_raw::cacheline_padding _pad_ accessibility(1) data_member_location(960) , member pointer_type *mut bindings::bindings_raw::cgroup { s#bindings::bindings_raw::cgroup } rstat_flush_next accessibility(1) data_member_location(960) , member s#bindings::bindings_raw::cgroup_base_stat last_bstat accessibility(1) data_member_location(968) , member s#bindings::bindings_raw::cgroup_base_stat bstat accessibility(1) data_member_location(1000) , member s#bindings::bindings_raw::prev_cputime prev_cputime accessibility(1) data_member_location(1032) , member s#bindings::bindings_raw::list_head pidlists accessibility(1) data_member_location(1056) , member s#bindings::bindings_raw::mutex pidlist_mutex accessibility(1) data_member_location(1072) , member s#bindings::bindings_raw::wait_queue_head offline_waitq accessibility(1) data_member_location(1120) , member s#bindings::bindings_raw::work_struct release_agent_work accessibility(1) data_member_location(1144) , member pointer_type *mut bindings::bindings_raw::psi_group { s#bindings::bindings_raw::psi_group } psi accessibility(1) data_member_location(1176) , member s#bindings::bindings_raw::cgroup_bpf bpf accessibility(1) data_member_location(1184) , member s#bindings::bindings_raw::cgroup_freezer_state freezer accessibility(1) data_member_location(1904) , member pointer_type *mut bindings::bindings_raw::bpf_local_storage { s#bindings::bindings_raw::bpf_local_storage } bpf_cgrp_storage accessibility(1) data_member_location(1920) , member base_type u64 byte_size(8) encoding(7) accessibility(1) data_member_location(1928) , member s#'bindings::bindings_raw::__IncompleteArrayField<*mut bindings::bindings_raw::cgroup>' ancestors accessibility(1) data_member_location(1936) } byte_size(1984) alignment(64)\0";

/// The top level entrypoint to implementing a kernel module.
///
/// For any teardown or cleanup operations, your type may implement [`Drop`].
pub trait Module: Sized + Sync + Send {
    /// Called at module initialization time.
    ///
    /// Use this method to perform whatever setup or registration your module
    /// should do.
    ///
    /// Equivalent to the `module_init` macro in the C API.
    fn init(module: &'static ThisModule) -> error::Result<Self>;
}

/// Equivalent to `THIS_MODULE` in the C API.
///
/// C header: [`include/linux/init.h`](srctree/include/linux/init.h)
pub struct ThisModule(*mut bindings::module);

// SAFETY: `THIS_MODULE` may be used from all threads within a module.
unsafe impl Sync for ThisModule {}

impl ThisModule {
    /// Creates a [`ThisModule`] given the `THIS_MODULE` pointer.
    ///
    /// # Safety
    ///
    /// The pointer must be equal to the right `THIS_MODULE`.
    pub const unsafe fn from_ptr(ptr: *mut bindings::module) -> ThisModule {
        ThisModule(ptr)
    }

    /// Access the raw pointer for this module.
    ///
    /// It is up to the user to use it correctly.
    pub const fn as_ptr(&self) -> *mut bindings::module {
        self.0
    }
}

#[cfg(not(any(testlib, test)))]
#[panic_handler]
fn panic(info: &core::panic::PanicInfo<'_>) -> ! {
    pr_emerg!("{}\n", info);
    // SAFETY: FFI call.
    unsafe { bindings::BUG() };
}

/// Produces a pointer to an object from a pointer to one of its fields.
///
/// # Safety
///
/// The pointer passed to this macro, and the pointer returned by this macro, must both be in
/// bounds of the same allocation.
///
/// # Examples
///
/// ```
/// # use kernel::container_of;
/// struct Test {
///     a: u64,
///     b: u32,
/// }
///
/// let test = Test { a: 10, b: 20 };
/// let b_ptr = &test.b;
/// // SAFETY: The pointer points at the `b` field of a `Test`, so the resulting pointer will be
/// // in-bounds of the same allocation as `b_ptr`.
/// let test_alias = unsafe { container_of!(b_ptr, Test, b) };
/// assert!(core::ptr::eq(&test, test_alias));
/// ```
#[macro_export]
macro_rules! container_of {
    ($ptr:expr, $type:ty, $($f:tt)*) => {{
        let ptr = $ptr as *const _ as *const u8;
        let offset: usize = ::core::mem::offset_of!($type, $($f)*);
        ptr.sub(offset) as *const $type
    }}
}

/// Helper for `.rs.S` files.
#[doc(hidden)]
#[macro_export]
macro_rules! concat_literals {
    ($( $asm:literal )* ) => {
        ::core::concat!($($asm),*)
    };
}

/// Wrapper around `asm!` configured for use in the kernel.
///
/// Uses a semicolon to avoid parsing ambiguities, even though this does not match native `asm!`
/// syntax.
// For x86, `asm!` uses intel syntax by default, but we want to use at&t syntax in the kernel.
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[macro_export]
macro_rules! asm {
    ($($asm:expr),* ; $($rest:tt)*) => {
        ::core::arch::asm!( $($asm)*, options(att_syntax), $($rest)* )
    };
}

/// Wrapper around `asm!` configured for use in the kernel.
///
/// Uses a semicolon to avoid parsing ambiguities, even though this does not match native `asm!`
/// syntax.
// For non-x86 arches we just pass through to `asm!`.
#[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
#[macro_export]
macro_rules! asm {
    ($($asm:expr),* ; $($rest:tt)*) => {
        ::core::arch::asm!( $($asm)*, $($rest)* )
    };
}
