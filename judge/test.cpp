/*
 * A simple judge
 * Author: TT_last
 *
 * use:
 * ./judge -l 语言 -D 数据目录 -d 临时目录 -t 时间限制 -m 内存限制 -o 输出限制 [-S dd] [-T]
 * -l 语言 C=1, C++=2, JAVA=3, C++11=4
 * -D 数据目录 包括题号的输入输出文件的目录
 * -d 临时目录 judge可以用来存放编译后的文件及其他临时文件的
 * -t 时间限制 允许程序运行的最长时间, 以毫秒为单位, 默认为1000, 1s
 * -m 内存限制 允许程序使用的最大内存量, 以KB为单位, 默认为65536, 64MB
 * -o 输出限制 允许程序输出的最大数据量, 以KB为单位, 默认为8192, 8MB
 * -S dd 可选, 如指定此参数, 则表示这是一个Special Judge的题目
 * -T 可选，如指定此参数，则表示这个topcoder模式题目，只要写相关头文件 和 相关类+接口就可以了。
 *
 * 其中-S dd和 -T可以混合用.
 *
 * 用-S dd，data需要有spj.exe spj.cpp
 * 用-T 则相应的需要tc.c tc.cpp tc.java之类的配套
 * for example:
 * ./judge -l 2 -D data -d temp -t 1000 -m 65536 -o 8192
 *
 * */
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/time.h>
#include "judge.h"
#include "okcall.h"

#define Max(x, y) (x) > (y) ? (x) : (y)
using namespace std;
//#define JUDGE_DEBUG
extern int errno;
const int MAXN = 8192;

//重构：令语言支持独立于评测逻辑 By Sine 2013_12_01
//In file "language.h"

void output_result(int result, int memory_usage = 0, int time_usage = 0)
{
    //OJ_SE发生时，传入本函数的time_usage即是EXIT错误号，取负数是为了强调和提醒
    //此时若memory_usage < 0则为errno，说明错误发生的原因，取负数是为了强调和提醒
    //在前台看到System Error时，Time一栏的数值取绝对值就是EXIT错误号，Memory一栏取绝对值则为errno
    //OJ_RF发生时，传入本函数的time_usage即是syscall号，取负数是为了强调和提醒
    //在前台看到Dangerous Code时，Time一栏的数值取绝对值就是Syscall号
    //Bugfix：之前版本评测过程中每处错误发生时一般会直接exit，导致前台status一直Running
    //本次fix在所有SE导致的exit前都添加了对本函数的调用，并给出EXIT错误号，解决了此问题，更方便了SE发生时对系统进行调试
    //fixed By Sine 2014-03-05
    if (result == judge_conf::OJ_SE || result == judge_conf::OJ_RF) time_usage *= -1;
    //#ifdef JUDGE_DEBUG
    LOG_DEBUG("result: %d, %dKB %dms", result, memory_usage, time_usage);
    //#endif
    printf("%d %d %d\n", result, memory_usage, time_usage);
}

void timeout(int signo)
{
    output_result(judge_conf::OJ_SE, 0, judge_conf::EXIT_TIMEOUT);
    if (signo == SIGALRM)
        exit(judge_conf::EXIT_TIMEOUT);
}

//normal compare file
void compare_until_nonspace(int &c_std, int &c_usr, FILE *&fd_std, FILE *&fd_usr, int &ret)
{
    while((isspace(c_std)) || (isspace(c_usr)))
    {
        if (c_std != c_usr)
        {
            if (c_std == '\r' && c_usr == '\n')
            {
                c_std = fgetc(fd_std);
                if (c_std != c_usr)
                    ret = judge_conf::OJ_PE;
            }
            else
            {
                ret = judge_conf::OJ_PE;
            }
        }
        if (isspace(c_std))
            c_std = fgetc(fd_std);
        if (isspace(c_usr))
            c_usr = fgetc(fd_usr);
    }
}

