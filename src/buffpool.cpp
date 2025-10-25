#include <buffpool.hpp>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <syscall.hpp>
#include <system_error>
#include <unistd.h>

namespace tofcam {

MmapBufferPool::MmapBufferPool(const int fd, const uint32_t num_buffers) {
    for (uint32_t i = 0; i < num_buffers; i++) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (syscall::ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_QUERYBUF failed.");
        }
        auto addr = syscall::mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (addr == MAP_FAILED) {
            throw std::system_error(errno, std::generic_category(), "mmap failed.");
        }
        this->buffers.emplace_back(addr, buf.length);
    }
}

void* MmapBufferPool::sync_start(const uint32_t index) {
    return this->buffers[index].first;
}

int MmapBufferPool::sync_end(const uint32_t) {
    return 0;
}

DmaBufferPool::DmaBufferPool(const char* allocator, const uint32_t num_buffers, const uint32_t length) {
    this->fd = syscall::open(allocator, O_RDWR | O_CLOEXEC, 0);
    if (this->fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open a contiguous memory allocator.");
    }
    for (uint32_t i = 0; i < num_buffers; i++) {
        struct dma_heap_allocation_data alloc = {};
        alloc.len = length;
        alloc.fd = 0;
        alloc.fd_flags = O_RDWR | O_CLOEXEC;
        alloc.heap_flags = 0;
        if (syscall::ioctl(this->fd, DMA_HEAP_IOCTL_ALLOC, &alloc) < 0) {
            throw std::system_error(errno, std::generic_category(), "ioctl DMA_HEAP_IOCTL_ALLOC failed.");
        }
        auto addr = syscall::mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, alloc.fd, 0);
        if (addr == MAP_FAILED) {
            throw std::system_error(errno, std::generic_category(), "mmap failed.");
        }
        this->buffers.emplace_back(addr, length, alloc.fd);
    }
}

void* DmaBufferPool::sync_start(const uint32_t index) {
    struct dma_buf_sync flags = {};
    flags.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
    const auto& [addr, length, fd] = this->buffers[index];
    if (syscall::ioctl(fd, DMA_BUF_IOCTL_SYNC, &flags) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl DMA_BUF_IOCTL_SYNC failed.");
    }
    return addr;
}

int DmaBufferPool::sync_end(const uint32_t index) {
    struct dma_buf_sync flags = {};
    flags.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
    const auto& [addr, length, fd] = this->buffers[index];
    if (syscall::ioctl(fd, DMA_BUF_IOCTL_SYNC, &flags) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl DMA_BUF_IOCTL_SYNC failed.");
    }
    return fd;
}

} // namespace tofcam
