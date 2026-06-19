
#ifdef USE_LIBUSEFUL_BUNDLED
  #include "libUseful-bundled/libUseful.h"
#else
  #ifdef HAVE_LIBUSEFUL4
  #include "libUseful-4/libUseful.h"
  #else
  #include "libUseful-5/libUseful.h"
  #endif
#endif

#include <stdarg.h>
#include <syslog.h>

#define FLAG_WAIT             1
#define FLAG_RUN_ANYWAY       2
#define FLAG_DELETE_FILE      4
#define FLAG_TERM_OWNER       8
#define FLAG_KILL_OWNER      16
#define FLAG_NO_WRITE        32
#define FLAG_GOT_LOCK        64
#define FLAG_NO_PROGRAM     128
#define FLAG_CLOEXEC        256
#define FLAG_BACKGROUND     512
#define FLAG_SILENT        1024
#define FLAG_DEBUG         2048
#define FLAG_NOHUP         4096
#define FLAG_INTERVAL_FILE 8192
#define FLAG_RESTART      16384
#define FLAG_SYSLOG       32768
#define FLAG_SU           65536
#define FLAG_NETNS       131072

#define RESULT_GOTLOCK 0
#define RESULT_BADARGS 1
#define RESULT_NOOPEN  2
#define RESULT_NOLOCK  3

#define MAX_PIDFILE_SIZE 25

#define LOCK_FAIL 0
#define LOCK_OKAY 1
#define LOCK_NOT_TIME 2

typedef struct
{
    int Flags;
    char *Path;
    int fd;
} TLockFile;

typedef struct
{
    int Flags;
    int Timeout;
    int GraceTime;
    int AbandonTime;
    int Interval;
    int RestartInterval;
    int MaxPids;
    int UserMaxPids;
    int MaxFiles;
    ListNode *LockFiles;
    char *Program;
    char *OnFail;
    char *User;
    char *Group;
    char *Security;
    char *MaxMemory;
    char *MaxFileSize;
} TSettings;

TSettings Settings;

char *Version="5.0";


void OpenSysLog()
{
    if (Settings.Flags & FLAG_SYSLOG)
    {
        openlog(NULL, LOG_NDELAY | LOG_PID, LOG_DAEMON);
    }
}


void LogEvent(int Level, const char *Fmt, ...)
{
    va_list args;
    char *Tempstr=NULL;

    va_start(args, Fmt);
    Tempstr=VFormatStr(Tempstr, Fmt, args);
    va_end(args);

    if (! (Settings.Flags & FLAG_SILENT))
    {
        if ( (Level != LOG_DEBUG) || (Settings.Flags & FLAG_DEBUG) ) printf("%s %s\n", GetDateStr("%Y-%m-%dT%H:%M:%S",NULL),  Tempstr);
    }

    if (Settings.Flags & FLAG_SYSLOG)
    {
        if (! StrValid(Settings.Program)) syslog(Level, "(no program) %s", Tempstr);
        syslog(Level, "(%s) %s", Settings.Program, Tempstr);
    }

    Destroy(Tempstr);
}


void SigHandler(int sig)
{
    if (sig==SIGALRM)
    {
        kill(0,SIGKILL);

        //use async/thread-safe quick_exit if available
#ifdef HAVE_QUICK_EXIT
        quick_exit(1);
#endif

        //else fall back to POSIX _exit,
        //which should also be async-safe
        //(whereas exit isn't)
        _exit(1);
    }
}



//this is supplied independantly of the version in libUseful
//incase the libuseful getlock is compiled against doesn't have this
int GetlockNoNewPrivs()
{
#ifdef HAVE_PRCTL
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#ifdef PR_SET_NO_NEW_PRIVS

//set, then check that the set worked. This correctly handles situations where we ask to set more than once
//as the second attempt may 'fail', but we already have the desired result
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    if (prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) == 1) return(TRUE);

    LogEvent(LOG_ERR, "Failed to set 'no new privs': '%s'. It may still be possible for process to go superuser", strerror(errno));
#else
    LogEvent(LOG_WARNING, "This platform doesn't seem to support the 'no new privs' option. It may still be possible for process to go superuser");
#endif
#else
    LogEvent(LOG_WARNING, "This platform doesn't seem to support the 'no new privs' option. It may still be possible for process to go superuser");
#endif
#endif

    return(FALSE);
}


