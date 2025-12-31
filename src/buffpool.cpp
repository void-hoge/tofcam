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
    this->buffers.reserve(num_buffers);
    try {
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
    } catch (...) {
        for (auto& [addr, len] : this->buffers) {
            if (addr) {
                syscall::munmap(addr, len);
            }
        }
        this->buffers.clear();
        throw;
    }
}

MmapBufferPool::~MmapBufferPool() noexcept {
    for (auto& [addr, len] : this->buffers) {
        if (addr) {
            syscall::munmap(addr, len);
        }
    }
    this->buffers.clear();
}

MmapBufferPool::MmapBufferPool(MmapBufferPool&& other) noexcept : buffers(std::move(other.buffers)) {
    other.buffers.clear();
}

MmapBufferPool& MmapBufferPool::operator=(MmapBufferPool&& other) noexcept {
    if (this != &other) {
        for (auto& [addr, len] : buffers) {
            if (addr) {
                syscall::munmap(addr, len);
            }
        }
        this->buffers.clear();
        this->buffers = std::move(other.buffers);
        other.buffers.clear();
    }
    return *this;
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
    try {
        for (uint32_t i = 0; i < num_buffers; i++) {
            struct dma_heap_allocation_data alloc = {};
            alloc.len = length;
            alloc.fd = 0;
            alloc.fd_flags = O_RDWR | O_CLOEXEC;
            alloc.heap_flags = 0;
            if (syscall::ioctl(this->fd, DMA_HEAP_IOCTL_ALLOC, &alloc) < 0) {
                fprintf(stderr, "%d\n", errno);
                throw std::system_error(errno, std::generic_category(), "ioctl DMA_HEAP_IOCTL_ALLOC failed.");
            }
            auto addr = syscall::mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, alloc.fd, 0);
            if (addr == MAP_FAILED) {
                const int e = errno;
                syscall::close(alloc.fd);
                throw std::system_error(e, std::generic_category(), "mmap failed.");
            }
            this->buffers.emplace_back(addr, length, alloc.fd);
        }
    } catch (...) {
        for (auto& [addr, len, bfd] : this->buffers) {
            if (addr) {
                syscall::munmap(addr, len);
            }
            if (fd >= 0) {
                syscall::close(bfd);
            }
        }
        this->buffers.clear();
        if (this->fd >= 0) {
            syscall::close(this->fd);
            this->fd = -1;
        }
        throw;
    }
}

DmaBufferPool::~DmaBufferPool() noexcept {
    for (auto& [addr, len, bfd] : this->buffers) {
        if (addr) {
            syscall::munmap(addr, len);
        }
        if (bfd >= 0) {
            syscall::close(bfd);
        }
    }
    this->buffers.clear();
    if (this->fd >= 0) {
        syscall::close(this->fd);
        this->fd = -1;
    }
}

DmaBufferPool::DmaBufferPool(DmaBufferPool&& other) noexcept : fd(other.fd), buffers(std::move(other.buffers)) {
    other.fd = -1;
    other.buffers.clear();
}

DmaBufferPool& DmaBufferPool::operator=(DmaBufferPool&& other) noexcept {
    if (this != &other) {
        for (auto& [addr, len, bfd] : this->buffers) {
            if (addr) {
                syscall::munmap(addr, len);
            }
            if (bfd >= 0) {
                syscall::close(bfd);
            }
        }
        this->buffers.clear();
        if (this->fd >= 0) {
            syscall::close(this->fd);
            this->fd = -1;
        }
        this->fd = other.fd;
        this->buffers = std::move(other.buffers);
        other.fd = -1;
        other.buffers.clear();
    }
    return *this;
}

void* DmaBufferPool::sync_start(const uint32_t index) {
    struct dma_buf_sync flags = {};
    flags.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ;
    const auto& [addr, len, bfd] = this->buffers[index];
    if (syscall::ioctl(bfd, DMA_BUF_IOCTL_SYNC, &flags) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl DMA_BUF_IOCTL_SYNC failed.");
    }
    return addr;
}

int DmaBufferPool::sync_end(const uint32_t index) {
    struct dma_buf_sync flags = {};
    flags.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
    const auto& [addr, len, bfd] = this->buffers[index];
    if (syscall::ioctl(bfd, DMA_BUF_IOCTL_SYNC, &flags) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl DMA_BUF_IOCTL_SYNC failed.");
    }
    return bfd;
}

} // namespace tofcam
