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
		fmt.fmt.pix.width = 240;
		fmt.fmt.pix.height = 180;
        if (syscall::ioctl(this->fd, VIDIOC_S_FMT, &fmt) < 0) {
			this->reset();
            throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_S_FMT failed.");
        }
        this->width = fmt.fmt.pix.width;
        this->height = fmt.fmt.pix.height;
        this->sizeimage = fmt.fmt.pix.sizeimage;
        if (this->width == 0) {
			this->reset();
			throw std::runtime_error("Width is zero, unsupported format?");
		}
		if (this->height == 0) {
			this->reset();
			throw std::runtime_error("Height is zero, unsupported format?");
		}
		if (this->sizeimage == 0) {
			this->reset();
			throw std::runtime_error("Sizeimage is zero, unsupported format?");
		}
		fprintf(stderr, "size: %dx%d, sizeimage: %d\n", this->width, this->height, this->sizeimage);
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
		}
	}
	{ // enqueue all buffers
		for (uint32_t i = 0; i < num_buffers; i++) {
			struct v4l2_buffer buf = {};
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;
			if (syscall::ioctl(this->fd, VIDIOC_QBUF, &buf) < 0) {
				throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_QBUF failed.");
			}
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
