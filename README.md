1、开发环境：
	Ubuntu 10.4 、 g++ 、python，vim

2、参考资料：
	hust oj    whu oj

3、设计模块：
	分为前端daemon 和 后台judge
	设计思想，将需求和评测分开，judge就只是评测代码和返回结果，daemon根据需求添加功能。

4、功能：
	可以编译运行，c、c++、java程序，同时支持普通评测、spj 和 TC （就写个类，头文件）三种模式（可以混合）

文件功能：
python daemon主体在
	daemon.py

评测主体在Judge目录下：
	test.cpp 是OJ的主体代码
	okcall.h 是调用限制定义
	logger.h 是日志部分
	judge.h 是OJ数据部分的定义
	language.h 是编程语言支持
	/data 文件夹是数据存储部分
	/temp 是临时文件部分

关于daemon：
daemon已从后台运行改为了前台运行，以便使用pm2进行管理。如果不想使用pm2,可以换用unusedfiles中的daemon-back.py