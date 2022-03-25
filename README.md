WHAT IS IT
==========

It's a program that locks files using kernel-locks, and then optionally runs a program. It can fork itself into the background while holding a lock, so that it can be used in shell scripts.


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
    cd movgrab-1.0
    ./configure
    make
    make install
```


USAGE
=====

In it's simplest form, getlock takes a list of one or more lockfiles, followed by a program to run (the last argument). It tries to lock the files using kernel 'lockf' locking. If it can lock them all, then it runs the program. It's default behavior is to exit if it can't get all the locks on the first try, using the -w flag (optionally with the -t flag) can pursuade it to try repeatedly to get the locks, either forever or until some timeout. 

By default getlock writes its Process ID into the lockfiles, though this can be suppressed with the -s option. It will not do this for files larger than 20 bytes though, as files that big are probably not lockfiles, but data files that it shouldn't overwrite!

getlock can be used within shell scripts by using the -b argument to make it fork into the background once it has the lock, the -w argument to make it hold the lock once it's got it, and the -N argument to tell it not to run a program, just to lock the files and hold the lock. When the shell script exits, it should get the pid of the getlock process from one of the lockfiles, and then kill the getlock to release the lock.


COMMAND
=======

```
getlock [options] LockFilePath [LockFilePath] ... Program


-a <n>	 Abandon running program/script after 'n' seconds, kill all subprocesses and exit, useful if script 'hangs'
-b       Fork into background when got lock
-d       delete lockfile when done
-n       'nohup', ignore SIGHUP
-nohup   ignore SIGHUP
-w       wait for lock (default is do not wait)
-t <n>   timeout <n> secs on wait (default is wait forever if -w used)
-s       SAFE MODE, do not write pid into file
-C       Close on exec, this prevents lockfiles being inherited by the child process.
-D       Debug. Print what you're doing.
-S       Silent. No error messages.
-F       Run specified program if can't get lock
-N       NO PROGRAM, just lock files, use with -w
-X       execute program even if lock fails!
-g <n>   Gracetime, wait <n> secs before doing kills
-k       Send SIGTERM to lockfile owner
-K       Send SIGKILL (kill -9) to lockfile owner
-U <user> Run as user
-user <user> Run as user
-G <group> Run as group
-group <group> Run as group
-v       print version
-version print version
-?       this help
```

The flags -d -s -k and -K are positional, lockfiles given before them on the command line will not be affected, those after will be.

-k and -K only work if the lock file owner has written their pid into the lockfile. Writing the pid is the default behavior, but -s prevents it, and getlock will also refuse to write into files bigger than 20 bytes, as they are too big to only contain a Process ID


RUN INTERVAL
============

The `-i` option prevents a command from running until a timeout has expired. It uses the date stored in the lock file, along with the interval specified on the command-line. The command will only run when the time goes past this combined value. However, the run command MUST RETURN AN EXIT STATUS OF 0. For any other exit status it is assumed the command failed, and lockfiles are deleted (provided that the -d option was specified before they were listed on the command-line).

This allows trying a command repeatedly, until it succeeds, and then not trying it again until a timeout has expired. This might be used for backups that process when a network comes up, but should only run once a day.

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