//合并文件
void addfile(string &main_file, string &tc_file)
{
    LOG_DEBUG("TC mode");
    char cc[MAXN+5];
    FILE *sc_fd = fopen(main_file.c_str(), "a+");
    FILE *tc_fd = fopen(tc_file.c_str(), "r");
    if (sc_fd && tc_fd)
    {
        fputs("\n", sc_fd);
        while(fgets(cc, MAXN, tc_fd))
        {
            fputs(cc, sc_fd);
        }
    }
    if (sc_fd) fclose(sc_fd);
    if (tc_fd) fclose(tc_fd);
}

int tc_mode()
{
    problem::source_file = problem::temp_dir + "/" + Langs[problem::lang]->MainFile;
    string syscmd = "mv -f ", source_temp = problem::temp_dir + "/tempfile.txt";
    syscmd += problem::source_file + " " + source_temp;
    system(syscmd.c_str());
    addfile(problem::source_file, problem::tc_head);
    addfile(problem::source_file, source_temp);
    addfile(problem::source_file, problem::tc_file);
    return -1;
}

//特判
int spj_compare_output(
        string input_file,  //标准输入文件
        string output_file, //用户程序的输出文件
        string spj_path,    //spj路径, change it from exefile to the folder who store the exefile
        string file_spj,    //spj的输出文件
        string output_file_std)
{
#ifdef JUDGE_DEBUG__
    cout<<"inputfile: "<<input_file<<endl;
    cout<<"outputfile: "<<output_file<<endl;
    cout<<"spj_exec: "<<spj_path<<endl;
    cout<<"file_spj: "<<file_spj<<endl;
#endif
    /*
    Improve: Auto rebuild spj.exe while find spj.cpp
    Improve: If spj.exe is not exist, return SE
    Date & Time: 2013-11-10 08:03
    Author: Sine
    */
    if (access((spj_path+"/spj.cpp").c_str(), 0) == 0)
    {
        string syscmd = "g++ -o ";
        syscmd += spj_path + "/"+problem::spj_exe_file+" " + spj_path + "/spj.cpp";
        system(syscmd.c_str());
        syscmd = "mv -f ";
        syscmd += spj_path + "/spj.cpp " + spj_path + "/spj.oldcode";
        system(syscmd.c_str());
    }
    if (access((spj_path+"/"+problem::spj_exe_file).c_str(), 0))
    {
        output_result(judge_conf::OJ_SE, 0, judge_conf::EXIT_ACCESS_SPJ);
        exit(judge_conf::EXIT_ACCESS_SPJ);
    }
    //    return judge_conf::OJ_SE;
    //End of the Improve*/
    int status = 0;
    pid_t pid_spj = fork();
    if (pid_spj < 0)
    {
        LOG_WARNING("error for spj failed, %d:%s", errno, strerror(errno));
        output_result(judge_conf::OJ_SE, -errno, judge_conf::EXIT_COMPARE_SPJ_FORK);
        exit(judge_conf::EXIT_COMPARE_SPJ_FORK);
    }
    else if (pid_spj == 0)
    {
        freopen(file_spj.c_str(), "w", stdout);
        if (EXIT_SUCCESS == malarm(ITIMER_REAL, judge_conf::spj_time_limit))
        {
            log_close();
            //argv[1] 标准输入 ， argv[2] 用户程序输出, argv[3] 标准输出
            if (execlp((spj_path+"/"+problem::spj_exe_file).c_str(),
                problem::spj_exe_file.c_str(), input_file.c_str(),
                output_file.c_str(), output_file_std.c_str(), NULL) < 0)
            {
                printf("spj execlp error\n");
            }
        }
    }
    else
    {
        if (waitpid(pid_spj, &status, 0) < 0)
        {
            LOG_BUG("waitpid failed, %d:%s", errno, strerror(errno));
            output_result(judge_conf::OJ_SE, -errno, judge_conf::EXIT_COMPARE_SPJ_WAIT);
            exit(judge_conf::EXIT_COMPARE_SPJ_WAIT);
        }

        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) == EXIT_SUCCESS)
            {
                FILE *fd = fopen(file_spj.c_str(), "r");
                if (fd == NULL)
                {
                    output_result(judge_conf::OJ_SE, 0, judge_conf::EXIT_COMPARE_SPJ_OUT);
                    exit(judge_conf::EXIT_COMPARE_SPJ_OUT);
                }
                char buf[20];
                if (fscanf(fd, "%19s", buf) == EOF)
                {
                    return judge_conf::OJ_WA;
                }
                if (fd) fclose(fd);
                if (strcmp(buf, "AC") == 0)
                {
                    return judge_conf::OJ_AC;
                }
                else if (strcmp(buf, "PE") == 0)
                {
                    return judge_conf::OJ_PE;
                }
                else if (strcmp(buf, "WA") == 0)
                {
                    return judge_conf::OJ_WA;
                }
            }
        }
    }
    return judge_conf::OJ_WA;
}