void PrintSecureUsage()
{
printf("The '-secure' option allows passing a config to the process security layer of libUseful, which is mostly built around seccomp sandboxing. Obviously, for this to work libUseful has to have been built with seccomp and namespaces enabled.\n");
printf("The config consists of a 'security level' and additional 'security modifiers', combined with a '+' symbol, like so: getlock -secure guest+nonet\n");
printf("\nSecurity Levels:\n");
printf("   minimal     - disable ptrace, deny ioctl(TIOCSTI) and kill apps that try to use: personality, uselib, userfaultfd, perf_event_open, kexec_load, get_kernel_syms, lookup_dcookie, vm86, vm86old, mbind, move_pages, nfsservctl, and anything involving kernel modules\n");
printf("   basic       - everything in 'minimal' but also disable the 'acct' and 'capset' syscalls\n");
printf("   user        - everything in 'basic' but also kill processes that try to use bpf, or any 'sysadmin' calls: settimeofday, clocksettime, clockadjtime, quotactl, reboot, swapon, swapoff, mount, umount, umount2, mknod, quotactl capset, or that try to use the TIOCSTI ioctl\n");
printf("   guest       - everything in 'user' but also deny keyring access and kill attempts to use ptrace\n");
printf("   untrusted   - everyting in 'user' but kill apps that try to: chroot, acct syscall, pidfd_open syscall, access the keyring, unshare or change namespaces, and deny access to utimes syscall\n");
printf("   constrained - everything in 'untrusted' but kill apps that try to change filesystem times, and deny all ioctls except those listed in ioctl(user)\n");
printf("   high        - everything in 'constrained' plus deny sending signals, making filesystem links and symlinks, or chmoding files to be exectuable\n");
printf("   paranoid    - everything in 'high' but kill processes that try to use exec to load another program, or that try to link or symlink to files, or change file timestamps, or send signals\n");
printf("   worker      - everything in 'paranoid' plus deny making any filesystem changes. Intended for processes that just do calculations and write them to a file\n");
printf("   memworker   - everything in 'worker' plus kill attempted use of 'open' or other filesystem calls/changes, or 'pipe'. Intended for processes that just do calculations and write them to an existing file descriptor (e.g. stdout).\n");
printf("\nSecurity Modifiers:\n");
printf("   local       - only allow UNIX sockets/networking. WARNING: this modifier only works on x86_64, not on x86 due to issues with socketcall syscall\n");
printf("   basic-net   - only allow unix, inet (IPv4) and inet6 (IPv6) sockets/networking, disabling all 'weird' address families including AF_BLUETOOTH, AF_VSOCK and AF_ALG. WARNING: this modifier only works on x86_64, not on x86 due to issues with socketcall syscall\n");
printf("   lan         - only allow connections to devices on the same ethernet segment. This doesn't use seccomp, but instead causes all sockets to be opened with the SO_DONTROUTE flag set.\n");
printf("   client      - deny syscalls: listen and accept, preventing 'server' activity\n");
printf("   nonet       - use namespaces to prevent network access AND/OR SECCOMP to deny networking syscalls like socket,bind and connect\n");
printf("   killnet     - kill processes that attempt to use networking syscalls like socket,bind and connect\n");
printf("   netns       - use namespaces to prevent access to network\n");
printf("   nopid       - use namespaces AND/OR SECCOMP to prevent access to other processes, sending signals or even seeing what processes are running (deny 'kill' syscall)\n");
printf("   pidns       - use namespaces to prevent access to other processes, sending signals or even seeing what processes are running\n");
printf("   noipc       - use namespace AND/OR SECCOMP to isolate posix IPC (deny syscalls included in both 'noshm' and 'nomsgq' above along with syscalls: semop, semget and semctl)\n");
printf("   ipcns       - use namespaces to prevent access to posix IPC (shared memory, message queues and semaphores)\n");
printf("   noinit      - when using a PID namespace, don't create an 'init' process for it, just run the main process in the namespace\n");
printf("   noshm       - deny 'shared memory' syscalls: shmat, shmdt, shmget and shmctl\n");
printf("   nomsgq      - deny 'message queues' syscalls: msgrcv, msgsnd, msgget, and msgctl\n");
printf("   noexec      - deny use of exec family of programs to load another program\n");
printf("\nExample:\n\n");
printf("   getlock -secure high+nopid+client /var/lock/myprogram.lck myprogram\n\n");
printf("run 'myprogram' with 'high' security, in a pid namespace, denying access to 'kill syscall' and limiting network comms to client (no listen or accept allowed for server communication).\n");
}




