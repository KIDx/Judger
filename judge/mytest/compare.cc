#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;

#define OJ_AC 1
#define OJ_PE 2
#define OJ_WA 3
#define OJ_RE 4

/* 
 * in windows '\r\n' linux '\n'
 * here compare to ignore this differece
 */
void compare_until_nonspace(int &c_std,int &c_usr,
		FILE *&fd_std,FILE *&fd_usr,int &ret)
{
	bool lines = true;
	while((isspace(c_std)) || (isspace(c_usr)))
	{
		if(c_std != c_usr){
			if(c_std == '\r' && c_usr == '\n' && lines)
			{
				lines = false;
				c_std = fgetc(fd_std);
				continue;
			}
			else{
				ret = OJ_PE;
			}
		}
		if(isspace(c_std))
			c_std = fgetc(fd_std);
		if(c_usr == '\n') lines = true;
		if(isspace(c_usr))
			c_usr = fgetc(fd_usr);
	}
}


int compare_output(char *file_std,char *file_usr)
{
	int ret = OJ_AC;
	int c_std,c_usr;
	FILE *fd_std = fopen(file_std,"r");
	FILE *fd_usr = fopen(file_usr,"r");
	if((!fd_std) || (!fd_usr)){
		ret = OJ_RE;
	}else
	{
		c_std = fgetc(fd_std);
		c_usr = fgetc(fd_usr);

		for(;;){
			compare_until_nonspace(c_std,c_usr,fd_std,fd_usr,ret);
			while((!isspace(c_std)) && (!isspace(c_usr))){
				if(c_std == EOF && c_usr == EOF)
					goto end;
				if(c_std == EOF || c_usr == EOF){
					ret = OJ_WA;
					goto end;
				}
				if(c_std != c_usr){
					ret = OJ_WA;
					goto end;
				}
				c_std = fgetc(fd_std);
				c_usr = fgetc(fd_usr);
			}
		}
	}
end: if(fd_std)
		fclose(fd_std);
	if(fd_usr)
		fclose(fd_usr);
	return ret;
}
int main(int argc,char *argv[])
{
	char file_std[] = "flipstd.txt";
	char file_usr[] = "flipdd.txt";
	printf("%d\n",compare_output(file_std,file_usr));
	printf("%d\n",compare_output(file_usr,file_std));
	return 0;
}