//普通比较
int tt_compare_output(string &file_std, string &file_usr)
{
    int ret = judge_conf::OJ_AC;
    int c_std, c_usr;
    FILE *fd_std = fopen(file_std.c_str(), "r");
    FILE *fd_usr = fopen(file_usr.c_str(), "r");
    //char *buffer;
        //buffer = getcwd(NULL, 0);
        //if (buffer != NULL) LOG_DEBUG("this is %s", buffer);

    //LOG_DEBUG("The standard file is %s", file_std.c_str());
    if (fd_std == NULL)
    {
        LOG_BUG("%s open standard file failed %s", strerror(errno), file_std.c_str());
    }
    //if (fd_usr == NULL)
    //{
        //LOG_BUG("open user file failed %s", file_usr.c_str());
    //}

    if (!fd_std || !fd_usr)
    {
        //LOG_DEBUG("compare This is the file: %s\n", problem::input_file.c_str());
        ret = judge_conf::OJ_RE_ABRT;
    }
    else
    {
        c_std = fgetc(fd_std);
        c_usr = fgetc(fd_usr);
        for(;;)
        {
            compare_until_nonspace(c_std, c_usr, fd_std, fd_usr, ret);
            while(!isspace(c_std) && !isspace(c_usr))
            {
            //    LOG_DEBUG("std: %c usr: %c", c_std, c_usr);
                if (c_std == EOF && c_usr == EOF)
                    goto end;
                if (c_std != c_usr)
                {
                    ret = judge_conf::OJ_WA;
                    goto end;
                }
                c_std = fgetc(fd_std);
                c_usr = fgetc(fd_usr);
            }
        }
    }
end:
    if (fd_std)
        fclose(fd_std);
    if (fd_usr)
        fclose(fd_usr);
    return ret;
}

int compare_output(string &file_std, string &file_usr)
{
//    LOG_DEBUG("compare_output ");
    return tt_compare_output(file_std, file_usr);
}

void parse_arguments(int argc, char *argv[]) //根据命令设置好配置.
{
    int opt;
    extern char *optarg;
    while((opt = getopt(argc, argv, "l:D:d:t:m:o:S:T")) != -1)
    {
        //LOG_DEBUG(" -%c", opt);
        switch(opt)
        {
            case 'l': sscanf(optarg, "%d", &problem::lang); break;
            case 'D': problem::data_dir = optarg; break;
            case 'd': problem::temp_dir = optarg; break;
            case 't': sscanf(optarg, "%d", &problem::time_limit); break;
            case 'm': sscanf(optarg, "%d", &problem::memory_limit); break;
            case 'o': sscanf(optarg, "%d", &problem::output_limit); break;
            case 'S': problem::spj = true; break;
            case 'T': problem::tc = true; break;
            default:
                    LOG_WARNING("unknown option -%c %s", opt, optarg);
                    output_result(judge_conf::OJ_SE, 0, judge_conf::EXIT_BAD_PARAM);
                    exit(judge_conf::EXIT_BAD_PARAM);
        }
    }

    problem::time_limit *= Langs[problem::lang]->TimeFactor;
    problem::memory_limit *= Langs[problem::lang]->MemFactor;
    if (problem::tc)
    {
        problem::tc_file = problem::data_dir + "/" + Langs[problem::lang]->TCfile;
        problem::tc_head = problem::data_dir + "/" + Langs[problem::lang]->TChead;
    }
    if (problem::spj == true)
    {
        problem::spj_exe_file = "spj.exe";
        problem::stdout_file_spj = "stdout_spj.txt";
    }
    judge_conf::judge_time_limit += problem::time_limit;
#ifdef JUDGE_DEBUG
    problem::Problem_debug();
#endif
}


