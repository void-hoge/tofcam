#include <syscall.hpp>
#include <cerrno>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace tofcam::syscall {

int ioctl(int fd, int request, void* arg) {
	int r;
	do {
		r = ::ioctl(fd, request, arg);
	} while (r == -1 && (errno == EINTR || errno == EAGAIN));
	return r;
}

int open(const char* path, int flags, mode_t mode) {
	return ::open(path, flags, mode);
}

int close(int fd) {
	return ::close(fd);
}

} // namespace tofcam::syscall
