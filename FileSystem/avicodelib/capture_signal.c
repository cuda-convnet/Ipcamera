#include <errno.h>
#include <linux/fb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <linux/prctl.h>  
#include <signal.h>
 #include <pthread.h>
 
#include "devfile.h"


#define TRUE   		1			/* Boolean constants */
#define FALSE 		0

/*
   -------------------------------------------------------------------------
       SIGHUP        1       Term    Hangup detected on controlling terminal
                                     or death of controlling process
       SIGINT        2       Term    Interrupt from keyboard
       SIGQUIT       3       Core    Quit from keyboard
       SIGILL        4       Core    Illegal Instruction
       SIGABRT       6       Core    Abort signal from abort(3)
       SIGFPE        8       Core    Floating point exception
       SIGKILL       9       Term    Kill signal
       SIGSEGV      11       Core    Invalid memory reference
       SIGPIPE      13       Term    Broken pipe: write to pipe with no readers
       SIGALRM      14       Term    Timer signal from alarm(2)
       SIGTERM      15       Term    Termination signal
       SIGUSR1   30,10,16    Term    User-defined signal 1
       SIGUSR2   31,12,17    Term    User-defined signal 2
       SIGCHLD   20,17,18    Ign     Child stopped or terminated
       SIGCONT   19,18,25            Continue if stopped
       SIGSTOP   17,19,23    Stop    Stop process
       SIGTSTP   18,20,24    Stop    Stop typed at tty
       SIGTTIN   21,21,26    Stop    tty input for background process
       SIGTTOU   22,22,27    Stop    tty output for background process  
*/
void SignalHander(int signal)
{
    static int bExit = FALSE;
    char name[32];
    unsigned int dwFdNum = 0, dwTaskNum = 0, dwTotalMem = 0, dwFreeMem = 0;
    if(bExit == FALSE)
    {
        bExit = TRUE;	
		
        printf("____________Application will exit by signal:%d, pid:%d, fd:%ld, tasknum:%ld,FreeMem:%ld\n",
								signal, getpid(), dwFdNum, dwTaskNum,dwFreeMem);
        prctl(PR_GET_NAME, (unsigned long)name);
        printf("____________Application exit thread name %s\n", name);

	{
		char msg[256];		
		sprintf(msg,"Application exit thread name %s signal:%d, pid:%d\n",name,signal, getpid());
		//XdvrWriteImportantLog(msg,strlen(msg));	
	}
        

        if(signal == SIGSEGV)
        {
            SIG_DFL(signal);
        }
        else
        {
            exit(1);
        }
    }
}


/*********************************************************************
    Function:
    Description:
        捕捉所有的信号，主要用于类似死机的BUG调试
    Calls:
      Called By:
    parameter:
      Return:
      author:z50825
            capture SIGKILL SIGINT and SIGTERM to protect disk
********************************************************************/
void CaptureAllSignal()
{
    int i = 0;
    for(i = 0; i < 32; i ++)
    {
        if ( (i == SIGPIPE) || (i == SIGCHLD) || (i == SIGALRM))
        {           
            //2010-10-14 10:33:01 薛长春 
            //此异常是忽略网络异常，不可注释
            //Daniel，2011。由于CD刻录的问题，此处也需要忽略SIGCHLD信号
            signal(i, SIG_IGN);     
        }
        else
        {
            signal(i, SignalHander);
        }
    } 
}

/*
void dump(int signo)
{
    char buf[1024];
    char cmd[1024] = {0};
    FILE *fh;
    
    snprintf(buf, sizeof(buf), "/proc/%d/cmdline", getpid());
    if(!(fh = fopen(buf, "r")))
        exit(0);
    if(!fgets(buf, sizeof(buf), fh))
        exit(0);
    fclose(fh);
    if(buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = '\0';
    snprintf(cmd, sizeof(cmd), "./gdb %s %d", buf, getpid());
    SYSTEMLOG(SLOG_TRACE, "%s\n", cmd);
//  system(cmd);
    SystemCall_msg(cmd,SYSTEM_MSG_BLOCK_RUN);
    exit(0);
}


main()
{
    prctl(PR_SET_NAME, (unsigned long)"main", 0,0,0);
    CaptureAllSignal();
}
*/