//redirect io
void io_redirect()
{
    freopen(problem::input_file.c_str(), "r", stdin);
    freopen(problem::output_file.c_str(), "w", stdout);
    freopen(problem::output_file.c_str(), "w", stderr);
    if (stdin == NULL || stdout == NULL)
    {
        LOG_BUG("error in freopen: stdin(%p) stdout(%p)", stdin, stdout);
        exit(judge_conf::EXIT_PRE_JUDGE);
    }
#ifdef JUDGE_DEBUG
    LOG_DEBUG("io redirece ok!");
#endif
}

void set_limit()
{
    rlimit lim;
    //时间限制
    lim.rlim_cur = (problem::time_limit - problem::time_usage + 999)/1000 + 1;
    lim.rlim_max = lim.rlim_cur * 10;
    if (setrlimit(RLIMIT_CPU, &lim) < 0)
    {
        LOG_BUG("error setrlimit for rlimit_cpu");
        exit(judge_conf::EXIT_SET_LIMIT);
    }

    //内存大小限制 java 无法运行
    /*
         if (problem::lang != judge_conf::LANG_JAVA)
         {
         lim.rlim_max = problem::memory_limit * judge_conf::KILO;
         }else lim.rlim_max = (problem::memory_limit + 8192)*judge_conf::KILO;
         lim.rlim_cur = lim.rlim_max;
         if (setrlimit(RLIMIT_AS, &lim) < 0)
         {
         LOG_BUG("error setrlimit for rlimit_as");
         exit(judge_conf::EXIT_SET_LIMIT);
         }*/
    //设置堆栈的大小    漏掉主程序会SIGSEGV
    getrlimit(RLIMIT_STACK, &lim);
    int rlim = judge_conf::stack_size_limit*judge_conf::KILO;
    //LOG_DEBUG("set stack size : %d", rlim);
    if (lim.rlim_max <= rlim)
    {
        LOG_WARNING("can't set stack size to higher(%d <= %d)", lim.rlim_max, rlim);
    }
    else
    {
        lim.rlim_max = rlim;
        lim.rlim_cur = rlim;
        if (setrlimit(RLIMIT_STACK, &lim) < 0)
        {
            LOG_WARNING("setrlimit RLIMIT_STACK failed: %s", strerror(errno));
            exit(judge_conf::EXIT_SET_LIMIT);
        }
    }

    log_close();

    //输出文件限制
    lim.rlim_max = problem::output_limit * judge_conf::KILO;
    lim.rlim_cur = lim.rlim_max;
    //LOG_BUG("rlim_fsize %d", lim.rlim_max);
    if (setrlimit(RLIMIT_FSIZE, &lim) < 0)
    {
        //LOG_BUG("error setrlimit for rlimit_fsize");
        exit(judge_conf::EXIT_SET_LIMIT);
    }
    //log_close();
}

int Compiler()
{
    int status = 0;
    pid_t compiler = fork();
    if (compiler < 0)
    {
        LOG_WARNING("error fork compiler, %d:%s", errno, strerror(errno));
        output_result(judge_conf::OJ_SE, -errno, judge_conf::EXIT_COMPILE);
        exit(judge_conf::EXIT_COMPILE);
    }
    else if (compiler == 0)
    {
        chdir(problem::temp_dir.c_str());
        freopen("ce.txt", "w", stderr); //编译出错信息
        freopen("/dev/null", "w", stdout); //防止编译器在标准输出中输出额外的信息影响评测
        malarm(ITIMER_REAL, judge_conf::compile_time_limit);
        execvp( Langs[problem::lang]->CompileCmd[0], (char * const *) Langs[problem::lang]->CompileCmd );
        //execvp    error
        LOG_WARNING("compile evecvp error");
        exit(judge_conf::EXIT_COMPILE);
    }
    else
    {
        waitpid(compiler, &status, 0);
        return status;
    }
}

