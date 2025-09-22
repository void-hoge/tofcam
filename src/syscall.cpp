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

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
	return ::mmap(addr, length, prot, flags, fd, offset);
}

int munmap(void* addr, size_t length) {
	return ::munmap(addr, length);
}


} // namespace tofcam::syscall
