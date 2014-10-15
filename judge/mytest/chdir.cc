#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/reg.h>
#include <dirent.h>
#include <fcntl.h>

using namespace std;

const int maxn = 1024;
int main(int argc,char *argv[])
{
	struct passwd *judge = getpwnam("ttlast");
	printf("%d \n",judge->pw_uid);
	if(judge == NULL)
	{
		puts("get passwd error");
		exit(1);
	}
	DIR *dp;
	struct dirent *dirp;
	char cwd[maxn];
	if(EXIT_SUCCESS != chdir("temp"))
	{
		puts("chdir error");
		exit(1);
	}
	system("./Main");
	system("ls");
	char *tmp = getcwd(cwd,maxn);
#ifdef JUDGE_DEBUG
	if(EXIT_SUCCESS != chroot(cwd)){
		puts("chroot  error");
	}
	if(EXIT_SUCCESS != setuid(judge->pw_uid))
	{
		puts("setuid error");
	}
#endif

	puts("after ls");
	FILE *fd = fopen("dd.txt","w");
	if(fd){
	fprintf(fd,"ok\n");
	fclose(fd);
	puts("file");
	}
	fd = fopen("Main","r");
	if(fd){
		fclose(fd);
		puts("open file ok");
	}
	system("./Main");
	execlp("./Main","Main",NULL);
	puts("execlp error");
	return 0;
}