void PrintUsage()
{
    printf("getlock %s:\n",Version);
    printf("Author: Colum Paget\n");
    printf("Email: colums.projects@gmail.com\n");
    printf("\n");
    printf("getlock tries to lock 1 or more lockfiles, and then runs a program if it can (or doesn't, if -N used).\n");
    printf("USAGE:\n	getlock [options] LockFilePath [LockFilePath] ... Program\n\n");
    printf("Note that if you wish to pass arguments to the program, then you must quote it like this: getlock /var/lock/lockfile.lck \"/usr/bin/md5sum /tmp/*\"\n\n");

    printf("%-20s %s\n","-a <seconds>","Abandon running task, killall subprocesses and exit after <seconds>");
    printf("%-20s %s\n","-b","Fork into background when got lock");
    printf("%-20s %s\n","-d","delete lockfile when done");
    printf("%-20s %s\n","-n","'nohup', ignore SIGHUP");
    printf("%-20s %s\n","-nohup","ignore SIGHUP");
    printf("%-20s %s\n","-w","wait for lock (default is do not wait)");
    printf("%-20s %s\n","-t <n>","timeout <n> secs on wait (default is wait forever if -w used)");
    printf("%-20s %s\n","-i <n>","interval between runs. Defauts to seconds, suffix with 'm' for minutes, 'h' for hours, 'd' for days.");
    printf("%-20s %s\n","-s","SAFE MODE, do not write pid into file");
    printf("%-20s %s\n","-C","Close on exec, this prevents lockfiles being inherited by the child process.");
    printf("%-20s %s\n","-D","Debug. Print what you're doing.");
    printf("%-20s %s\n","-R","Restart program if it exits.");
    printf("%-20s %s\n","-q <n>","Restart interval. Milliseconds delay between restart attempts.");
    printf("%-20s %s\n","-S","Silent. No error/info/debug messages.");
    printf("%-20s %s\n","-L","Send error/info/debug messages to syslog.");
    printf("%-20s %s\n","-syslog","Send error/info/debug messages to syslog.");
    printf("%-20s %s\n","-F","Run specified program if can't get lock");
    printf("%-20s %s\n","-N","NO PROGRAM, just lock files, use with -w");
    printf("%-20s %s\n","-X","execute program even if lock fails!");
    printf("%-20s %s\n","-g <n>","Gracetime, wait <n> secs before doing kills");
    printf("%-20s %s\n","-k","Send SIGTERM to lockfile owner");
    printf("%-20s %s\n","-K","Send SIGKILL (kill -9) to lockfile owner");
    printf("%-20s %s\n","-su","'allow su' allow process escalate privilege");
    printf("%-20s %s\n","-pids <n>","max child pids (requires linux namespaces to work)");
    printf("%-20s %s\n","-upids <n>","max system-wide pids owned by the user of the run program");
    printf("%-20s %s\n","-files <n>","max files open by run program");
    printf("%-20s %s\n","-mem <size>","max memory in bytes. 'size' accepts suffixes like k, M and G");
    printf("%-20s %s\n","-fsize <size>","max file size in bytes. 'size' accepts suffixes like k, M and G");
    printf("%-20s %s\n","-nonet","unshare network namespace, preventing all network access");
    printf("%-20s %s\n","-nopid","unshare pid namespace, preventing access to other processes");
    printf("%-20s %s\n","-noipc","unshare ipc namespace, preventing access shared memory, message queues and semaphores");
    printf("%-20s %s\n","-sec <config>","set security config, see --help-secure for more");
    printf("%-20s %s\n","-secure <config>","set security config, see --help-secure for more");
    printf("%-20s %s\n","-U <user>","Run as user");
    printf("%-20s %s\n","-user <user>","Run as user");
    printf("%-20s %s\n","-G <group>","Run as group");
    printf("%-20s %s\n","-group <group>","Run as group");
    printf("%-20s %s\n","-v","print version");
    printf("%-20s %s\n","-version","print version");
    printf("%-20s %s\n","--version","print version");
    printf("%-20s %s\n","-?","this help");
    printf("%-20s %s\n","-h","this help");
    printf("%-20s %s\n","-help","this help");
    printf("%-20s %s\n","--help","this help");
    printf("\n  The flags -d -s -i -k and -K are positional, lockfiles given before them on the command line will not be affected, those after will be.\n\n");
    printf("  -k and -K only work if the lock file owner has written their pid into the lockfile. Writing the pid is the default behavior, but -s prevents it, and getlock will also refuse to write into files bigger than %d bytes, as they are too big to only contain a Process ID\n\n", MAX_PIDFILE_SIZE);
    printf("  -i specifies an interval of time between runs. It uses the timestamp stored in lockfiles. It will not run until that timestamp, plus the interval, specifies a time that's in the past. It is a positional lock-file modifier, like -d. Thus it specifies lockfiles that will be used to store a timestamp, and which will be deleted if the run command fails (returns an exit status other than 0). In this way if the command fails, it can be immedialtely tried again. If it succeeds it will not run again until the interval expires.\n\n");
    printf("  -C is used in situations where you want to allow child processes to be launched without holding a lock. Normally, when running a program or script, one wants to hold a lock file until not only the program has exited, but also any child programs that it starts. However, if using a script to launch long-running processes it may not be desirable to hold onto the lock. The -C option sets all lockfiles to be 'Close on Exec', so that only the getlock process is holding those files locked, and when it exits the locks will be released, even if child processes are still running in the background.\n\n");
    printf("  -R restarts the launched program if it exits. This implies getlock will run forever unless the getlock process itself is terminated. The '-q' option allows specifying a 'dwell time' in milliseconds between restarts. Beware of setting this to zero, as a program that fails to startup will be launched over and over, fast as possible, burning up CPU resources.\n\n");
    printf("RETURN VALUE: 0 on lock, 1 if bad args on the command line, 3 if failed to get lock. Works with bash-style 'if'\n\n");
    printf("EXAMPLES:\n");
    printf("getlock /tmp/file1.lck /var/lock/file2.lck \"echo Got locks!\"\n");
    printf("	Lock /tmp/file1.lck and /var/lock/file2.lck, then run 'echo'\n\n");
    printf("getlock -k -w -g 5 /tmp/file1.lck \"echo Got locks!\"\n");
    printf("	Lock /tmp/file1.lck, killing any current owner if made to wait more than 5 secs\n\n");
    printf("getlock file1.lck -k file2.lck -d file3.lck \"echo Got locks!\"\n");
    printf("	Lock 3 files, killing the current owners of 'file2.lck' and 'file3.lck' and deleting 'file3.lck' when done\n\n");
    printf("getlock -w file1.lck \"echo Got locks!\"\n");
    printf("	wait till we get a lock on file1.lck (without -w will exit if cannot lock on first try)\n\n");
    printf("getlock -w file1.lck -s /home/colum/MyWork.txt \"vi /home/colum/MyWork.txt\"\n");
    printf("	wait till we get a lock on file1.lck and /home/colum/MyWork.txt, but DO NOT WRITE A PID INTO /home/colum/MyWork.txt\n\n");
    printf("getlock -N -k file1.lck file2.lck\n");
    printf("	Don't run a program, just kill owners of file1.lck and file2.lck\n\n");
    printf("getlock -N -w -b file1.lck\n");
    printf("	Don't run any program, just wait till we get a lock on file1.lck, then fork into background and hold the lock. Useful in shell scripts. e.g:\n");
    printf("		if getlock -b -N -w /tmp/file1.lck\n");
    printf("		then\n");
    printf("			echo \"got lock\"\n");
    printf("			DoStuff\n");
    printf("			kill `cat /tmp/file1.lck`\n");
    printf("		else\n");
    printf("		echo \"FAILED TO GET LOCK\"\n");
    printf("		fi\n\n");
    printf("getlock -N -w -t 10 -b file1.lck\n");
    printf("	As above, but only wait ten secs for lock, and only hold lock for ten secs.\n\n");
}


