WHAT IS IT
==========

It's a program that locks files using kernel-locks, and then optionally runs a program. The main use of this is to prevent two copies of a program running at the same time, or to prevent programs from accessing the same resource at the same time. It can fork itself into the background while holding a lock, so that it can be used in shell scripts. It has a lot of options to seize locks, only run a program after a certain interval, lock multiple files, etc, etc.


AUTHOR
======

getlock and libUseful are (C) 2010 Colum Paget. They are released under the GPL so you may do anything with them that the GPL allows.

Email: colums.projects@gmail.com


DISCLAIMER
==========

This is free software. It comes with no guarentees and I take no responsiblity if it makes your computer explode or opens a portal to the demon dimensions, or does anything.


INSTALL
=======

After unpacking the tar-ball:

```
    tar -zxf getlock-0.1.tar.gz
```

Do the usual './configure; make; make install'

```
    cd getlock-2.0
    ./configure
    make
    make install
```


USAGE
=====

In its simplest form, getlock takes a list of one or more lockfiles, followed by a program to run (the last argument). It tries to lock the files using kernel 'lockf' locking. If it can lock them all, then it runs the program. It's default behavior is to exit if it can't get all the locks on the first try, using the -w flag (optionally with the -t flag) can pursuade it to try repeatedly to get the locks, either forever or until some timeout. 

By default getlock writes its Process ID into the lockfiles, though this can be suppressed with the -s option. It will not do this for files larger than 25 bytes though, as files that big are probably not lockfiles, but data files that it shouldn't overwrite!

getlock can be used within shell scripts by using the -b argument to make it fork into the background once it has the lock, the -w argument to make it hold the lock once it's got it, and the -N argument to tell it not to run a program, just to lock the files and hold the lock. When the shell script exits, it should get the pid of the getlock process from one of the lockfiles, and then kill the getlock to release the lock.


COMMAND
=======

