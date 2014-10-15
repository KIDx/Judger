#ifndef _LANGUAGE_H
#define _LANGUAGE_H

#include <string>

namespace LanguageSupport
{

struct LangSupport
{
    std::string Name;                 //编程语言名称
    std::string MainFile;             //待测程序源码文件
    std::string TCfile;               //TC模式测试文件的文件名
    std::string TChead;               //TC模式附加头文件的文件名
    const char* const CompileCmd[20]; //编译待评测程序的命令行
    const char* const RunCmd[20];     //运行待评测程序的命令行
    int TimeFactor;                   //时间限制的倍数
    int MemFactor;                    //内存限制的倍数
    bool VMrunning;                   //该语言是否以虚拟机方式运行
};

const LangSupport UnknownLang = {
    "unknown", "NA", "NA", "NA",
    {NULL},
    {NULL},
    0, 0, false
};

const LangSupport CLang = {
    "c", "Main.c", "tc.c", "tc.h",
#ifdef JUDGE_DEBUG
    {"gcc","Main.c","-o","Main", 
    "-std=c99", "-O2", NULL},
#else
    {"gcc", "Main.c", "-o", "Main", "-Wall", "-lm",
    "--static", "-std=c99", "-DONLINE_JUDGE", NULL },
#endif
    {"./Main", NULL},
    1, 1, false
};

const LangSupport CppLang = {
    "c++", "Main.cpp", "tc.cpp", "tc.hpp",
#ifdef JUDGE_DEBUG
    {"g++","Main.cpp","-o",
    "Main", "-std=c++98", "-O2",NULL},
#else
    { "g++", "Main.cpp", "-o", "Main", "-std=c++98",
     "-Wall","-lm", "--static", "-DONLINE_JUDGE", NULL },
#endif
    {"./Main", NULL},
    1, 1, false
};

const LangSupport JavaLang = {
    "java", "Main.java", "tc.java", "tch.java",
#ifdef JUDGE_DEBUG
    {"javac", "Main.java", NULL },
#else
    { "javac", "-J-Xms128M", "-J-Xmx512M", "Main.java", NULL },
#endif
    { "java", "-Xms128M", "-Xms512M", "-DONLINE_JUDGE=true", "Main", NULL },
    2, 2, true
};

const LangSupport CC11Lang = {
    "c++11", "Main.cpp", "tc11.cpp", "tc11.hpp",
#ifdef JUDGE_DEBUG
    {"g++","Main.cpp","-o",
    "Main", "-std=c++11","-O2",NULL},
#else
    { "g++", "Main.cpp", "-o", "Main", "-std=c++11",
    "-Wall", "-lm", "--static", "-DONLINE_JUDGE", NULL },
#endif
    {"./Main", NULL},
    1, 1, false
};

const LangSupport CSLang = {
    "C#", "Main.cs", "tc.cs", "tch.cs",
    {"gmcs", "-define:ONLINE_JUDGE", "-warn:0", "Main.cs", NULL},
    {"mono", "Main.exe", NULL},
    2, 2, false
};

const LangSupport VBLang = {
    "VB.Net", "Main.vb", "tc.vb", "tch.vb",
    {"vbnc", "-define:ONLINE_JUDGE", "-nowarn", "Main.vb", NULL},
    {"mono", "Main.exe", NULL},
    2, 2, false
};

}; //End of namespace

LanguageSupport::LangSupport const *Langs[] =
{
    &LanguageSupport::UnknownLang,
    &LanguageSupport::CLang,
    &LanguageSupport::CppLang,
    &LanguageSupport::JavaLang,
    &LanguageSupport::CC11Lang,
    &LanguageSupport::CSLang,
    &LanguageSupport::VBLang
};

namespace judge_conf
{
    //OJÓïÑÔ
    const int LANG_UNKNOWN  = 0;
    const int LANG_C        = 1;
    const int LANG_CPP      = 2;
    const int LANG_JAVA     = 3;
    const int LANG_CC11     = 4;
    const int LANG_CS       = 5;
    const int LANG_VB       = 6;
};

#endif

//不要忘了修改okcall.h
