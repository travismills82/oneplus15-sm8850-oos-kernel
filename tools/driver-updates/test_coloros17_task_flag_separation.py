#!/usr/bin/env python3
"""Extracted real helpers; host model does not replace ARM64/device gates."""
import re
import unittest

from test_coloros17_unified_kernel_backports import ROOT, function, host_c


class TaskFlagSeparation(unittest.TestCase):
    def source(self):
        sched = (ROOT / 'kernel_platform/common/include/linux/sched.h').read_text()
        bit = re.search(r'^#define PFA_NO_NOTIFY_SIGNAL\s+(\d+)', sched, re.M).group(1)
        prefix = r'''
#include <assert.h>
#include <stdbool.h>
#define unlikely(x) (x)
#define likely(x) (x)
#define TIF_NOTIFY_SIGNAL 0
#define TIF_SIGPENDING 1
#define PF_MEMALLOC_BOOST 0x00800000U
struct task_struct { unsigned flags; unsigned long atomic_flags; unsigned thread_flags; };
static struct task_struct task;
#define current (&task)
#define PFA_NO_NOTIFY_SIGNAL BIT_INDEX
static int test_and_set_bit(unsigned n, unsigned long *p) {
 return !!(__atomic_fetch_or(p, 1UL << n, __ATOMIC_SEQ_CST) & (1UL << n));
}
static void set_bit(unsigned n, unsigned long *p) {
 __atomic_fetch_or(p, 1UL << n, __ATOMIC_SEQ_CST);
}
static void clear_bit(unsigned n, unsigned long *p) {
 __atomic_fetch_and(p, ~(1UL << n), __ATOMIC_SEQ_CST);
}
static bool task_no_notify_signal(struct task_struct *p) {
 return !!(__atomic_load_n(&p->atomic_flags, __ATOMIC_RELAXED) & (1UL << PFA_NO_NOTIFY_SIGNAL));
}
static int test_tsk_thread_flag(struct task_struct *p, unsigned n) {
 return !!(p->thread_flags & (1U << n));
}
static int task_sigpending(struct task_struct *p) {
 return test_tsk_thread_flag(p, TIF_SIGPENDING);
}
'''.replace('BIT_INDEX', bit)
        return prefix + '\n'.join(function('include/linux/sched/signal.h', signature)
                                  for signature in (
                                      'static inline unsigned int no_notify_signal_save(void)',
                                      'static inline void no_notify_signal_restore(unsigned int flags)',
                                      'static inline int signal_pending(struct task_struct *p)'))

    def run_c(self, main):
        result = host_c(self.source() + '\nint main(void) {\nassert(!signal_pending(current));\n'
                        + main + '\nreturn 0; }\n')
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_nested_sections_and_both_preexisting_states(self):
        self.run_c(r'''
for (unsigned initial=0; initial<2; ++initial) {
 task.atomic_flags = 0xa5UL | (initial << PFA_NO_NOTIFY_SIGNAL);
 unsigned long before = task.atomic_flags;
 unsigned a=no_notify_signal_save(), b=no_notify_signal_save();
 assert(a==initial && b==1 && task_no_notify_signal(current));
 no_notify_signal_restore(b);
 assert(task_no_notify_signal(current));
 no_notify_signal_restore(a);
 assert(task.atomic_flags==before);
}
''')

    def test_oplus_boost_and_notify_state_are_independent(self):
        self.run_c(r'''
task.thread_flags = 1U << TIF_NOTIFY_SIGNAL;
assert(signal_pending(current));
task.flags |= PF_MEMALLOC_BOOST;
assert(signal_pending(current));
unsigned token=no_notify_signal_save();
assert(!signal_pending(current) && (task.flags & PF_MEMALLOC_BOOST));
task.flags &= ~PF_MEMALLOC_BOOST;
assert(!signal_pending(current));
task.flags |= PF_MEMALLOC_BOOST;
no_notify_signal_restore(token);
assert(signal_pending(current) && (task.flags & PF_MEMALLOC_BOOST));
''')

    def test_real_signals_are_never_suppressed(self):
        self.run_c(r'''
for (unsigned notify=0; notify<2; ++notify) {
 task.thread_flags = (1U << TIF_SIGPENDING) | (notify << TIF_NOTIFY_SIGNAL);
 unsigned token=no_notify_signal_save();
 assert(signal_pending(current));
 no_notify_signal_restore(token);
 assert(signal_pending(current));
}
''')

    def test_preserves_other_atomic_and_task_flags(self):
        self.run_c(r'''
task.flags = 0xfedcba98U;
task.atomic_flags = 0xffUL;
unsigned token=no_notify_signal_save();
set_bit(10, &task.atomic_flags);
clear_bit(2, &task.atomic_flags);
no_notify_signal_restore(token);
assert(task.flags==0xfedcba98U);
assert(task.atomic_flags==((0xffUL | (1UL << 10)) & ~(1UL << 2)));
''')

    def test_only_available_core_atomic_bit_is_allocated(self):
        text = (ROOT / 'kernel_platform/common/include/linux/sched.h').read_text()
        bits = re.findall(r'^#define (PFA_\w+)\s+(\d+)\b', text, re.M)
        self.assertEqual(len(bits), len({value for _, value in bits}))
        self.assertIn(('PFA_NO_NOTIFY_SIGNAL', '8'), bits)
        self.assertNotRegex(text, r'^#define PF_NO_NOTIFY_SIGNAL\b')
        self.assertIn('#define PF__HOLE__00800000\t0x00800000', text)
        pidns = (ROOT / 'kernel_platform/common/kernel/pid_namespace.c').read_text()
        self.assertIn('guard(no_notify_signal)();', pidns)

    def test_old_collision_negative_control(self):
        source = self.source().replace('likely(!task_no_notify_signal(p))',
                                       'likely(!(p->flags & PF_MEMALLOC_BOOST))')
        # Keep the unrelated helper referenced under the strict host compiler.
        result = host_c(source + r'''
int main(void) {
 task.flags = PF_MEMALLOC_BOOST;
 task.thread_flags = 1U << TIF_NOTIFY_SIGNAL;
 unsigned token = no_notify_signal_save();
 assert(task_no_notify_signal(current));
 no_notify_signal_restore(token);
 assert(signal_pending(current));
 return 0;
}
''')
        self.assertNotEqual(result.returncode, 0)

    def test_atomic_updates_do_not_lose_unrelated_bits(self):
        result = host_c(self.source() + r'''
#include <pthread.h>
static void *writer(void *unused) {
 for (unsigned i=0; i<20000; ++i) {
  set_bit(10, &task.atomic_flags);
  clear_bit(10, &task.atomic_flags);
 }
 set_bit(10, &task.atomic_flags);
 return 0;
}
int main(void) {
 pthread_t thread;
 assert(!signal_pending(current));
 assert(!pthread_create(&thread, 0, writer, 0));
 for (unsigned i=0; i<20000; ++i) {
  unsigned token=no_notify_signal_save();
  assert(task_no_notify_signal(current));
  no_notify_signal_restore(token);
 }
 assert(!pthread_join(thread, 0));
 assert(task.atomic_flags == (1UL << 10));
 return 0;
}
''')
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