```
	getlock [options] LockFilePath [LockFilePath] ... Program

-a <seconds> Abandon running task, killall subprocesses and exit after <seconds>
-b       Fork into background when got lock
-d       delete lockfile when done
-n       'nohup', ignore SIGHUP
-nohup   ignore SIGHUP
-w       wait for lock (default is do not wait)
-t <n>   timeout <n> secs on wait (default is wait forever if -w used)
-i <n>   interval between runs. Defauts to seconds, suffix with 'm' for minutes, 'h' for hours, 'd' for days.
-s       SAFE MODE, do not write pid into file
-C       Close on exec, this prevents lockfiles being inherited by the child process.
-D       Debug. Print what you're doing.
-R       Restart program if it exits.
-q <n>   Restart interval. Milliseconds delay between restart attempts.
-S       Silent. No error/info/debug messages.
-L       Send error/info/debug messages to syslog.
-syslog  Send error/info/debug messages to syslog.
-F       Run specified program if can't get lock
-N       NO PROGRAM, just lock files, use with -w
-X       execute program even if lock fails!
-g <n>   Gracetime, wait <n> secs before doing kills
-k       Send SIGTERM to lockfile owner
-K       Send SIGKILL (kill -9) to lockfile owner
-P       'no new privs' do not allow process escalate privilege
-nosu    'no new privs' do not allow process escalate privilege
-nopriv  'no new privs' do not allow process escalate privilege
-U <user> Run as user
-user <user> Run as user
-G <group> Run as group
-group <group> Run as group
-v       print version
-version print version
--version print version
-?       this help
-h       this help
-help    this help
--help   this help

  The flags -d -s -i -k and -K are positional, lockfiles given before them on the command line will not be affected, those after will be.

  -k and -K only work if the lock file owner has written their pid into the lockfile. Writing the pid is the default behavior, but -s prevents it, and getlock will also refuse to write into files bigger than 25 bytes, as they are too big to only contain a Process ID

  -i specifies an interval of time between runs. It uses the timestamp stored in lockfiles. It will not run until that timestamp, plus the interval, specifies a time that's in the past. It is a positional lock-file modifier, like -d. Thus it specifies lockfiles that will be used to store a timestamp, and which will be deleted if the run command fails (returns an exit status other than 0). In this way if the command fails, it can be immedialtely tried again. If it succeeds it will not run again until the interval expires.

  -C is used in situations where you want to allow child processes to be launched without holding a lock. Normally, when running a program or script, one wants to hold a lock file until not only the program has exited, but also any child programs that it starts. However, if using a script to launch long-running processes it may not be desirable to hold onto the lock. The -C option sets all lockfiles to be 'Close on Exec', so that only the getlock process is holding those files locked, and when it exits the locks will be released, even if child processes are still running in the background.

  -R restarts the launched program if it exits. This implies getlock will run forever unless the getlock process itself is terminated. The '-q' option allows specifying a 'dwell time' in milliseconds between restarts. Beware of setting this to zero, as a program that fails to startup will be launched over and over, fast as possible, burning up CPU resources.

RETURN VALUE: 0 on lock, 1 if bad args on the command line, 3 if failed to get lock. Works with bash-style 'if'

EXAMPLES:
getlock /tmp/file1.lck /var/lock/file2.lck "echo Got locks!"
	Lock /tmp/file1.lck and /var/lock/file2.lck, then run 'echo'

getlock -k -w -g 5 /tmp/file1.lck "echo Got locks!"
	Lock /tmp/file1.lck, killing any current owner if made to wait more than 5 secs

getlock file1.lck -k file2.lck -d file3.lck "echo Got locks!"
	Lock 3 files, killing the current owners of 'file2.lck' and 'file3.lck' and deleting 'file3.lck' when done

getlock -w file1.lck "echo Got locks!"
	wait till we get a lock on file1.lck (without -w will exit if cannot lock on first try)

getlock -w file1.lck -s /home/colum/MyWork.txt "vi /home/colum/MyWork.txt"
	wait till we get a lock on file1.lck and /home/colum/MyWork.txt, but DO NOT WRITE A PID INTO /home/colum/MyWork.txt

getlock -N -k file1.lck file2.lck
	Don't run a program, just kill owners of file1.lck and file2.lck

getlock -N -w -b file1.lck
	Don't run any program, just wait till we get a lock on file1.lck, then fork into background and hold the lock. Useful in shell scripts. e.g:
		if getlock -b -N -w /tmp/file1.lck
		then
			echo "got lock"
			DoStuff
			kill `cat /tmp/file1.lck`
		else
		echo "FAILED TO GET LOCK"
		fi

getlock -N -w -t 10 -b file1.lck
	As above, but only wait ten secs for lock, and only hold lock for ten secs.


```


Note that if you wish to pass arguments to the program that getlock runs, then you must quote it like this: 

```
getlock /var/lock/lockfile.lck "/usr/bin/md5sum /tmp/*"
```

The flags -d -s -i -k and -K are positional, lockfiles given before them on the command line will not be affected, those after will be.


LOCKFILES
=========

Getlock writes it's Process ID (PID) to the lockfile and also a timestamp. However, if the file already exists, and is larger than 25 bytes long, then getlock will refuse to write to it, as the file clearly contains more data than just a PID and timestamp. Lockfiles are locked with kernel locking mechanisms that are released automatically when getlock and all it's child processes exit.


CHILD PROCESS LOCKS
===================

The `-C` option  is used in situations where you want to allow child processes to be launched without holding a lock. Normally all programs launched by 'getlock' inherit a lock on the file, so that even if getlock exits, any child process should still be holding the lock. When running a program or script one usually wants to hold a lock file until not only the parent program has exited, but also any child programs that it starts. However, if using a script to launch long-running processes it may not be desirable to hold onto the lock. The -C option sets all lockfiles to be 'Close on Exec', so that only the getlock process is holding those files locked, and when it exits the locks will be released, even if child processes are still running.


KILLING LOCKFILE HOLDERS
========================

