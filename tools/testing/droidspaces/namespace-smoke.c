/* SPDX-License-Identifier: GPL-2.0-only */
/* Disposable VM functional tests, never a phone installation or host daemon. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/ipc.h>
#include <linux/filter.h>
#include <linux/mqueue.h>
#include <linux/seccomp.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/sem.h>
#include <linux/shm.h>
#include <sched.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

static void require(int ok, const char *what)
{
    if (!ok) { fprintf(stderr, "SMOKE_FAIL %s errno=%d\n", what, errno); exit(1); }
}

static void child_ok(pid_t child)
{
    int status;
    require(child > 0, "fork");
    require(waitpid(child, &status, 0) == child, "waitpid");
    require(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child result");
}

static void write_text(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);
    require(fd >= 0, path);
    require(write(fd, value, strlen(value)) == (ssize_t)strlen(value), "controller write");
    close(fd);
}

static void namespace_child(void)
{
    require(unshare(CLONE_NEWNS | CLONE_NEWIPC | CLONE_NEWUTS | CLONE_NEWNET |
                    CLONE_NEWCGROUP | CLONE_NEWPID) == 0, "namespace unshare");
    require(mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == 0, "private mounts");
    require(sethostname("droidspaces-vm", 14) == 0, "private hostname");
    require(mount("tmpfs", "/mnt/nsprobe", "tmpfs", 0, NULL) == 0, "isolated tmpfs");
    int marker = open("/mnt/nsprobe/child", O_CREAT | O_WRONLY, 0600);
    require(marker >= 0, "private mount marker");
    close(marker);
    pid_t init = fork();
    if (init == 0) {
        require(getpid() == 1, "PID namespace init");
        exit(0);
    }
    child_ok(init);
    exit(0);
}

int main(void)
{
    struct utsname before, after;
    require(mkdir("/mnt/nsprobe", 0755) == 0, "mount test directory");
    require(uname(&before) == 0, "uname before");
    pid_t child = fork();
    if (child == 0) namespace_child();
    child_ok(child);
    require(uname(&after) == 0 && strcmp(before.nodename, after.nodename) == 0,
            "host hostname unchanged");
    require(access("/mnt/nsprobe/child", F_OK) != 0 && errno == ENOENT,
            "child mount is invisible in parent");
    puts("PID_MNT_IPC_UTS_NET_CGROUP_ISOLATION=PASS");

    child = fork();
    if (child == 0) {
        require(unshare(CLONE_NEWUSER) == 0, "user namespace");
        write_text("/proc/self/uid_map", "0 0 1\n");
        write_text("/proc/self/setgroups", "deny\n");
        write_text("/proc/self/gid_map", "0 0 1\n");
        require(getuid() == 0 && getgid() == 0, "user namespace ID mappings");
        errno = 0;
        require(sethostname("must-not-change-host", 20) == -1 && errno == EPERM,
                "userns capabilities do not authorize parent UTS changes");
        exit(0);
    }
    child_ok(child);
    puts("USER_NAMESPACE=PASS");

    child = fork();
    if (child == 0) {
        struct sock_filter filter[] = {
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_getppid, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EACCES),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
        };
        struct sock_fprog program = { .len = sizeof(filter) / sizeof(filter[0]), .filter = filter };
        require(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "no_new_privs");
        require(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0, "seccomp install");
        errno = 0;
        require(syscall(SYS_getppid) == -1 && errno == EACCES, "seccomp filter applies");
        exit(0);
    }
    child_ok(child);
    puts("SECCOMP_FILTER_OPERATION=PASS");

    struct mq_attr attr = { .mq_maxmsg = 4, .mq_msgsize = 64 };
    int mq = syscall(SYS_mq_open, "ds-vm", O_CREAT | O_RDWR | O_CLOEXEC, 0600, &attr);
    require(mq >= 0, "mq_open");
    require(syscall(SYS_mq_timedsend, mq, "hello", 5, 7, NULL) == 0, "mq send");
    char message[64];
    unsigned priority;
    require(syscall(SYS_mq_timedreceive, mq, message, sizeof(message), &priority, NULL) == 5,
            "mq receive");
    require(priority == 7 && memcmp(message, "hello", 5) == 0, "mq data");
    child = fork();
    if (child == 0) {
        require(unshare(CLONE_NEWIPC) == 0, "isolated mqueue namespace");
        errno = 0;
        require(syscall(SYS_mq_open, "ds-vm", O_RDONLY, 0, NULL) == -1 && errno == ENOENT,
                "parent mqueue is not visible in child IPC namespace");
        exit(0);
    }
    child_ok(child);
    close(mq);
    require(syscall(SYS_mq_unlink, "ds-vm") == 0, "mq remove");
    puts("POSIX_MQUEUE_SEND_RECEIVE=PASS");

    write_text("/sys/fs/cgroup/cgroup.subtree_control", "+cpu +memory");
    require(mkdir("/sys/fs/cgroup/ds-smoke", 0755) == 0, "create cgroup");
    write_text("/sys/fs/cgroup/ds-smoke/memory.max", "67108864");
    write_text("/sys/fs/cgroup/ds-smoke/cpu.max", "50000 100000");
    require(rmdir("/sys/fs/cgroup/ds-smoke") == 0, "remove cgroup");
    puts("CGROUP2_CPU_MEMORY_CONTROLS=PASS");

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    require(sock >= 0, "netfilter socket");
    struct xt_get_revision revision = { .name = "addrtype", .revision = 1 };
    socklen_t length = sizeof(revision);
    require(getsockopt(sock, IPPROTO_IP, IPT_SO_GET_REVISION_MATCH, &revision, &length) == 0,
            "addrtype match revision 1");
    struct ipt_getinfo nat = { .name = "nat" };
    length = sizeof(nat);
    require(getsockopt(sock, IPPROTO_IP, IPT_SO_GET_INFO, &nat, &length) == 0, "IPv4 NAT table");
    close(sock);
    puts("IPV4_NAT_ADDRTYPE=PASS");

    int shm = syscall(SYS_shmget, IPC_PRIVATE, 4096, IPC_CREAT | 0600);
    require(shm >= 0, "shmget");
    int *memory = (void *)syscall(SYS_shmat, shm, NULL, 0);
    require(memory != (void *)-1, "shmat");
    *memory = 0x12345678;
    child = fork();
    if (child == 0) {
        require(*memory == 0x12345678, "inherited shared memory");
        *memory = 0x76543210;
        require(syscall(SYS_shmdt, memory) == 0, "child shmdt");
        exit(0);
    }
    child_ok(child);
    require(*memory == 0x76543210, "shared-memory result");
    child = fork();
    if (child == 0) {
        require(unshare(CLONE_NEWIPC) == 0, "isolated shared-memory namespace");
        errno = 0;
        require(syscall(SYS_shmat, shm, NULL, 0) == -1 && errno == EINVAL,
                "parent shmid is not valid in empty child IPC namespace");
        int private_id = syscall(SYS_shmget, IPC_PRIVATE, 4096, IPC_CREAT | 0600);
        require(private_id >= 0, "child namespace shmget");
        int *private_memory = (void *)syscall(SYS_shmat, private_id, NULL, 0);
        require(private_memory != (void *)-1, "child namespace shmat");
        *private_memory = 0;
        require(syscall(SYS_shmdt, private_memory) == 0, "child namespace shmdt");
        require(syscall(SYS_shmctl, private_id, IPC_RMID, NULL) == 0, "child namespace shm remove");
        exit(0);
    }
    child_ok(child);
    require(*memory == 0x76543210, "private namespace did not change parent shared memory");
    puts("SYSV_POSIX_IPC_NAMESPACE_ISOLATION=PASS");
    require(syscall(SYS_shmdt, memory) == 0, "parent shmdt");
    require(syscall(SYS_shmctl, shm, IPC_RMID, NULL) == 0, "shm remove");
    puts("SYSV_SHM_FORK_CLEANUP=PASS");

    int sem = syscall(SYS_semget, IPC_PRIVATE, 1, IPC_CREAT | 0600);
    require(sem >= 0, "semget");
    require(syscall(SYS_semctl, sem, 0, SETVAL, 1) == 0, "sem init");
    for (int i = 0; i < 100; ++i) {
        child = fork();
        if (child == 0) {
            struct sembuf op = { .sem_num = 0, .sem_op = -1, .sem_flg = SEM_UNDO };
            require(syscall(SYS_semop, sem, &op, 1) == 0, "semop undo");
            exit(0);
        }
        child_ok(child);
        require(syscall(SYS_semctl, sem, 0, GETVAL, 0) == 1, "exit restores SEM_UNDO");
    }
    require(syscall(SYS_semctl, sem, 0, IPC_RMID, 0) == 0, "sem remove");
    puts("SYSV_SEM_UNDO_100_FORKS=PASS");
    return 0;
}
