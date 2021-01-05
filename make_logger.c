/*
 * make_logger - make command logger for recursive make
 *
 * Copyright (c) 2012 ISHII Takeshi
 * Distributed under the MIT/X11 software license, see the accompanying
 * file license.txt or http://www.opensource.org/licenses/mit-license.php.
 *
 * repository URL = https://github.com/mtei/make_logger.git
 */
/*
 * usage:
 *    Example 1
 *     $ export MAKE_LOGGER_LOG=`pwd`/logfile.txt
 *     $ make_logger <make command arguments>...
 *     $ cat logfile.txt
 *
 *    Example 2 (realtime watching)
 *     $ export MAKE_LOGGER_LOG=`pwd`/logfile.txt
 *     $ touch $MAKE_LOGGER_LOG
 *     $ xterm -e tail -f $MAKE_LOGGER_LOG &
 *     $ make_logger <make command arguments>...
 *
 *  environment variable
 *     MAKE_LOGGER_LOG  -- log file name (full path)
 *     MAKE_LOGGER      -- make command name (default 'make')
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/wait.h>

const char *get_path_substr(const char *path);

void print_args(struct timeval *stimev, struct timeval *etimev,
		pid_t ppid, pid_t pid, int exitcode,int mlevel, const char *cwd,
		int argc, char *argv[]);

char filenamestr[FILENAME_MAX+1];

int main(int argc, char *argv[])
{
    struct timeval stimev;
    struct timeval etimev;
    int r, mlevel;
    pid_t thisp, subp, ppid, wp;
    const char *cmd;
    const char *cwd;
    const char *logfile;

    if( getenv("MAKE_LOGGER_LEVEL") != NULL ) {
	mlevel = atoi(getenv("MAKE_LOGGER_LEVEL"));
    } else {
	mlevel = 0;
	logfile = getenv("MAKE_LOGGER_LOG");
	if( logfile != NULL && logfile[0] != '/' ) {
	    getcwd(filenamestr, sizeof(filenamestr));
	    strcat(filenamestr, "/");  strcat(filenamestr, logfile);
	    setenv("MAKE_LOGGER_LOG", filenamestr, 1);
	}
#ifdef macOS
        setenv("MAKE", argv[0],0);
#endif
    }
    sprintf(filenamestr, "%d", mlevel+1);
    setenv("MAKE_LOGGER_LEVEL", filenamestr, 1);
    cmd = getenv("MAKE_LOGGER");
    if( cmd == NULL )
	cmd = "make";
    ppid = getppid();
    thisp = getpid();
    getcwd(filenamestr, sizeof(filenamestr));
    cwd = get_path_substr(filenamestr);
    gettimeofday(&stimev,NULL);

    subp = fork();
    if( subp == 0 ) {
	execvp(cmd, argv);
    }
    print_args(&stimev, &stimev, ppid, subp, -1, mlevel, cwd, argc, argv);
    wp = wait(&r);
    gettimeofday(&etimev,NULL);
    if( wp < 0 ) { perror(argv[0]); exit(1); }
    if( subp != wp ) {
	fprintf(stderr, "make_logger %s: pid %d %d unmatch\n", argv[0], subp, wp);
	exit(1);
    }
    if( WIFEXITED(r) ) {
	print_args(&stimev, &etimev, ppid, subp, WEXITSTATUS(r), mlevel, cwd, argc, argv);
	exit(WEXITSTATUS(r));
    } else if (WIFSIGNALED(r)) {
	fprintf(stderr, "make_logger %s signal %d\n", argv[0], WTERMSIG(r));
    }
    exit(1);
}

const char *get_path_substr(const char *path)
{
    const char *result;
    result = path + strlen(path) - 1;
    while( result > path && *result != '/' )
	result --;
    if( result > path ) result --;
    while( result >= path && *result != '/' )
	result --;
    if( result > path && *result == '/' ) result ++;
    return result;
}

/*
 * print format
 *  ppid,pid: clock time (nest) {start,exit(code)} : cwd/ argv[0] argv[1] ...
 *
 */
void print_args(struct timeval *stimev, struct timeval *etimev,
		pid_t ppid, pid_t pid, int exitcode,int mlevel, const char *cwd,
		int argc, char *argv[])
{
    struct tm *rlc;
    int i;
    const char *logfile;
    FILE *fp;
    struct timeval dtimev;
    
    fp = stdout;
    logfile = getenv("MAKE_LOGGER_LOG");
    if( logfile != NULL ) {
	fp = fopen(logfile, "a");
	if( fp == NULL )
	    fp = stdout;
    }
    rlc = localtime(&(etimev->tv_sec));
    fprintf(fp, "%3d,%3d: %02d:%02d:%02d.%03d ", ppid, pid,
	    rlc->tm_hour, rlc->tm_min, rlc->tm_sec,
	    etimev->tv_usec/1000);
    dtimev.tv_sec = etimev->tv_sec - stimev->tv_sec;
    dtimev.tv_usec = etimev->tv_usec - stimev->tv_usec;
    if( dtimev.tv_usec < 0 ) {
	dtimev.tv_usec += 1000000;
	dtimev.tv_sec --;
    }
    rlc = gmtime(&(dtimev.tv_sec));
    fprintf(fp, "%2d:%02d:%02d.%03d (%d) ",
	    rlc->tm_hour, rlc->tm_min, rlc->tm_sec,
	    dtimev.tv_usec/1000, mlevel);
    if( exitcode < 0 )
	fprintf(fp, "start   : ");
    else
	fprintf(fp, "exit(%d) : ",exitcode);
    fprintf(fp,"%s/  ", cwd);
    for( i = 0; i < argc; i ++ )
	fprintf(fp, "%s%s", argv[i], (i + 1 < argc)?" ":"");
    if( mlevel == 0 && exitcode >= 0 )
	fprintf(fp, "\n\n");
    else
	fprintf(fp, "\n");

    if( fp != stdout )
	fclose(fp);
}
