#include <camera.hpp>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <syscall.hpp>
#include <system_error>
#include <vector>

namespace tofcam {

Camera::Camera(const char* device, const char* subdevice, const uint32_t num_buffers, const int range, const MemType memtype)
    : MemoryType(memtype == MemType::MMAP ? V4L2_MEMORY_MMAP : V4L2_MEMORY_DMABUF) {
    if (range != 2000 && range != 4000) {
        throw std::invalid_argument("Invalid range mode.");
    }
    this->fd = syscall::open(device, O_RDWR, 0);
    if (this->fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open camera device.");
    }
    this->subfd = syscall::open(subdevice, O_RDWR, 0);
    if (this->subfd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open subdevice.");
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
        { // set format
            struct v4l2_format fmt = {};
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            fmt.fmt.pix.field = V4L2_FIELD_NONE;
            fmt.fmt.pix.pixelformat = v4l2_fourcc('Y', '1', '2', 'P');
            fmt.fmt.pix.width = WIDTH;
            fmt.fmt.pix.height = HEIGHT;
            if (syscall::ioctl(this->fd, VIDIOC_S_FMT, &fmt) < 0) {
                throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_S_FMT failed.");
            }
            this->sizeimage = fmt.fmt.pix.sizeimage;
            this->bytesperline = fmt.fmt.pix.bytesperline;
            if (fmt.fmt.pix.width != WIDTH) {
                throw std::runtime_error("Unsupported width.");
            }
            if (fmt.fmt.pix.height != HEIGHT) {
                throw std::runtime_error("Unsupported height.");
            }
            if (this->sizeimage == 0) {
                throw std::runtime_error("Sizeimage is zero, unsupported format?");
            }
            if (this->bytesperline == 0) {
                throw std::runtime_error("Bytesperline is zero, unsupported format?");
            }
        }
        { // setup mmap buffers
            struct v4l2_requestbuffers req = {};
            req.count = num_buffers;
            req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            req.memory = this->MemoryType;
            if (syscall::ioctl(this->fd, VIDIOC_REQBUFS, &req) < 0) {
                throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_REQBUFS failed.");
            }
            if (req.count != num_buffers) {
                throw std::runtime_error("Buffer request failed.");
            }
        }
        // allocate buffers
        if (this->MemoryType == V4L2_MEMORY_MMAP) {
            this->buffers = std::make_unique<MmapBufferPool>(this->fd, num_buffers);
        } else {
            this->buffers = std::make_unique<DmaBufferPool>("/dev/dma_heap/linux,cma", num_buffers, this->sizeimage);
        }
        { // enqueue all buffers
            for (uint32_t i = 0; i < num_buffers; i++) {
                this->enqueue(i);
            }
        }
        { // change mesuaring range
            struct v4l2_control ctrl = {
                    .id = V4L2_CTRL_CLASS_USER + 0x1901,
                    .value = range == 2000 ? 1 : 0,
            };
            if (syscall::ioctl(this->subfd, VIDIOC_S_CTRL, &ctrl)) {
                throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_S_CTRL failed.");
            }
        }
    } catch (...) {
        if (this->fd >= 0) {
            syscall::close(this->fd);
            this->fd = -1;
        }
        if (this->subfd >= 0) {
            syscall::close(this->subfd);
            this->subfd = -1;
        }
        throw;
    }
}

Camera::~Camera() noexcept {
    if (this->subfd >= 0) {
        syscall::close(this->subfd);
        this->subfd = -1;
    }
    if (this->fd >= 0) {
        syscall::close(this->fd);
        this->fd = -1;
    }
}

Camera::Camera(Camera&& other) noexcept
    : MemoryType(other.MemoryType), fd(other.fd), sizeimage(other.sizeimage), bytesperline(other.bytesperline),
      buffers(std::move(other.buffers)) {}

Camera& Camera::operator=(Camera&& other) noexcept {
    if (this != &other) {
        this->MemoryType = other.MemoryType;
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
    buf.memory = this->MemoryType;
    if (syscall::ioctl(this->fd, VIDIOC_DQBUF, &buf) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_DQBUF failed.");
    }
    return {this->buffers->sync_start(buf.index), buf.index};
}

void Camera::enqueue(const uint32_t index) {
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = this->MemoryType;
    buf.index = index;
    buf.m.fd = this->buffers->sync_end(index);
    buf.length = this->sizeimage;
    if (syscall::ioctl(this->fd, VIDIOC_QBUF, &buf) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_QBUF failed.");
    }
}

std::pair<uint32_t, uint32_t> Camera::get_size() const {
    return {WIDTH, HEIGHT};
}

std::pair<uint32_t, uint32_t> Camera::get_bytes() const {
    return {this->sizeimage, this->bytesperline};
}

} // namespace tofcam
