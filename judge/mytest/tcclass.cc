#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
using namespace std;

class myclass{
	int a,b;
	
public:
	int scan(){
		if(scanf("%d%d",&a,&b) == EOF)
			return -1;
		return 0;
	}
	int add(){
		return a+b;
	}
};
int main()
{
	myclass mc;
	mc.scan();
	printf("%d\n",mc.add());
	return 0;
}
