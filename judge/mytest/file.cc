#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <pwd.h>
using namespace std;

int main(int argc,char *argv[])
{
	pid_t pid = fork();
	if(pid < 0)
	{
		exit(1);
	}
	if(pid == 0){
		if(ptrace(PTRACE_TRACEME,0,NULL,NULL) < 0)
		{
			printf("chirld ptrace error\n");
			exit(1);
		}
		sleep(1);
		system("ls -l");
	}else{
		int status = 0;
		struct rusage rused;
		struct user_regs_struct regs;
		for(;;){
		if(wait4(pid,&status,0,&rused) < 0){
			printf("get child resource error\n");
			exit(1);
		}
		if(WIFEXITED(status))
		{
			printf("结束退出\n");
			break;
		}
		if(ptrace(PTRACE_GETREGS,pid,NULL,&regs) < 0)
		{
			printf("father ptrace getregs error\n");
			exit(1);
		}
		int syscall_id;
#ifdef __i386__
		syscall_id = regs.orig_eax;
#else
		syscall_id = regs.orig_rax;
#endif

		if(syscall_id > 0)
		{
			printf("syscall_id: %d\n",syscall_id);
		}
		if(ptrace(PTRACE_SYSCALL,pid,NULL,NULL) < 0)
		{
			printf("father ptrace syscall error\n");
			exit(1);
		}
		}
	}
	return 0;
}
