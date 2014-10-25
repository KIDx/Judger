/*
 * syscall 调用的可用次数
 *  0 -> 禁止调用
 *  1 -> 可以调用次数
 * -1 -> 不限制调用次数
 */

#ifndef __OKCALL__
#define __OKCALL__

#include <sys/syscall.h>
#include <cstdio>
#include "judge.h"

short ok_table[1024] = {0};

struct SysCallLimit {
    short CallId, Limit;
    SysCallLimit(int id, int lim) : CallId(id), Limit(lim) {}
};
#define Allow(CALL_ID) SysCallLimit((CALL_ID),
#define Calls(CALL_LIM) (CALL_LIM))
#define EndLimitTable SysCallLimit(-1, -1)
#define INF -1

SysCallLimit CLimit[] = {
    Allow (SYS_access) Calls (1),
    Allow (SYS_arch_prctl) Calls (1),
    Allow (SYS_brk) Calls (INF),
    Allow (SYS_clock_gettime) Calls (INF),
    Allow (SYS_close) Calls (1),
    Allow (SYS_execve) Calls (1),
    Allow (SYS_exit_group) Calls (1),
    Allow (SYS_fstat) Calls (3),
    Allow (SYS_get_thread_area) Calls (1),
    Allow (SYS_gettid) Calls (1),
    Allow (SYS_mmap) Calls (INF),
    Allow (SYS_mprotect) Calls (16),
    Allow (SYS_munmap) Calls (INF),
    Allow (SYS_open) Calls (1),
    Allow (SYS_read) Calls (INF),
    Allow (SYS_readlink) Calls (1),
    Allow (SYS_rt_sigprocmask) Calls (1),
    Allow (SYS_set_thread_area) Calls (1),
    Allow (SYS_tgkill) Calls (2),
    Allow (SYS_time) Calls (INF),
    Allow (SYS_uname) Calls (1),
    Allow (SYS_write) Calls (INF),
    Allow (SYS_writev) Calls (INF),
    EndLimitTable
};

SysCallLimit CppLimit[] = {
    Allow (SYS_access) Calls (1),
    Allow (SYS_arch_prctl) Calls (1),
    Allow (SYS_brk) Calls (INF),
    Allow (SYS_clock_gettime) Calls (INF),
    Allow (SYS_close) Calls (1),
    Allow (SYS_execve) Calls (1),
    Allow (SYS_exit_group) Calls (1),
    Allow (SYS_futex) Calls (INF),
    Allow (SYS_fstat) Calls (3),
    Allow (SYS_get_thread_area) Calls (1),
    Allow (SYS_gettid) Calls (1),
    Allow (SYS_mmap) Calls (INF),
    Allow (SYS_mprotect) Calls (16),
    Allow (SYS_munmap) Calls (INF),
    Allow (SYS_open) Calls (2),
    Allow (SYS_read) Calls (INF),
    Allow (SYS_readlink) Calls (1),
    Allow (SYS_rt_sigprocmask) Calls (1),
    Allow (SYS_set_thread_area) Calls (1),
    Allow (SYS_set_tid_address) Calls (1),
    Allow (SYS_tgkill) Calls (2),
    Allow (SYS_time) Calls (INF),
    Allow (SYS_uname) Calls (1),
    Allow (SYS_write) Calls (INF),
    Allow (SYS_writev) Calls (INF),
    EndLimitTable
};

SysCallLimit JavaLimit[] = {
    Allow (SYS_access) Calls (16),
    Allow (SYS_arch_prctl) Calls (1),
    Allow (SYS_brk) Calls (4),
    Allow (SYS_clock_gettime) Calls (INF),
    Allow (SYS_clone) Calls (1),
    Allow (SYS_close) Calls (INF),
    Allow (SYS_execve) Calls (1),
    Allow (SYS_exit_group) Calls (1),
    Allow (SYS_fstat) Calls (16),
    Allow (SYS_futex) Calls (INF),
    Allow (SYS_get_thread_area) Calls (1),
    Allow (SYS_getdents64) Calls (2),
    Allow (SYS_getrlimit) Calls (1),
    Allow (SYS_mmap) Calls (INF),
    Allow (SYS_mprotect) Calls (INF),
    Allow (SYS_munmap) Calls (INF),
    Allow (SYS_open) Calls (INF),
    Allow (SYS_openat) Calls (1),
    Allow (SYS_read) Calls (INF),
    Allow (SYS_readlink) Calls (2),
    Allow (SYS_rt_sigaction) Calls (2),
    Allow (SYS_rt_sigprocmask) Calls (1),
    Allow (SYS_set_robust_list) Calls (1),
    Allow (SYS_set_thread_area) Calls (1),
    Allow (SYS_set_tid_address) Calls (1),
    Allow (SYS_stat) Calls (16),
    Allow (SYS_time) Calls (INF),
    Allow (SYS_uname) Calls (1),
    Allow (SYS_write) Calls (INF),
    EndLimitTable
};

SysCallLimit* TableFor[] = {
    NULL, CLimit, CppLimit, JavaLimit,
};

//初始化，系统调用限制表格
//
void init_ok_table(int lang)
{
    int scall, stimes;
    SysCallLimit* table = TableFor[lang];
    memset(ok_table, 0, sizeof(ok_table));
    for (int i = 0; table[i].CallId != -1; ++i)
    {
        scall = table[i].CallId;
        stimes = table[i].Limit;
        ok_table[scall] = stimes;
    }
    //若比赛时出现不应出现的DC，可以临时配置进okcall.cfg中，赛后再修改程序重新编译。
    FILE* cfg = fopen("okcall.cfg", "r");
    if (!cfg) return;
    while (~fscanf(cfg, "%d := %d", &scall, &stimes))
    {
        ok_table[scall] = stimes;
    }
    fclose(cfg);
}

static bool in_syscall = true;

bool is_valid_syscall(int lang, int syscall_id, pid_t child, user_regs_struct regs)
{
    in_syscall = !in_syscall;

    //被限制调用的函数，syscall_id = 0
    if (ok_table[syscall_id] == 0)
    {
        long addr;
        if (syscall_id == SYS_open)
        {
            addr = regs.rbx;
            union u{unsigned long val;char chars[sizeof(long)];} data;
            unsigned long i = 0, j = 0, k = 0;
            char filename[1024];
            while (true)
            {
                data.val = ptrace(PTRACE_PEEKDATA, child, addr+i, NULL);
                i += sizeof(long);
                for(j = 0; j < sizeof(long) && data.chars[j] > 0 && k < 256; ++j)
                {
                    filename[k++] = data.chars[j];
                }
                if (j < sizeof(long) && data.chars[j] == 0)
                    break;
            }
            filename[k] = 0;
            if (strstr(filename, "/proc/") == filename)
                return true;
            LOG_NOTICE("syscall_id %d syscall open: %s", syscall_id, filename);
        }
        return false;
    }
    else if (ok_table[syscall_id] > 0)
    {
        if (!in_syscall)
            --ok_table[syscall_id];
    }
    return true;
}
#endif
