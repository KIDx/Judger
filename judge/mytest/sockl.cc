#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
using namespace std;

int main(int argc,char *argv[])
{
	int sfd;
	struct sockaddr_un addr;
	sfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(sfd == -1)
	{
		perror("socket error");
		exit(EXIT_FAILURE);
	}
	
	close(sfd);
	return 0;
}
