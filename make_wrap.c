/*
 * make_wrap
 *
 *  make command logging warpper for recursive make
 *
 * usage:
 *     $ export MAKE_WRAP_LOG=`pwd`/logfile.txt
 *     $ make_wrap <make command arguments>...
 *     $ cat logfile.txt
 *
 *  environment variable
 *     MAKE_WRAP_LOG  -- log file name
 *     MAKE_WRAP      -- make command name
 *
 *      Date      Design                     Log
 *  ------------  -------------------------  --------------------
 *   2012-08-11   isii@Harmony Systems       created
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

void print_args(struct timeval *stimev, struct timeval *etimev,
		pid_t ppid, pid_t pid, int exitcode,int mlevel, const char *cwd,
		int argc, char *argv[]);

char cwdbuf[1024];

int main(int argc, char *argv[])
{
    struct timeval stimev;
    struct timeval etimev;
    int r, mlevel;
    pid_t thisp, subp, ppid, wp;
    const char *cmd;
    const char *cwd;

    if( getenv("MAKE_WRAP_LEVEL") != NULL ) {
	mlevel = atoi(getenv("MAKE_WRAP_LEVEL"));
    } else
	mlevel = 0;
    sprintf(cwdbuf, "%d", mlevel+1);
    setenv("MAKE_WRAP_LEVEL", cwdbuf, 1);
    cmd = getenv("MAKE_WRAP");
    if( cmd == NULL )
	cmd = "make";
    ppid = getppid();
    thisp = getpid();
    getcwd(cwdbuf, sizeof(cwdbuf));
    cwd = strrchr(cwdbuf, '/')+1;
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
	fprintf(stderr, "make_wrap %s: pid %d %d unmatch\n", argv[0], subp, wp);
	exit(1);
    }
    if( WIFEXITED(r) ) {
	print_args(&stimev, &etimev, ppid, subp, WEXITSTATUS(r), mlevel, cwd, argc, argv);
	exit(WEXITSTATUS(r));
    } else if (WIFSIGNALED(r)) {
	fprintf(stderr, "make_wrap %s signal %d\n", argv[0], WTERMSIG(r));
    }
    exit(1);
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
    logfile = getenv("MAKE_WRAP_LOG");
    if( logfile != NULL ) {
	fp = fopen(logfile, "a");
	if( fp == NULL )
	    fp = stdout;
    }
    rlc = gmtime(&(etimev->tv_sec));
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
