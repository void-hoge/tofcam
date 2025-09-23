#include <tofcam.hpp>
#include <syscall.hpp>
#include <system_error>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <vector>

namespace tofcam {

Camera::~Camera() {
    this->reset();
}

Camera::Camera(const char* device, const uint32_t num_buffers) {
    this->fd = syscall::open(device, O_RDWR, 0);
    if (this->fd < 0) {
        this->reset();
        throw std::system_error(errno, std::generic_category(), "Failed to open camera device.");
    }
    { // check capability
        struct v4l2_capability cap = {};
        if (syscall::ioctl(this->fd, VIDIOC_QUERYCAP, &cap) < 0) {
            this->reset();
            throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_QUERYCAP failed.");
        }
        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
            this->reset();
            throw std::runtime_error("Device does not support video capture.");
        }
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            this->reset();
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
            this->reset();
            throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_S_FMT failed.");
        }
        this->sizeimage = fmt.fmt.pix.sizeimage;
        this->bytesperline = fmt.fmt.pix.bytesperline;
        if (fmt.fmt.pix.width != WIDTH) {
            this->reset();
            throw std::runtime_error("Unsupported width.");
        }
        if (fmt.fmt.pix.height != HEIGHT) {
            this->reset();
            throw std::runtime_error("Unsupported height.");
        }
        if (this->sizeimage == 0) {
            this->reset();
            throw std::runtime_error("Sizeimage is zero, unsupported format?");
        }
        if (this->bytesperline == 0) {
            this->reset();
            throw std::runtime_error("Bytesperline is zero, unsupported format?");
        }
        fprintf(stderr, "sizeimage: %d, bytesperline: %d\n", this->sizeimage, this->bytesperline);
    }
    { // setup mmap buffers
        struct v4l2_requestbuffers req = {};
        req.count = num_buffers;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        if (syscall::ioctl(this->fd, VIDIOC_REQBUFS, &req) < 0) {
            this->reset();
            throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_REQBUFS failed.");
        }
        if (req.count != num_buffers) {
            this->reset();
            throw std::runtime_error("Buffer request failed.");
        }
    }
    { // mmap buffers
        buffers.reserve(num_buffers);
        for (uint32_t i = 0; i < num_buffers; i++) {
            struct v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            if (syscall::ioctl(this->fd, VIDIOC_QUERYBUF, &buf) < 0) {
                this->reset();
                throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_QUERYBUF failed.");
            }
            auto data = syscall::mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, buf.m.offset);
            if (data == MAP_FAILED) {
                throw std::system_error(errno, std::generic_category(), "mmap failed.");
            }
            this->buffers.emplace_back(data, buf.length);
        }
    }
    { // enqueue all buffers
        for (uint32_t i = 0; i < num_buffers; i++) {
            this->enqueue(i);
        }
    }
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
    buf.memory = V4L2_MEMORY_MMAP;
    if (syscall::ioctl(this->fd, VIDIOC_DQBUF, &buf) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_DQBUF failed.");
    }
    return {buffers[buf.index].first, buf.index};
}

void Camera::enqueue(const uint32_t index) {
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;
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

void Camera::reset() noexcept {
    if (this->fd != -1) {
        syscall::close(this->fd);
        this->fd = -1;
    }
}

}
