/*
 * make_wrap
 *
 *  make command logging warpper for recursive make
 *
 * usage:
 *     $ export MAKE_WRAP_LOG=/tmp/logfile
 *     $ make_wrap <make command arguments>...
 *     $ cat $MAKE_WRAP_LOG
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

void print_args(struct timeval *timev, pid_t ppid, pid_t pid, int exitcode,
		const char *cwd, int argc, char *argv[]);

char cwdbuf[1024];

int main(int argc, char *argv[])
{
    struct timeval stimev;
    struct timeval etimev;
    int r;
    pid_t thisp, subp, ppid, wp;
    const char *cmd;
    const char *cwd;

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
    print_args(&stimev, ppid, subp, -1, cwd, argc, argv);
    wp = wait(&r);
    gettimeofday(&etimev,NULL);
    if( wp < 0 ) { perror(argv[0]); exit(1); }
    if( subp != wp ) {
	fprintf(stderr, "make_wrap %s: pid %d %d unmatch\n", argv[0], subp, wp);
	exit(1);
    }
    if( WIFEXITED(r) ) {
	print_args(&etimev, ppid, subp, WEXITSTATUS(r), cwd, argc, argv);
	exit(WEXITSTATUS(r));
    } else if (WIFSIGNALED(r)) {
	fprintf(stderr, "make_wrap %s signal %d\n", argv[0], WTERMSIG(r));
    }
    exit(1);
}

/*
 * print format
 *  ppid,pid: time {start,exit(code)} : cwd/ argv[0] argv[1] ...
 *
 */
void print_args(struct timeval *timev, pid_t ppid, pid_t pid, int exitcode,
		const char *cwd, int argc, char *argv[])
{
    struct tm *rlc;
    int i;
    const char *logfile;
    FILE *fp;
    
    fp = stdout;
    logfile = getenv("MAKE_WRAP_LOG");
    if( logfile != NULL ) {
	fp = fopen(logfile, "a");
	if( fp == NULL )
	    fp = stdout;
    }
    rlc = gmtime(&(timev->tv_sec));
    fprintf(fp, "%3d,%3d: %02d:%02d:%02d.%03d ", ppid, pid,
	    rlc->tm_hour, rlc->tm_min, rlc->tm_sec,
	    timev->tv_usec/1000);
    if( exitcode < 0 )
	fprintf(fp, "start   : ");
    else
	fprintf(fp, "exit(%d) : ",exitcode);
    fprintf(fp,"%s/  ", cwd);
    for( i = 0; i < argc; i ++ )
	fprintf(fp, "%s%c", argv[i], (i + 1 < argc)?' ':'\n');
    if( fp != stdout )
	fclose(fp);
}
