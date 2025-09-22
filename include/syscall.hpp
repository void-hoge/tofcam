#pragma once

#include <fcntl.h>

namespace tofcam::syscall {

int ioctl(int fd, int request, void* arg);

int open(const char* path, int flags, mode_t mode);

int close(int fd);

}
