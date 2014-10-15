#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <fcntl.h>
using namespace std;

const int bufsize = 1024;
int main(int argc,char *argv[])
{
	int memory = 0;
	pid_t pid = 1543;
	char fn[bufsize],buf[bufsize];
	sprintf(fn,"/proc/%d/status",pid);
	FILE *fd = fopen(fn,"r");
	while(fgets(buf,bufsize-1,fd)){
		if(strncmp(buf,"VmPeak:",7) == 0){
			sscanf(buf+8,"%d",&memory);
			break;
		}
	}
	printf("%d kb\n",memory);
	return 0;
}