The `-k` and `-K` options allow getlock to kill any other programs holding a lock on a lockfile, by sending 'kill' to the existing process. These options only work if the lock file owner has written their pid into a file. This file can be any pidfile, although getlock will then write its own pid to that file and lock it. For getlock writing the pid is the default behavior, but `-s` prevents it.


RUN INTERVAL
============

The `-i` option prevents a command from running until a timeout has expired. It accepts a suffix of 'm' for minutes, 'h' for hours or 'd' for days. Without a suffix, it assumes the value is seconds. It uses the date stored in the lock file, along with the interval specified on the command-line. getlock will only run programs when the time goes past this combined value. 

However, the program run by getlock MUST RETURN AN EXIT STATUS OF 0. For any other exit status it is assumed the command failed, and the wait interval is cancelled by deleting any lockfiles that the '-i' option was specified before. In this manner `-i` is positional like `-d`, and specifies lockfiles to be deleted if the run command fails. 


This allows trying a command repeatedly, until it succeeds, and then not trying it again until a timeout has expired. This might be used for backups that process when a network comes up, but should only run once a day.


PROGRAM RESTART
===============

The `-R` option restarts a program if it exits. This implies that a getlock launched with this configuration will run forever, unless the getlock process itself is terminated. The '-q' option allows specifying a number of milliseconds 'dwell time' before launching the process again. The default dwell time is 100 milliseconds. Beware of setting this to '0', as a program that fails to run will attempt restart over and over again, burning up CPU resources.


"NO NEW PRIVS"
==============

The `-nosu` `-nopriv` and `-P` options are equivalent and all set a flag in the os kernel that prevents any attempt to escalate privildges. This is only supported on linux. If getlock is run with these options then no use of 'su' 'sudo' or raising privildges via setuid will work.


RETURN VALUE 
============

0 on lock, 1 if bad args on the command line, 3 if failed to get lock. Works with bash-style 'if'



EXAMPLES
========


```
getlock /tmp/file1.lck /var/lock/file2.lck "echo Got locks!"
```
	Lock /tmp/file1.lck and /var/lock/file2.lck, then run 'echo'

```
getlock -k -w -g 5 /tmp/file1.lck "echo Got locks!"
```
	Lock /tmp/file1.lck, killing any current owner if made to wait more than 5 secs

```
getlock file1.lck -k file2.lck -d file3.lck "echo Got locks!"
```
	Lock 3 files, killing the current owners of 'file2.lck' and 'file3.lck' and deleting 'file3.lck' when done

```
getlock -w file1.lck "echo Got locks!"
```
	wait till we get a lock on file1.lck (without -w will exit if cannot lock on first try)

```
getlock -w file1.lck -s /home/colum/MyWork.txt "vi /home/colum/MyWork.txt"
```
	wait till we get a lock on file1.lck and /home/colum/MyWork.txt, but DO NOT WRITE A PID INTO /home/colum/MyWork.txt

```
getlock -N -k file1.lck file2.lck
```
	Don't run a program, just kill owners of file1.lck and file2.lck

```
getlock -N -w -b file1.lck
```
	Don't run any program, just wait till we get a lock on file1.lck, then fork into background and hold the lock. Useful in shell scripts. e.g:

```
		if getlock -b -N -w /tmp/file1.lck
		then
			echo "got lock"
			DoStuff
			kill `cat /tmp/file1.lck`
		else
		echo "FAILED TO GET LOCK"
		fi
```

```
getlock -N -w -t 10 -b file1.lck
```
	As above, but only wait ten secs for lock, and only hold lock for ten secs.


```
getlock -i 5d /var/locks/backup.lck "tar -zcO mydir | ssh backuphost 'tar -zxf -'"
```
	Only run the command if we get a lock on /var/locks/backup.lck and command has not run for 5 days. Note the use of the `-d` flag to ensure that, if the command fails to run (ssh returns something other than 0) the lockfile will be deleted so the command can be tried again and again until it suceeds.