int ParseInterval(const char *Config)
{
    int val;
    char *ptr;

    val=strtol(Config, &ptr, 10);
    if (ptr)
    {
        switch (*ptr)
        {
        case 'm':
            val *= 60;
            break;
        case 'h':
            val *= 3600;
            break;
        case 'd':
            val *= 24 * 3600;
            break;
        }
    }

    LogEvent(LOG_DEBUG, "Interval %d", val);
    return(val);
}


void ParseArgs(int argc, char *argv[])
{
    ListNode *Curr;
    TLockFile *LF;
    CMDLINE *CMD;
    const char *arg;
    int i;

    memset(&Settings, 0, sizeof(Settings));
    Settings.RestartInterval=100; //100 milliseconds
    Settings.MaxPids=-1;
    Settings.UserMaxPids=-1;
    Settings.MaxFiles=-1;
    Settings.LockFiles=ListCreate();

    CMD=CommandLineParserCreate(argc, argv);

    arg=CommandLineNext(CMD);
    while(arg)
    {
        if (strcmp(arg,"-w")==0) Settings.Flags |= FLAG_WAIT;
        else if (strcmp(arg,"-a")==0) Settings.AbandonTime = atoi(CommandLineNext(CMD));
        else if (strcmp(arg,"-g")==0) Settings.GraceTime=atoi(CommandLineNext(CMD));
        else if (strcmp(arg,"-i")==0) Settings.Interval=ParseInterval(CommandLineNext(CMD));
        else if (strcmp(arg,"-q")==0) Settings.RestartInterval=atoi(CommandLineNext(CMD));
        else if (strcmp(arg,"-t")==0) Settings.Timeout=atoi(CommandLineNext(CMD));
        else if (strcmp(arg,"-b")==0) Settings.Flags |= FLAG_BACKGROUND;
        else if (strcmp(arg,"-d")==0) Settings.Flags |= FLAG_DELETE_FILE;
        else if (strcmp(arg,"-n")==0) Settings.Flags |= FLAG_NOHUP;
        else if (strcmp(arg,"-nohup")==0) Settings.Flags |= FLAG_NOHUP;
        else if (strcmp(arg,"-s")==0) Settings.Flags |= FLAG_NO_WRITE;
        else if (strcmp(arg,"-C")==0) Settings.Flags |= FLAG_CLOEXEC;
        else if (strcmp(arg,"-X")==0) Settings.Flags |= FLAG_RUN_ANYWAY;
        else if (strcmp(arg,"-N")==0) Settings.Flags |= FLAG_NO_PROGRAM;
        else if (strcmp(arg,"-D")==0) Settings.Flags |= FLAG_DEBUG;
        else if (strcmp(arg,"-S")==0) Settings.Flags |= FLAG_SILENT;
        else if (strcmp(arg,"-R")==0) Settings.Flags |= FLAG_RESTART;
        else if (strcmp(arg,"-k")==0) Settings.Flags |= FLAG_TERM_OWNER;
        else if (strcmp(arg,"-K")==0) Settings.Flags |= FLAG_KILL_OWNER;
        else if (strcmp(arg,"-L")==0) Settings.Flags |= FLAG_SYSLOG;
        else if (strcmp(arg,"-syslog")==0) Settings.Flags |= FLAG_SYSLOG;
        else if (strcmp(arg,"-sec")==0) Settings.Security=CopyStr(Settings.Security, CommandLineNext(CMD));
        else if (strcmp(arg,"-secure")==0) Settings.Security=CopyStr(Settings.Security, CommandLineNext(CMD));
        else if (strcmp(arg,"-su")==0) Settings.Flags |= FLAG_SU;
        else if (strcmp(arg,"-pids")==0) Settings.MaxPids=atoi(CommandLineNext(CMD));
        else if (strcmp(arg,"-upids")==0) Settings.UserMaxPids=atoi(CommandLineNext(CMD));
        else if (strcmp(arg,"-files")==0) Settings.MaxFiles=atoi(CommandLineNext(CMD));
        else if (strcmp(arg,"-mem")==0) Settings.MaxMemory=CopyStr(Settings.MaxMemory, CommandLineNext(CMD));
        else if (strcmp(arg,"-fsize")==0) Settings.MaxFileSize=CopyStr(Settings.MaxFileSize, CommandLineNext(CMD));
        else if (strcmp(arg,"-nonet")==0) Settings.Flags |= FLAG_NETNS;
        else if (strcmp(arg,"-P")==0) Settings.MaxPids=atoi(CommandLineNext(CMD));
        else if (strcmp(arg,"-F")==0) Settings.OnFail=CopyStr(Settings.OnFail,CommandLineNext(CMD));
        else if (strcmp(arg,"-U")==0) Settings.User=CopyStr(Settings.User, CommandLineNext(CMD));
        else if (strcmp(arg,"-user")==0) Settings.User=CopyStr(Settings.User, CommandLineNext(CMD));
        else if (strcmp(arg,"-G")==0) Settings.Group=CopyStr(Settings.Group, CommandLineNext(CMD));
        else if (strcmp(arg,"-group")==0) Settings.Group=CopyStr(Settings.Group, CommandLineNext(CMD));
        else if (
            (strcmp(arg,"-v")==0) ||
            (strcmp(arg,"-version")==0) ||
            (strcmp(arg,"--version")==0)
        )
        {
            printf("version: %s\n",Version);
            exit(0);
        }
        else if
        (
            (strcmp(arg,"-?")==0) ||
            (strcmp(arg,"-h")==0) ||
            (strcmp(arg,"-help")==0) ||
            (strcmp(arg,"--help")==0)
        )
        {
            PrintUsage();
            exit(0);
        }
        else if (strcmp(arg,"--help-secure")==0)
        {
            PrintSecureUsage();
            exit(0);
        }
 
        else if (
            (! (Settings.Flags & FLAG_NO_PROGRAM)) &&
            (CommandLinePeek(CMD) == NULL)
        ) Settings.Program=CopyStr(Settings.Program, arg);
        else
        {
            LF=(TLockFile *) calloc(1,sizeof(TLockFile));
            LF->Path=CopyStr(LF->Path, arg);
            LF->Flags=Settings.Flags;
            LF->fd=-1;
            Curr=ListAddItem(Settings.LockFiles,LF);
        }


    arg=CommandLineNext(CMD);
    }

}



