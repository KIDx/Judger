#ifndef __JUDGE_H__
#define __JUDGE_H__

#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <cctype>
#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include "logger.h"
#include "language.h"

//#define JUDGE_DEBUG

namespace judge_conf
{
    const char config_file[] = "config.ini";

    std::string log_file = "log.txt";

    std::string sysuser = "kidx";

    int judge_time_limit = 40000;

    int stack_size_limit = 8192;

    int compile_time_limit = 60000;

    int spj_time_limit = 10000;

    int time_limit_addtion = 10000;

    void ReadConf()
    {
        FILE *conf = fopen(config_file, "r");
        if (!conf) return;
        enum { UNKNOW, JUDGE, SYSTEM } group;
        std::map<std::string, std::string> judm, sysm;
        char line[1024], key[512], val[512];
        while (~fscanf(conf, "%s", line))
        {
            switch (line[0])
            {
            case '[':
                if (!strncmp(line+1, "judge", 5))
                    group = JUDGE;
                else if (!strncmp(line+1, "system", 6))
                    group = SYSTEM;
                else group = UNKNOW;
            case '#':
                break;
            default:
                sscanf(line, "%[^=]=%s", key, val);
                switch (group)
                {
                case JUDGE:
                    judm[key] = val;
                    break;
                case SYSTEM:
                    sysm[key] = val;
                    break;
                default:
                    break;
                }
            }
        }
        if (judm.find("judge_time_limit") != judm.end())
            sscanf(judm["judge_time_limit"].c_str(), "%d", &judge_time_limit);

        if (judm.find("stack_size_limit") != judm.end())
            sscanf(judm["stack_size_limit"].c_str(), "%d", &stack_size_limit);

        if (judm.find("compile_time_limit") != judm.end())
            sscanf(judm["compile_time_limit"].c_str(), "%d", &compile_time_limit);

        if (judm.find("spj_time_limit") != judm.end())
            sscanf(judm["spj_time_limit"].c_str(), "%d", &spj_time_limit);

        if (sysm.find("log_file") != sysm.end())
            log_file = sysm["log_file"];

        if (sysm.find("sysuser") != sysm.end())
            sysuser = sysm["sysuser"];
    }

    const int OJ_WAIT       = 0;    //Queue
    const int OJ_RUN        = 1;    //RUN
    const int OJ_AC         = 2;    //AC
    const int OJ_PE         = 3;    //PE
    const int OJ_TLE        = 4;    //TLE
    const int OJ_MLE        = 5;    //MLE
    const int OJ_WA         = 6;    //WA
    const int OJ_OLE        = 7;    //OLE
    const int OJ_CE         = 8;    //CE
    const int OJ_RE_SEGV    = 9;    //SEG Violation
    const int OJ_RE_FPU     = 10;   //float.../0
    const int OJ_RE_ABRT    = 11;   //Abort
    const int OJ_RE_UNKNOW  = 12;   //Unknow
    const int OJ_RF         = 13;   //Restricted Function
    const int OJ_SE         = 14;   //System Error
    const int OJ_RE_JAVA    = 15;   //Java Run Time Exception

    const int KILO          = 1024;         //1K
    const int MEGA          = KILO*KILO;    //1M
    const int GIGA          = KILO*MEGA;    //1G

    const int EXIT_OK               = 0;
    const int EXIT_UNPRIVILEGED     = 1;
    const int EXIT_BAD_PARAM        = 3;
    const int EXIT_VERY_FIRST       = 4;
    const int EXIT_COMPILE          = 6;
    const int EXIT_PRE_JUDGE        = 9;
    const int EXIT_PRE_JUDGE_PTRACE = 10;
    const int EXIT_PRE_JUDGE_EXECLP = 11;
    const int EXIT_SET_LIMIT        = 15;
    const int EXIT_SET_SECURITY     = 17;
    const int EXIT_JUDGE            = 21;
    const int EXIT_COMPARE          = 27;
    const int EXIT_ACCESS_SPJ       = 29;
    const int EXIT_RUNTIME_SPJ      = 30;
    const int EXIT_COMPARE_SPJ_FORK = 31;
    const int EXIT_COMPARE_SPJ_WAIT = 32;
    const int EXIT_COMPARE_SPJ_OUT  = 33;
    const int EXIT_TIMEOUT          = 36;
    const int EXIT_NO_LOGGER        = 38;
    const int EXIT_UNKNOWN          = 127;
};

namespace problem
{
    int id = 0;
    int lang = 0;
    int time_limit = 1000;
    int memory_limit = 65536;
    int output_limit = 8192;
    int result = judge_conf::OJ_SE;
    int status;

    long memory_usage = 0;
    long time_usage = 0;
    bool tc = false;
    bool spj = false;

    std::string uid;
    std::string temp_dir = "temp";
    std::string data_dir = "data";


    std::string source_file;
    std::string tc_file;
    std::string tc_head;
    std::string spj_exe_file;

    std::string input_file;
    std::string output_file;
    std::string output_file_std;
    std::string stdout_file_spj;


#ifdef JUDGE_DEBUG
    void Problem_debug()
    {
        LOG_DEBUG("----Problem\tinformation----");
        LOG_DEBUG("Problem spj : %s Problem tc %s", spj?"True":"False", tc?"True":"False");
        LOG_DEBUG("time_limit %d memory_limit %d", time_limit, memory_limit);
        LOG_DEBUG("output_limit %d", output_limit);

        LOG_DEBUG("temp_dir %s", temp_dir.c_str());
        LOG_DEBUG("data_dir %s", data_dir.c_str());

        LOG_DEBUG("input_file %s", input_file.c_str());
        LOG_DEBUG("output_file %s", output_file.c_str());
        LOG_DEBUG("output_file_std %s", output_file_std.c_str());
    }
#endif
};

// example:malarm(ITIMER_REAL, judge_conf::judge_time_limit);
int malarm(int which, int milliseconds)
{
    struct itimerval it;
    it.it_value.tv_sec = milliseconds/1000;
    it.it_value.tv_usec = (milliseconds%1000)*1000;
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    return setitimer(which, &it, NULL);
}

#endif
