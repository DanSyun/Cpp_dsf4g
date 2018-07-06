#pragma once

#include <signal.h>
#include <sys/stat.h>

inline void Daemonize()
{
    int pid;
    if ((pid = fork() != 0))
        exit(0);

    setsid();
    umask(0);

    signal(SIGHUP, SIG_IGN); // 
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN); // 

    if ((pid = fork() != 0))
        exit(0);
}

