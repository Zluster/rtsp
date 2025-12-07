#pragma once
#include <sys/syscall.h>
#include <fcntl.h>
#include <unistd.h>
namespace base
{
    pid_t gettid()
    {
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }
}