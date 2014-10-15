#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <syscall.h>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

int int_ignored;  // warn_if_unused 
int main()
{
	pid_t compiler = fork();
	const char * CP_C[] = { "gcc", "Main.c", "-o", "Main", "-O2","-Wall",
					 "-std=c99", "-DONLINE_JUDGE", NULL };
	if(compiler < 0)
	{
		printf("error");
		exit(1);
	}
	if(compiler == 0)
	{
		freopen("ce.txt","w",stderr);
		execvp(CP_C[0],(char * const *) CP_C);
	}else{
		int status = 0;
		waitpid(compiler,&status,0);
		printf("%d ",status);
		system("cat ce.txt");
	}
	return 0;
}