const int bufsize = 1024;
int getmemory(pid_t userexe)
{
    int ret = 0;
    FILE *pd;
    char fn[bufsize], buf[bufsize];
    sprintf(fn, "/proc/%d/status", userexe);
    pd = fopen(fn, "r");
    while(pd && fgets(buf, bufsize-1, pd))    //这里漏掉了pd & 导致主进程SIGSEGV
    {
        if (strncmp(buf, "VmPeak:", 7) == 0)
        {
            sscanf(buf+8, "%d", &ret);
        }
    }
    if (pd) fclose(pd); //9.19 发现漏了，会too many open file
    return ret;
}

bool isInFile(char *filename)
{
    int len = strlen(filename);
    if (len < 3 || strcmp(filename+len-3, ".in") != 0)
        return false;
    return true;
}

void sigseg(int)
{
    output_result(judge_conf::OJ_SE, 0, judge_conf::EXIT_UNPRIVILEGED);
    exit(judge_conf::EXIT_UNPRIVILEGED);
}

int main(int argc, char *argv[])
{
    judge_conf::ReadConf();
    log_open(judge_conf::log_file.c_str());

#ifdef JUDGE_DEBUG
    LOG_DEBUG("start judge");
#endif
    if (geteuid() != 0)
    {
#ifdef JUDGE_DEBUG
        LOG_DEBUG("euid %d != 0, please run as root , or chmod 4755 judge", geteuid());
#endif
        output_result(judge_conf::OJ_SE, 0, judge_conf::EXIT_UNPRIVILEGED);
        exit(judge_conf::EXIT_UNPRIVILEGED);
    }
    parse_arguments(argc, argv);
    //设置judge运行时限
    if (EXIT_SUCCESS != malarm(ITIMER_REAL, judge_conf::judge_time_limit))
    {
        LOG_WARNING("set judge alarm failed, %d : %s", errno, strerror(errno));
        output_result(judge_conf::OJ_SE, 0, judge_conf::EXIT_VERY_FIRST);
        exit(judge_conf::EXIT_VERY_FIRST);
    }
    signal(SIGALRM, timeout);

    //tc 模式
    if (problem::tc)
    {
        tc_mode();
    }

    //编译
    int compile_ok = Compiler();
    if (compile_ok != 0) //测试结果OJ_CE
    {
        output_result(judge_conf::OJ_CE);
        exit(judge_conf::EXIT_OK);
    }

    //运行 judge
    DIR * dp;
    struct dirent *dirp;
    dp = opendir(problem::data_dir.c_str());
    if (dp == NULL)
    {
        LOG_WARNING("error opening dir %s", problem::data_dir.c_str());
        output_result(judge_conf::OJ_SE, 0, judge_conf::EXIT_PRE_JUDGE);
        return judge_conf::EXIT_PRE_JUDGE;
    }
    char nametmp[1024];
    while((dirp = readdir(dp)) != NULL)
    {
        struct rusage rused;

        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
            continue;
        if (isInFile(dirp->d_name))
        {
            strcpy(nametmp, dirp->d_name);
            int len = strlen(nametmp);
            nametmp[len-3] = '\0';
            problem::input_file = problem::data_dir + "/" + nametmp + ".in";
            problem::output_file_std = problem::data_dir + "/" + nametmp + ".out";
            problem::output_file = problem::temp_dir + "/" + nametmp + ".out";

#ifdef JUDGE_DEBUG
            problem::Problem_debug();
#endif

            pid_t userexe = fork();
            if (userexe < 0)
            {
                LOG_WARNING("fork for userexe failed, %d:%s", errno, strerror(errno));
                output_result(judge_conf::OJ_SE, -errno, judge_conf::EXIT_PRE_JUDGE);
                exit(judge_conf::EXIT_PRE_JUDGE);
            }
            else if (userexe == 0)
            {

                signal(SIGSEGV, sigseg);
                //重新定向io
                io_redirect();

                //设置运行根目录、运行用户
                //获得运行用户的信息
                struct passwd *judge = getpwnam(judge_conf::sysuser.c_str());
                if (judge == NULL)
                {
                    LOG_BUG("no user named %s", judge_conf::sysuser.c_str());
                    exit(judge_conf::EXIT_SET_SECURITY);
                }

                //切换目录
                if (EXIT_SUCCESS != chdir(problem::temp_dir.c_str()))
                {
                    LOG_BUG("chdir failed");
                    exit(judge_conf::EXIT_SET_SECURITY);
                }
            /*    char cwd[1024], *tmp = getcwd(cwd, 1024);
                if (tmp == NULL)
                { //获取当前目录失败
                    LOG_BUG("getcwd failed");
                    exit(judge_conf::EXIT_SET_SECURITY);
                }*/

                //#ifdef JUDEG_DEBUG
                //这里现在在fedora下有bug
                //设置根目录
            /*    if (problem::lang == judge_conf::LANG_JAVA)
             *    {
                    if (EXIT_SUCCESS != chroot(cwd))
                    {
                        LOG_BUG("chroot failed");
                        exit(judge_conf::EXIT_SET_SECURITY);
                    }
                }*/

                //#endif
                //设置有效用户
                if (EXIT_SUCCESS != setuid(judge->pw_uid))
                {
                    LOG_BUG("setuid failed");
                    exit(judge_conf::EXIT_SET_SECURITY);
                }


                int user_time_limit = problem::time_limit - problem::time_usage
                    + judge_conf::time_limit_addtion;

                //设置用户程序的运行实际时间，防止意外情况卡住
                if (EXIT_SUCCESS != malarm(ITIMER_REAL, user_time_limit))
                {
                    LOG_WARNING("malarm failed");
                    exit(judge_conf::EXIT_PRE_JUDGE);
                }

                //ptrace 监控下面程序
                if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0)
                {
                    LOG_BUG("ptrace failed");
                    exit(judge_conf::EXIT_PRE_JUDGE_PTRACE);
                }

                //用setrlimit 设置用户程序的 内存 时间 输出文件大小的限制
                //log close for fsize
                set_limit();

                //运行程序
                execvp( Langs[problem::lang]->RunCmd[0], (char * const *) Langs[problem::lang]->RunCmd );
                int errsa = errno;
                exit(judge_conf::EXIT_PRE_JUDGE_EXECLP);
            }
            else
            {
                //父进程监控子进程的状态和系统调用

                int status = 0;
                int syscall_id = 0;
                struct user_regs_struct regs;

                init_ok_table(problem::lang);

                while (true)
                {
                    if (wait4(userexe, &status, 0, &rused) < 0)
                    {
                        LOG_BUG("wait4 failed, %d:%s", errno, strerror(errno));
                        output_result(judge_conf::OJ_SE, -errno, judge_conf::EXIT_JUDGE);
                        exit(judge_conf::EXIT_JUDGE);
                    }
                    //如果是退出信号
                    if (WIFEXITED(status))
                    {
                        //java 返回非0表示出错
                        if (!Langs[problem::lang]->VMrunning || WEXITSTATUS(status) == EXIT_SUCCESS)
                        {
                            int result;
                            if (problem::spj)
                            {
                                //spj
                                result = spj_compare_output(problem::input_file,
                                    problem::output_file,
                                    problem::data_dir, //problem::spj_exe_file, modif y in 13-11-10
                                    problem::temp_dir + "/" + problem::stdout_file_spj,
                                    problem::output_file_std);
                            }
                            else
                            {
                                result = compare_output(problem::output_file_std, problem::output_file);
                            }
                            //记录结果
                            if (result != judge_conf::OJ_AC)
                            {
                                problem::result = result;
                            }
                            else
                            {
                                if (problem::result != judge_conf::OJ_PE)
                                    problem::result = result;
                            }
                        }
                        else
                        {
                            LOG_BUG("abort quit code: %d\n", WEXITSTATUS(status));
                            problem::result = judge_conf::OJ_RE_JAVA;
                        }
                        break;
                    }

                    // 收到信号
                    // RE/TLE/OLE     超过sterlimit限制而结束
                    // 且过滤掉被ptrace暂停的 SIGTRAP
                    if (WIFSIGNALED(status) || (WIFSTOPPED(status) && WSTOPSIG(status) != SIGTRAP))
                    {
                        int signo = 0;
                        if (WIFSIGNALED(status))
                            signo = WTERMSIG(status);
                        else
                            signo = WSTOPSIG(status);
                        switch(signo)
                        {
                            //TLE
                //            case SIGALRM:
                                //LOG_BUG("ALRM");
                            case SIGXCPU:
                                //LOG_BUG("XCPU");
                //            case SIGVTALRM:
                                //LOG_BUG("TALRM");
                            case SIGKILL:
                                //LOG_BUG("KILL");
                                problem::result = judge_conf::OJ_TLE;
                                break;
                            //OLE
                            case SIGXFSZ:
                                problem::result = judge_conf::OJ_OLE;
                                break;
                            case SIGSEGV:
                                problem::result = judge_conf::OJ_RE_SEGV;
                                break;
                            case SIGABRT:
                                problem::result = judge_conf::OJ_RE_ABRT;
                                break;
                            default:
                                problem::result = judge_conf::OJ_RE_UNKNOW;
                        }
                        //This is a debug
                        LOG_DEBUG("This is the file: %s\n", problem::input_file.c_str());
                        //
                        ptrace(PTRACE_KILL, userexe);
                        break;
                    }//end if

                    //MLE
//                    LOG_DEBUG("old memory: %d , new memory: %d", problem::memory_usage,
//                            rused.ru_minflt *(getpagesize()/judge_conf::KILO));
                    int tempmemory = 0;

                    if (Langs[problem::lang]->VMrunning)
                    {
                        tempmemory = rused.ru_minflt *(getpagesize()/judge_conf::KILO);
                    }
                    else
                    {
                        tempmemory = getmemory(userexe);
                    }
                    problem::memory_usage = Max(problem::memory_usage, tempmemory);
                    if (problem::memory_usage > problem::memory_limit)
                    {
                        problem::result = judge_conf::OJ_MLE;
                        ptrace(PTRACE_KILL, userexe);
                        break;
                    }

                    //检查userexe的syscall
                    if (ptrace(PTRACE_GETREGS, userexe, 0, &regs) < 0)
                    {
                        LOG_BUG("error ptrace ptrace_getregs, %d:%s", errno, strerror(errno));
                        output_result(judge_conf::OJ_SE, -errno, judge_conf::EXIT_JUDGE);
                        exit(judge_conf::EXIT_JUDGE);
                    }
#ifdef __i386__
                    syscall_id = regs.orig_eax;
#else
                    syscall_id = regs.orig_rax;
#endif

                    if (syscall_id > 0 && (!is_valid_syscall(problem::lang, syscall_id, userexe, regs)))
                    {
                        LOG_WARNING("error for syscall %d", syscall_id);
                        problem::result = judge_conf::OJ_RF;
                        problem::time_usage = syscall_id;
                        ptrace(PTRACE_KILL, userexe, NULL, NULL);
                        break;
                    }

                    if (ptrace(PTRACE_SYSCALL, userexe, NULL, NULL) < 0)
                    {
                        LOG_BUG("error ptrace ptrace syscall, %d:%s", errno, strerror(errno));
                        output_result(judge_conf::OJ_SE, -errno, judge_conf::EXIT_JUDGE);
                        exit(judge_conf::EXIT_JUDGE);
                    }
                }//while (true)
            }//else     userexe end

            if (problem::result == judge_conf::OJ_RF) break;

            int time_tmp = rused.ru_utime.tv_sec*1000 + rused.ru_utime.tv_usec/1000
              + rused.ru_stime.tv_sec*1000 + rused.ru_stime.tv_usec/1000;
            if (problem::time_usage < time_tmp)
            {
                problem::time_usage = time_tmp;
            }

            if (problem::time_usage > problem::time_limit)
            {
                problem::time_usage = problem::time_limit;
                problem::result = judge_conf::OJ_TLE;
            }

            if (problem::memory_usage > problem::memory_limit)
            {
                problem::memory_usage = problem::memory_limit;
                problem::result = judge_conf::OJ_MLE;
            }

            if (problem::result != judge_conf::OJ_AC && problem::result != judge_conf::OJ_PE)
            {
                break;
            }

        }//if (isInfile())

    }//end while, next input file
    output_result(problem::result, problem::memory_usage, problem::time_usage);
    return 0;
}
