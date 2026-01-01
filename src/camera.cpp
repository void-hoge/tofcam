#include <camera.hpp>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <syscall.hpp>
#include <system_error>
#include <vector>

namespace tofcam {

Camera::Camera(
        const char* device, const uint32_t num_buffers, const MemType memtype,
        std::optional<const std::pair<uint32_t, uint32_t>> imagesize)
    : memorytype(memtype == MemType::MMAP ? V4L2_MEMORY_MMAP : V4L2_MEMORY_DMABUF) {
    this->fd = syscall::open(device, O_RDWR, 0);
    if (this->fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open camera device.");
    }
    try {
        { // check capability
            struct v4l2_capability cap = {};
            if (syscall::ioctl(this->fd, VIDIOC_QUERYCAP, &cap) < 0) {
                throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_QUERYCAP failed.");
            }
            if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                throw std::runtime_error("Device does not support video capture.");
            }
            if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                throw std::runtime_error("Device does not support video streaming.");
            }
        }
        { // set capture format
            struct v4l2_format fmt = {};
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (imagesize) {
                auto [width, height] = imagesize.value();
                fmt.fmt.pix.width = width;
                fmt.fmt.pix.height = height;
                fmt.fmt.pix.pixelformat = v4l2_fourcc('Y', '1', '2', 'P');
                fmt.fmt.pix.colorspace = V4L2_COLORSPACE_DEFAULT;
                if (syscall::ioctl(this->fd, VIDIOC_TRY_FMT, &fmt) < 0) {
                    throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_TRY_FMT failed.");
                }
            } else {
                if (syscall::ioctl(this->fd, VIDIOC_G_FMT, &fmt) < 0) {
                    throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_G_FMT failed.");
                }
                if (fmt.fmt.pix.sizeimage == 0) {
                    throw std::runtime_error("Sizeimage is zero, unsupported format?");
                }
                if (fmt.fmt.pix.bytesperline == 0) {
                    throw std::runtime_error("Bytesperline is zero, unsupported format?");
                }
            }
            this->width = fmt.fmt.pix.width;
            this->height = fmt.fmt.pix.height;
            this->sizeimage = fmt.fmt.pix.sizeimage;
            this->bytesperline = fmt.fmt.pix.bytesperline;
            this->pixelformat = fmt.fmt.pix.pixelformat;
            if (syscall::ioctl(this->fd, VIDIOC_S_FMT, &fmt) < 0) {
                throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_S_FMT failed.");
            }
        }
        { // setup buffers
            struct v4l2_requestbuffers req = {};
            req.count = num_buffers;
            req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            req.memory = this->memorytype;
            if (syscall::ioctl(this->fd, VIDIOC_REQBUFS, &req) < 0) {
                throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_REQBUFS failed.");
            }
            if (req.count != num_buffers) {
                throw std::runtime_error("Buffer request failed.");
            }
        }
        // allocate buffers
        if (this->memorytype == V4L2_MEMORY_MMAP) {
            this->buffers = std::make_unique<MmapBufferPool>(this->fd, num_buffers);
        } else {
            this->buffers = std::make_unique<DmaBufferPool>("/dev/dma_heap/linux,cma", num_buffers, this->sizeimage);
        }
        { // enqueue all buffers
            for (uint32_t i = 0; i < num_buffers; i++) {
                this->enqueue(i);
            }
        }
    } catch (...) {
        if (this->fd >= 0) {
            syscall::close(this->fd);
            this->fd = -1;
        }
        throw;
    }
}

Camera::~Camera() noexcept {
    if (this->fd >= 0) {
        syscall::close(this->fd);
        this->fd = -1;
    }
}

Camera::Camera(Camera&& other) noexcept
    : memorytype(other.memorytype), fd(other.fd), sizeimage(other.sizeimage), bytesperline(other.bytesperline),
      buffers(std::move(other.buffers)) {}

Camera& Camera::operator=(Camera&& other) noexcept {
    if (this != &other) {
        this->memorytype = other.memorytype;
        if (this->fd >= 0) {
            syscall::close(this->fd);
        }
        this->fd = other.fd;
        this->sizeimage = other.sizeimage;
        this->bytesperline = other.bytesperline;
        this->buffers = std::move(other.buffers);
    }
    return *this;
}

void Camera::stream_on() {
    uint32_t type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (syscall::ioctl(this->fd, VIDIOC_STREAMON, &type) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_STREAMON failed.");
    }
}

void Camera::stream_off() {
    uint32_t type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (syscall::ioctl(this->fd, VIDIOC_STREAMOFF, &type) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_STREAMOFF failed.");
    }
}

std::pair<void*, uint32_t> Camera::dequeue() {
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = this->memorytype;
    if (syscall::ioctl(this->fd, VIDIOC_DQBUF, &buf) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_DQBUF failed.");
    }
    return {this->buffers->sync_start(buf.index), buf.index};
}

void Camera::enqueue(const uint32_t index) {
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = this->memorytype;
    buf.index = index;
    buf.m.fd = this->buffers->sync_end(index);
    buf.length = this->sizeimage;
    if (syscall::ioctl(this->fd, VIDIOC_QBUF, &buf) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_QBUF failed.");
    }
}

std::pair<uint32_t, uint32_t> Camera::get_size() const {
    return {this->width, this->height};
}

std::pair<uint32_t, uint32_t> Camera::get_bytes() const {
    return {this->sizeimage, this->bytesperline};
}

uint32_t Camera::get_format() const {
    return this->pixelformat;
}

} // namespace tofcam
