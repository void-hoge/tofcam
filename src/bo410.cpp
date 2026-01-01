#include <bo410.hpp>
#include <linux/videodev2.h>
#include <syscall.hpp>
#include <utility.hpp>

namespace tofcam {

BO410::BO410(const char* device, const char* subdevice, const int range, const MemType memtype)
    : camera(device, 8, memtype, std::nullopt) {
    if (range != 2000 && range != 4000) {
        throw std::invalid_argument("Invalid range mode.");
    }
    this->subfd = syscall::open(subdevice, O_RDWR, 0);
    if (this->subfd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open the subdevice.");
    }
    try {
        struct v4l2_control ctrl = {
                .id = V4L2_CTRL_CLASS_USER + 0x1901,
                .value = range == 2000 ? 1 : 0,
        };
        if (syscall::ioctl(this->subfd, VIDIOC_S_CTRL, &ctrl)) {
            syscall::close(this->subfd);
            throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_S_CTRL failed.");
        }
        auto [width, height] = this->camera.get_size();
        this->range = range;
        this->depth = std::vector<float>(width * height);
        this->confidence = std::vector<float>(width * height);
    } catch (...) {
        if (this->subfd >= 0) {
            syscall::close(this->subfd);
            this->subfd = -1;
        }
        throw;
    }
}

BO410::~BO410() {
    if (this->subfd >= 0) {
        syscall::close(this->subfd);
        this->subfd = -1;
    }
}

void BO410::stream_on() {
    this->camera.stream_on();
}

void BO410::stream_off() {
    this->camera.stream_off();
}

std::pair<float*, float*> BO410::get_frame() {
    const auto [width, height] = this->camera.get_size();
    const auto [bytesused, bytesperline] = this->camera.get_bytes();
    const int modfreq_hz = 300'000'000 / this->range / 2 * 1000;
    std::pair<void*, uint32_t> frames[4];
    for (int i = 0; i < 4; i++) {
        frames[i] = this->camera.dequeue();
    }
    if (this->range == 2000) {
        compute_depth_confidence_from_y12p_neon<true, Rotation::Zero>(
                this->depth.data(), this->confidence.data(), frames[0].first, frames[1].first, frames[2].first, frames[3].first,
                width, height, bytesperline, modfreq_hz);
    } else {
        compute_depth_confidence_from_y12p_neon<true, Rotation::Quarter>(
                this->depth.data(), this->confidence.data(), frames[0].first, frames[1].first, frames[2].first, frames[3].first,
                width, height, bytesperline, modfreq_hz);
    }
    for (int i = 0; i < 4; i++) {
        this->camera.enqueue(frames[i].second);
    }
    return {this->depth.data(), this->confidence.data()};
}

} // namespace tofcam
