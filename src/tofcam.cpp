#include <tofcam.hpp>
#include <syscall.hpp>
#include <system_error>
#include <linux/videodev2.h>
#include <vector>

namespace tofcam {

Camera::~Camera() {
    this->reset();
}

Camera::Camera(const char* device, const uint32_t buffer_count) {
    this->fd = syscall::open(device, O_RDWR, 0);
    if (this->fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open camera device.");
    }
    { // check capability
        struct v4l2_capability cap = {0};
        if (syscall::ioctl(this->fd, VIDIOC_QUERYCAP, &cap) < 0) {
            throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_QUERYCAP failed.");
        }
        if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
            throw std::runtime_error("Device does not support video capture.");
        }
        if (cap.capabilities & V4L2_CAP_STREAMING) {
            throw std::runtime_error("Device does not support video streaming.");
        }
    }
    { // set format
        struct v4l2_format fmt = {0};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.field = V4L2_FIELD_NONE;
        fmt.fmt.pix.pixelformat = v4l2_fourcc('Y', '1', '2', 'P');
        if (syscall::ioctl(this->fd, VIDIOC_S_FMT, &fmt) < 0) {
            throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_S_FMT failed.");
        }
        this->width = fmt.fmt.pix.width;
        this->height = fmt.fmt.pix.height;
        this->sizeimage = fmt.fmt.pix.sizeimage;
        if (this->width == 0) {
			throw std::runtime_error("Width is zero, unsupported format?");
		}
		if (this->height == 0) {
			throw std::runtime_error("Height is zero, unsupported format?");
		}
		if (this->sizeimage == 0) {
			throw std::runtime_error("Sizeimage is zero, unsupported format?");
		}
    }
	{ // setup mmap buffers
		struct v4l2_requestbuffers req = {0};
		req.count = 4;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;
		if (syscall::ioctl(this->fd, VIDIOC_REQBUFS, &req) < 0) {
			throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_REQBUFS failed.");
		}
		if (req.count < buffer_count) {
			throw std::runtime_error("Buffer request failed.");
		}
	}
}

void Camera::reset() noexcept {
    if (this->fd != -1) {
        syscall::close(this->fd);
        this->fd = -1;
    }
}

}
