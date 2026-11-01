#ifndef __KSU_H_KSUD
#define __KSU_H_KSUD

#include <asm/syscall.h>

#define KSUD_PATH "/data/adb/ksud"

void ksu_ksud_init();
void ksu_ksud_exit();

#define MAX_ARG_STRINGS 0x7FFFFFFF
struct user_arg_ptr {
#ifdef CONFIG_COMPAT
    bool is_compat;
#endif
    union {
        const char __user *const __user *native;
#ifdef CONFIG_COMPAT
        const compat_uptr_t __user *compat;
#endif
    } ptr;
};

static inline const char __user *get_user_arg_ptr(struct user_arg_ptr argv, int nr)
{
    const char __user *native;

#ifdef CONFIG_COMPAT
    if (unlikely(argv.is_compat)) {
        compat_uptr_t compat;

        if (get_user(compat, argv.ptr.compat + nr))
            return ERR_PTR(-EFAULT);

        return compat_ptr(compat);
    }
#endif

    if (get_user(native, argv.ptr.native + nr))
        return ERR_PTR(-EFAULT);

    return native;
}

int ksu_handle_execveat_ksud(int *fd, struct filename **filename_ptr,
                             struct user_arg_ptr *argv,
                             struct user_arg_ptr *envp, int *flags);
#endif