int ReadLockFile(TLockFile *LF, pid_t *OwnerPid, time_t *LockTime)
{
    char *Tempstr=NULL;
    char *ptr;
    int result;
    int RetVal=FALSE;

    *LockTime=0;
    Tempstr=SetStrLen(Tempstr,40);
    result=lseek(LF->fd,0,SEEK_SET);
    if (result == 0)
    {
        result=read(LF->fd,Tempstr,40);
        if (result >= 0) RetVal=TRUE; //zero is permissiable if the lockfile is empty

        if (result > 0)
        {
            StrTrunc(Tempstr, result);
            *OwnerPid=strtol(Tempstr, &ptr, 10);
            if (StrValid(ptr))
            {
                *LockTime=DateStrToSecs("%Y%m%d%H%M%S", ptr, NULL);
            }
        }
    }

    Destroy(Tempstr);

    return(RetVal);
}


void CommitLock(TLockFile *LF)
{
    char *Tempstr=NULL;
    struct stat FStat;
    int result;

    fstat(LF->fd, &FStat);

    if (LF->Flags & FLAG_NO_WRITE)
    {
        LogEvent(LOG_DEBUG, "Not writing pid to file '%s'", LF->Path);
    }
    else if (FStat.st_size > MAX_PIDFILE_SIZE)
    {
        LogEvent(LOG_ERR, "Size of file '%s' is greater than %d bytes, probably not a lockfile! Not writing pid to file!", LF->Path, MAX_PIDFILE_SIZE);
    }
    else
    {
        Tempstr=FormatStr(Tempstr,"%d %s", getpid(), GetDateStr("%Y%m%d%H%M%S", NULL));
        if (ftruncate(LF->fd,0) != 0) LogEvent(LOG_ERR, "ERROR: ftruncate failed on file '%s'\n", LF->Path);
        lseek(LF->fd,0,SEEK_SET);
        result=write(LF->fd,Tempstr,StrLen(Tempstr));
        if (result != StrLen(Tempstr)) LogEvent(LOG_ERR, "Issue writing pid to file '%s' for program '%s", LF->Path, Settings.Program);
        LF->Flags |= FLAG_GOT_LOCK;
    }

    Destroy(Tempstr);
}


