#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace std;

int main(int argc,char *argv[])
{
	string file_std = "lines8.in";
	string file_usr = "lines8.out";
	FILE *fd_std = fopen(file_std.c_str(),"r");
	FILE *fd_usr = fopen(file_usr.c_str(),"r");
	if(!fd_std) printf("std cann't open");
	if(!fd_usr) printf("usr cann't open");
	return 0;
}
