
#include "libUseful-2.0/libUseful.h"

#define FLAG_WAIT 1
#define FLAG_RUN_ANYWAY 2
#define FLAG_DELETE_FILE 4
#define FLAG_TERM_OWNER 8
#define FLAG_KILL_OWNER 16
#define FLAG_NO_WRITE 32
#define FLAG_GOT_LOCK 64
#define FLAG_NO_PROGRAM 128
#define FLAG_BACKGROUND 512
#define FLAG_SILENT 1024
#define FLAG_DEBUG 2048
#define FLAG_NOHUP 4096

#define RESULT_GOTLOCK 0
#define RESULT_BADARGS 1
#define RESULT_NOOPEN  2
#define RESULT_NOLOCK  3

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
ListNode *LockFiles;
char *Program;
char *OnFail;
char *User;
char *Group;
} TSettings;

TSettings Settings;

char *Version="0.3";

void SigHandler(int sig)
{
if (sig==SIGALRM)
{
kill(0,SIGKILL);
exit(1);
}

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
	printf("%-8s %s\n","-a <seconds>","Abandon running task, killall subprocesses and exit after <seconds>");
	printf("%-8s %s\n","-b","Fork into background when got lock");
	printf("%-8s %s\n","-d","delete lockfile when done");
	printf("%-8s %s\n","-n","'nohup', ignore SIGHUP");
	printf("%-8s %s\n","-nohup","ignore SIGHUP");
	printf("%-8s %s\n","-w","wait for lock (default is do not wait)");
	printf("%-8s %s\n","-t <n>","timeout <n> secs on wait (default is wait forever if -w used)");
	printf("%-8s %s\n","-s","SAFE MODE, do not write pid into file");
	printf("%-8s %s\n","-D","Debug. Print what you're doing.");
	printf("%-8s %s\n","-S","Silent. No error messages.");
	printf("%-8s %s\n","-F","Run specified program if can't get lock");
	printf("%-8s %s\n","-N","NO PROGRAM, just lock files, use with -w");
	printf("%-8s %s\n","-X","execute program even if lock fails!");
	printf("%-8s %s\n","-g <n>","Gracetime, wait <n> secs before doing kills");
	printf("%-8s %s\n","-k","Send SIGTERM to lockfile owner");
	printf("%-8s %s\n","-K","Send SIGKILL (kill -9) to lockfile owner");
	printf("%-8s %s\n","-U <user>","Run as user");
	printf("%-8s %s\n","-user <user>","Run as user");
	printf("%-8s %s\n","-G <group>","Run as group");
	printf("%-8s %s\n","-group <group>","Run as group");
	printf("%-8s %s\n","-v","print version");
	printf("%-8s %s\n","-version","print version");
	printf("%-8s %s\n","-?","this help");
	printf("\n	The flags -d -s -k and -K are positional, lockfiles given before them on the command line will not be affected, those after will be.\n");
	printf("	-k and -K only work if the lock file owner has written their pid into the lockfile. Writing the pid is the default behavior, but -s prevents it, and getlock will also refuse to write into files bigger than 20 bytes, as they are too big to only contain a Process ID\n\n");
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

void ParseArgs(int argc, char *argv[])
{
ListNode *Curr;
TLockFile *LF;
int i;

memset(&Settings, 0, sizeof(Settings));
Settings.LockFiles=ListCreate();

for (i=1; i < argc; i++)
{
	if (! StrLen(argv[i])) continue;
	if (strcmp(argv[i],"-w")==0) Settings.Flags |= FLAG_WAIT;
	else if (strcmp(argv[i],"-a")==0) Settings.AbandonTime = atoi(argv[++i]);
	else if (strcmp(argv[i],"-b")==0) Settings.Flags |= FLAG_BACKGROUND;
	else if (strcmp(argv[i],"-d")==0) Settings.Flags |= FLAG_DELETE_FILE;
	else if (strcmp(argv[i],"-n")==0) Settings.Flags |= FLAG_NOHUP;
	else if (strcmp(argv[i],"-nohup")==0) Settings.Flags |= FLAG_NOHUP;
	else if (strcmp(argv[i],"-s")==0) Settings.Flags |= FLAG_NO_WRITE;
	else if (strcmp(argv[i],"-X")==0) Settings.Flags |= FLAG_RUN_ANYWAY;
	else if (strcmp(argv[i],"-N")==0) Settings.Flags |= FLAG_NO_PROGRAM;
	else if (strcmp(argv[i],"-D")==0) Settings.Flags |= FLAG_DEBUG;
	else if (strcmp(argv[i],"-S")==0) Settings.Flags |= FLAG_SILENT;
	else if (strcmp(argv[i],"-g")==0) Settings.GraceTime=atoi(argv[++i]);
	else if (strcmp(argv[i],"-k")==0) Settings.Flags |= FLAG_TERM_OWNER;
	else if (strcmp(argv[i],"-K")==0) Settings.Flags |= FLAG_KILL_OWNER;
	else if (strcmp(argv[i],"-F")==0) Settings.OnFail=CopyStr(Settings.OnFail,argv[++i]);
	else if (strcmp(argv[i],"-U")==0) Settings.User=CopyStr(Settings.User, argv[++i]);
	else if (strcmp(argv[i],"-user")==0) Settings.User=CopyStr(Settings.User, argv[++i]);
	else if (strcmp(argv[i],"-G")==0) Settings.Group=CopyStr(Settings.Group, argv[++i]);
	else if (strcmp(argv[i],"-group")==0) Settings.Group=CopyStr(Settings.Group, argv[++i]);
	else if (strcmp(argv[i],"-t")==0) Settings.Timeout=atoi(argv[++i]);
	else if (strcmp(argv[i],"-v")==0) 
	{
		printf("version: %s\n",Version);
		exit(0);
	}
	else if (strcmp(argv[i],"-version")==0)
	{
		 printf("version: %s\n",Version);
		exit(0);
	}
	else if (
		(strcmp(argv[i],"-?")==0) ||
		(strcmp(argv[i],"-help")==0) ||
		(strcmp(argv[i],"--help")==0)
		)
	{
		PrintUsage();
		exit(0);
	}
	else if (
					(! (Settings.Flags & FLAG_NO_PROGRAM)) && 
					(i==(argc-1))
				) Settings.Program=CopyStr(Settings.Program, argv[i]);
	else
	{
		LF=(TLockFile *) calloc(1,sizeof(TLockFile));
		LF->Path=CopyStr(LF->Path,argv[i]);
		LF->Flags=Settings.Flags;
		LF->fd=-1;
		Curr=ListAddItem(Settings.LockFiles,LF);
	}

}

}


int DoLock(TLockFile *LF, int *OwnerPid)
{
int result=FALSE;
char *Tempstr=NULL;
struct stat FStat;

if (LF->Flags & FLAG_GOT_LOCK) return(TRUE);

if (LF->fd==-1) LF->fd=open(LF->Path,O_RDWR | O_CREAT,0666);
if (LF->fd==-1)
{
  if (! (Settings.Flags & FLAG_SILENT)) printf("Couldn't open lock file\n");
  return(FALSE);
}

if (lockf(LF->fd,F_TLOCK,0)==0) 
{
	result=TRUE;
	fstat(LF->fd, &FStat);

	if (Settings.Flags & FLAG_DEBUG) printf("GOT LOCK ON %s\n",LF->Path);
	if (LF->Flags & FLAG_NO_WRITE) 
	{
	if (Settings.Flags & FLAG_DEBUG) printf("Not writing pid to file.\n");
	}
	else if (FStat.st_size > 20)
	{
		if (! (Settings.Flags & FLAG_SILENT)) printf("File size is greater than 20 bytes, probably not a lockfile! Not writing pid to file!\n");
	}
	else
	{
		Tempstr=SetStrLen(Tempstr,40);
		sprintf(Tempstr,"%d",getpid());
		ftruncate(LF->fd,0);
		lseek(LF->fd,0,SEEK_SET);
		write(LF->fd,Tempstr,StrLen(Tempstr));
		LF->Flags |= FLAG_GOT_LOCK;
	}
}
else
{
		Tempstr=SetStrLen(Tempstr,40);
		lseek(LF->fd,0,SEEK_SET);
		read(LF->fd,Tempstr,40);
		*OwnerPid=atoi(Tempstr);
 		if (Settings.Flags & FLAG_DEBUG) printf("FILE: %s is locked by PID: %d\n",LF->Path,*OwnerPid);
}


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
	if (! DoLock(LF, &OwnerPid)) 
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


void DeleteFiles(ListNode *LockFiles)
{
ListNode *Curr;
TLockFile *LF;

Curr=ListGetNext(LockFiles);
while (Curr)
{
	LF=(TLockFile *) Curr->Item;
	if (LF->Flags & FLAG_DELETE_FILE) unlink(LF->Path);
	Curr=ListGetNext(Curr);
}
}



int ProcessLocks(int NotifyFD)
{
time_t Start, Now=0;
int GotLock=FALSE, Flags=0;
int result;
char *Tempstr=NULL;


time(&Start);
while (1)
{
	time(&Now);
	if	((Now-Start) > Settings.GraceTime) Flags=FLAG_TERM_OWNER | FLAG_KILL_OWNER;
	GotLock=LockAll(Settings.LockFiles,Flags);
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
 	 if (! (Settings.Flags & FLAG_SILENT)) printf("Couldn't lock file\n");
	 if (StrLen(Settings.OnFail)) system(Settings.OnFail);
	 if (NotifyFD > -1) write(NotifyFD,"N\n",2);
 	 if (! (Settings.Flags & FLAG_RUN_ANYWAY)) exit(RESULT_NOLOCK);
}
else if (NotifyFD > -1) write(NotifyFD,"Y\n",2);

if (StrLen(Settings.Program)) 
{
	if (Settings.AbandonTime > 0) 
	{
		setsid();
		signal(SIGALRM,SigHandler);
		alarm(Settings.AbandonTime);
	}

	//Actually run program or script!
	system(Settings.Program);
}
else 
{
	if (Settings.Flags & FLAG_WAIT)
	{
		if (Settings.Timeout > 0) sleep(Settings.Timeout);
		else while (1) sleep(999999);
	}
}

if (GotLock) DeleteFiles(Settings.LockFiles);

free(Tempstr);

return(GotLock);
}

main(int argc, char *argv[])
{
int result, GotLock=FALSE;
char *Tempstr=NULL;
int pfds[2];

ParseArgs(argc,argv);

if (StrLen(Settings.Group)) SwitchGroup(Settings.Group);
if (StrLen(Settings.User)) SwitchGroup(Settings.User);


if (! ListSize(Settings.LockFiles)) 
{
	if (! (Settings.Flags & FLAG_SILENT)) printf("ERROR: No Lockfile Path Given! Use -? to get more help.\n");
	exit(RESULT_BADARGS);
}

if (! StrLen(Settings.Program)) 
{
	if (Settings.Flags & FLAG_NO_PROGRAM) 
	{
		if (! (Settings.Flags & FLAG_SILENT))
		{
			 printf("Running without executing client program, just locking files\n");
		}
	}
	else
	{
	if (! (Settings.Flags & FLAG_SILENT) ) printf("ERROR: No Program Path Given! Use -? to get more help.\n");
	exit(RESULT_BADARGS);
	}
}

if (Settings.Flags & FLAG_NOHUP) signal(SIGHUP, SIG_IGN);

//if we are running in background (holding lock while letting parent process
//continue) then the parent can exit, the child will take it from here
if (Settings.Flags & FLAG_BACKGROUND)
{
	pipe(pfds);
 	result=fork();
	if (result ==0) 
	{
		GotLock=ProcessLocks(pfds[1]);
	}
	else 
	{
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

if (GotLock) exit(RESULT_GOTLOCK);
exit(RESULT_NOLOCK);
}