int DoLock(TLockFile *LF, int *OwnerPid)
{
    int result=LOCK_FAIL;
    char *Tempstr=NULL;
    int oflags=O_RDWR | O_CREAT;
    time_t LockTime, Now;

    if (LF->Flags & FLAG_GOT_LOCK) return(LOCK_OKAY);

    if (Settings.Flags & FLAG_CLOEXEC) oflags |= O_CLOEXEC;

    if (LF->fd==-1) LF->fd=open(LF->Path, oflags, 0666);
    if (LF->fd==-1)
    {
        LogEvent(LOG_ERR, "Couldn't open lock file: %s", LF->Path);
        return(LOCK_FAIL);
    }

    time(&Now);
    if (ReadLockFile(LF, OwnerPid, &LockTime))
    {
        if (lockf(LF->fd,F_TLOCK,0)==0)
        {
            result=LOCK_OKAY;

            if (Settings.Flags & FLAG_DEBUG) LogEvent(LOG_DEBUG, "GOT LOCK ON %s", LF->Path);

            if ((Settings.Interval > 0) && ((LockTime + Settings.Interval) > Now))
            {
                Tempstr=CopyStr(Tempstr, GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", LockTime, NULL));
                LogEvent(LOG_DEBUG, "Not yet time for next run. Previous: %s Next: %s", Tempstr, GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", LockTime + Settings.Interval, NULL) );
                result=LOCK_NOT_TIME;
            }
            else CommitLock(LF);
        }
        else LogEvent(LOG_INFO, "FILE: %s is locked by PID: %d",LF->Path,*OwnerPid);
    }
    else LogEvent(LOG_ERR, "ERROR: error reading lockfile at: %s",LF->Path);

    DestroyString(Tempstr);

    return(result);
}


int LockAll(ListNode *LockFiles, int Flags)
{
    ListNode *Curr;
    TLockFile *LF;
    int result=TRUE;
    pid_t OwnerPid=0;

    Curr=ListGetNext(LockFiles);
    while (Curr)
    {
        LF=(TLockFile *) Curr->Item;
        OwnerPid=0;
        if (DoLock(LF, &OwnerPid) != LOCK_OKAY)
        {
            result=FALSE;
            if (OwnerPid > 0)
            {
                if (LF->Flags & Flags & FLAG_TERM_OWNER)
                {
                    kill(OwnerPid,SIGTERM);
                    sleep(1);
                }

                if (LF->Flags & Flags & FLAG_KILL_OWNER) kill(OwnerPid,SIGKILL);
            }
        }
        Curr=ListGetNext(Curr);
    }
    return(result);
}


void DeleteFiles(ListNode *LockFiles, int RequiredFlag)
{
    ListNode *Curr;
    TLockFile *LF;

    Curr=ListGetNext(LockFiles);
    while (Curr)
    {
        LF=(TLockFile *) Curr->Item;
        if (LF->Flags & RequiredFlag) unlink(LF->Path);
        Curr=ListGetNext(Curr);
    }
}



void SetupProgram()
{
    char *Tempstr=NULL, *Config=NULL;


    Config=CopyStr(Config, "");
    if (Settings.MaxPids > -1) 
    {
	Tempstr=FormatStr(Tempstr, "nprocs=%d ", Settings.MaxPids);
	Config=CatStr(Config, Tempstr);
    }

    if (Settings.UserMaxPids > -1) 
    {
	Tempstr=FormatStr(Tempstr, "uprocs=%d ", Settings.UserMaxPids);
	Config=CatStr(Config, Tempstr);
    }


    if (Settings.MaxMemory) Config=MCatStr(Config, "mem=", Settings.MaxMemory, " ", NULL);

    if (Settings.MaxFiles > -1) 
    {
	Tempstr=FormatStr(Tempstr, "files=%d ", Settings.MaxFiles);
	Config=CatStr(Config, Tempstr);
    }
    if (Settings.MaxFileSize) Config=MCatStr(Config, "fsize=", Settings.MaxFileSize, " ", NULL);

    if (Settings.Flags & FLAG_NETNS) Config=CatStr(Config, "nonet ");

 
    if (StrValid(Settings.Security)) Config=MCatStr(Config, "security='", Settings.Security, "' ", NULL);



    if (StrLen(Settings.Group)) SwitchGroup(Settings.Group);
    if (StrLen(Settings.User)) SwitchUser(Settings.User);

    ProcessApplyConfig(Config);
    if (! (Settings.Flags & FLAG_SU)) GetlockNoNewPrivs();


    Destroy(Tempstr);
    Destroy(Config);
}


void RunProgram()
{
    int ExitCode;

    //assume we will restart the program
    while (1)
    {
        if (Settings.AbandonTime > 0)
        {
            setsid();
            signal(SIGALRM,SigHandler);
            alarm(Settings.AbandonTime);
        }

        LogEvent(LOG_INFO, "Run: %s", Settings.Program);

        SetupProgram();
        //Actually run program or script!
        ExitCode=system(Settings.Program);

        LogEvent(LOG_INFO, "Exited: %s ExitStatus: %d", Settings.Program, ExitCode);
        if (ExitCode != 0) DeleteFiles(Settings.LockFiles, FLAG_INTERVAL_FILE);

        //if 'restart' is not active, then exit loop
        if (! (Settings.Flags & FLAG_RESTART)) break;

        usleep(Settings.RestartInterval * 1000);
        LogEvent(LOG_DEBUG, "Restart: %s", Settings.Program);
    }
}


int ProcessLocks(int NotifyFD)
{
    time_t Start, Now=0;
    int GotLock=FALSE, Flags=0;
    int result, ExitCode;
    char *Tempstr=NULL;


    time(&Start);
    while (1)
    {
        time(&Now);
        if ((Now-Start) > Settings.GraceTime) Flags=FLAG_TERM_OWNER | FLAG_KILL_OWNER;
        GotLock=LockAll(Settings.LockFiles, Flags);
        if (GotLock) break;
        if (! (Settings.Flags & FLAG_WAIT)) break;


        if (
            (Settings.Timeout > 0) &&
            ((Now-Start) > Settings.Timeout)
        ) break;
        usleep(250000);
    }


    if (! GotLock)
    {
        LogEvent(LOG_ERR, "Couldn't lock all files for process '%s'", Settings.Program);
        if (StrValid(Settings.OnFail))
        {
            result=system(Settings.OnFail);
            if (result !=0) fprintf(stderr, "ERROR: failed to run 'onfail' script: %s  error was: %s\n", Settings.OnFail, strerror(errno));
        }
        if (NotifyFD > -1) result=write(NotifyFD,"N\n",2);
        if (! (Settings.Flags & FLAG_RUN_ANYWAY)) exit(RESULT_NOLOCK);
    }
    else if (NotifyFD > -1) result=write(NotifyFD,"Y\n",2);

    if (StrLen(Settings.Program)) RunProgram();
    else
    {
        if (Settings.Flags & FLAG_WAIT)
        {
            if (Settings.Timeout > 0) sleep(Settings.Timeout);
            else while (1) sleep(999999);
        }
    }

    if (GotLock) DeleteFiles(Settings.LockFiles, FLAG_DELETE_FILE);

    free(Tempstr);

    return(GotLock);
}




int main(int argc, char *argv[])
{
    int result, GotLock=FALSE;
    int pfds[2];
    char *Tempstr=NULL;

    ParseArgs(argc,argv);

    OpenSysLog();

    if (! ListSize(Settings.LockFiles))
    {
        LogEvent(LOG_ERR, "ERROR: No Lockfile Path Given! Use -? to get more help.");
        exit(RESULT_BADARGS);
    }

    if (! StrLen(Settings.Program))
    {
        if (Settings.Flags & FLAG_NO_PROGRAM)
        {
            LogEvent(LOG_INFO, "Running without executing client program, just locking files");
        }
        else
        {
            LogEvent(LOG_ERR, "ERROR: No Program Path Given! Use -? to get more help.");
            exit(RESULT_BADARGS);
        }
    }

    if (Settings.Flags & FLAG_NOHUP) signal(SIGHUP, SIG_IGN);

//if we are running in background (holding lock while letting parent process
//continue) then the parent can exit, the child will take it from here
    if (Settings.Flags & FLAG_BACKGROUND)
    {
        if (pipe(pfds) !=0)
        {
            Tempstr=CopyStr(Tempstr, strerror(errno));
            LogEvent(LOG_ERR, "ERROR: Failed to create pipe to subprocess: %s", Tempstr);
        }

        result=fork();
        if (result ==0)
        {
            GotLock=ProcessLocks(pfds[1]);
        }
        else
        {
            //read from child, it will inform us by sending 'Y' if it got the lock
            Tempstr=SetStrLen(Tempstr,10);
            result=read(pfds[0],Tempstr,10);
            if (result > 0)
            {
                StripTrailingWhitespace(Tempstr);
                if (strcmp(Tempstr,"Y")==0) GotLock=TRUE;
            }
        }
    }
    else GotLock=ProcessLocks(-1);


    Destroy(Tempstr);


    if (GotLock) return(RESULT_GOTLOCK);
    return(RESULT_NOLOCK);
}
