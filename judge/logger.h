/*
 * c++
 * A logger for Linux ,  multiprocess safe
 * Author: TT_last
 * 
 * Use:
 * //Open log file first
 * log_open("log.txt");
 *
 * //use it as printf("Hello %s\n", name)
 * LOG_DEBUG("Here is a bug %d", line);
 * LOG_NOTICE("-_- ");
 * LOG_WARNING("...");
 * LOG_BUG("-_-!!!");
 * 
 * //close log file
 * log_close();
 *
 * //3 level: DEBUG NOTICE WARNING BUG
 */

#ifndef __LOGGER_H__ 
#define __LOGGER_H__

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int log_open(const char *filename);
void log_close();
static int log_write(int level, const char *file, const int line, const char *fmt, ...);
int log_lock(int fd, int cmd, int type, off_t offset, int whence, off_t len);

const int LOG_DEBUG = 0;
const int LOG_NOTICE = 1;
const int LOG_WARNING = 2;
const int LOG_BUG = 3;

static char LOG_INFO[][10] = 
{"DEBUG", "NOTICE", "WARNING", "BUG"};

// Lock file log
#define LOG_WLOCK(fd, offset, whence, len) \
    log_lock(fd, F_SETLKW, F_WRLCK, offset, whence, len)
#define LOG_UNLOCK(fd, offset, whence, len) \
    log_lock(fd, F_SETLK, F_UNLCK, offset, whence, len)


#define LOG_DEBUG(x...) log_write(LOG_DEBUG,  __FILE__,  __LINE__,  ##x)
#define LOG_NOTICE(x...) log_write(LOG_NOTICE,  __FILE__,  __LINE__,  ##x)
#define LOG_WARNING(x...) log_write(LOG_WARNING,  __FILE__,  __LINE__,  ##x)
#define LOG_BUG(x...) log_write(LOG_BUG,  __FILE__,  __LINE__,  ##x)

//log 文件指针 文件名 线程安全
static    int log_fd = -1;
static char *log_filename = NULL;
static int log_opened = 0;

#define log_buffer_size 8192 // 8k
static char log_buffer[log_buffer_size];

int log_open(const char *filename)
{
    if(log_opened)
    {
        fprintf(stderr, "logger: log is opened\n");
        return 1;
    }
    int len = strlen(filename);
    log_filename = (char *) malloc(sizeof(char)*len + 1);
    strcpy(log_filename, filename);
    
    log_fd = open(log_filename, O_APPEND|O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);

    if(log_fd == -1)
    {
        printf("14 0 -38\n");
        fprintf(stderr, "log_file: %s", log_filename);
        perror("Can not open log file");
        exit(1);
    }
    log_opened = 1;
    atexit(log_close);

    return 0;
}

void log_close()
{
    if(log_opened)
    {
        close(log_fd);
        free(log_filename);
        log_fd = -1;
        log_filename = NULL;
        log_opened = 0;
    }
}

int log_lock(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
    struct flock lock;
    //F_RDLCK    F_WRLCK    F_UNLCK
    lock.l_type = type;
    //byte offset, relation to l_whence
    lock.l_start = offset;
    //SEEK_SET SEEK_CUR SEEK_END
    lock.l_whence = whence;
    //**bytes (0 means to eof)
    lock.l_len = len;

    return ( fcntl(fd, cmd, &lock) );
}

int log_write(int level, const char *file, const int line, const char *fmt, ...)
{
    if(log_opened == 0)
    {
        fprintf(stderr, "log_open not called yet\n");
        exit(1);
    }
    static char buffer[log_buffer_size];
    static char datatime[100];
    static time_t now;

    now = time(NULL);
    strftime(datatime, 99, "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(log_buffer, log_buffer_size, fmt, ap);
    va_end(ap);

    size_t count = snprintf(buffer, log_buffer_size, "%s [%s] [%s:%d]--->%s\n",
        LOG_INFO[level], datatime, file, line, log_buffer);

    //write(log_fd, buffer, count);
    if(LOG_WLOCK(log_fd, 0, SEEK_SET, 0) < 0)
    {
        perror("lock error");
        exit(1);
    }
    else
    {
        if(write(log_fd, buffer, count) < 0)
        {
            perror("write error");
            exit(1);
        }
        LOG_UNLOCK(log_fd, 0, SEEK_SET, 0);        
    }
    return 0;
}

#endif

/* debug
int main()
{
    pid_t pid = fork();
    //if(pid > 0)
    //{
        //log_open("log.txt");
        //LOG_WLOCK(log_fd, 0, SEEK_SET, 0);
        //sleep(5);
        //LOG_UNLOCK(log_fd, 0, SEEK_SET, 0);
    //}else
    //{
        //sleep(1);
        //printf("chirld running\n");
        fork(); fork();
        for(int i = 0;i < 5;i ++)
        {
            log_open("log.txt");
            LOG_BUG("here is a bug %d", getpid());
            LOG_DEBUG("here is a debug %d", getpid());
            log_close();
        }
    //}
    return 0;
}*/


