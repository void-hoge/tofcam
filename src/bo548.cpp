#include <bo548.hpp>
#include <linux/v4l2-subdev.h>
#include <linux/videodev2.h>
#include <syscall.hpp>
#include <utility.hpp>

namespace tofcam {

BO548::BO548(
        const char* device, const char* csi_device, const char* sensor_device, const bool vflip, const bool hflip,
        const MemType memtype, const Mode mode)
    : camera(device, 4, memtype,
             mode == Mode::Single ? std::pair<uint32_t, uint32_t>{640, 2405} : std::pair<uint32_t, uint32_t>{640, 4810}),
      mode(mode) {
    this->csi_fd = syscall::open(csi_device, O_RDWR, 0);
    if (this->csi_fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open the CSI sub-device.");
    }
    this->sensor_fd = syscall::open(sensor_device, O_RDWR, 0);
    if (this->sensor_fd < 0) {
        syscall::close(this->csi_fd);
        throw std::system_error(errno, std::generic_category(), "Failed to open the sensor sub-device.");
    }
    auto [width, height] = this->camera.get_size();
    try {
        { // flip
            if (vflip) {
                struct v4l2_control ctrl = {};
                ctrl.id = V4L2_CID_HFLIP;
                ctrl.value = 1;
                if (syscall::ioctl(this->sensor_fd, VIDIOC_S_CTRL, &ctrl)) {
                    throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_S_CTRL failed.");
                }
            }
            if (hflip) {
                struct v4l2_control ctrl = {};
                ctrl.id = V4L2_CID_VFLIP;
                ctrl.value = 1;
                if (syscall::ioctl(this->sensor_fd, VIDIOC_S_CTRL, &ctrl)) {
                    throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_S_CTRL failed.");
                }
            }
        }
        { // setup the format of the csi2 sub-device
            struct v4l2_subdev_format fmt = {};
            fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
            fmt.pad = 0;
            fmt.format.code = V4L2_MBUS_FMT_Y12_1X12;
            fmt.format.field = V4L2_FIELD_NONE;
            fmt.format.width = 640;
            fmt.format.height = mode == Mode::Single ? 2405 : 4810;
            if (syscall::ioctl(this->csi_fd, VIDIOC_SUBDEV_S_FMT, &fmt) < 0) {
                throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_SUBDEV_S_FMT failed.");
            }
        }
        { // setup the format of the sensor sub-device
            struct v4l2_subdev_format fmt = {};
            fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
            fmt.pad = 0;
            fmt.format.code = V4L2_MBUS_FMT_Y12_1X12;
            fmt.format.field = V4L2_FIELD_NONE;
            fmt.format.width = 640;
            fmt.format.height = mode == Mode::Single ? 2405 : 4810;
            fmt.format.colorspace = V4L2_COLORSPACE_RAW;
            fmt.format.xfer_func = V4L2_XFER_FUNC_NONE;
            fmt.format.ycbcr_enc = V4L2_YCBCR_ENC_601;
            fmt.format.quantization = V4L2_QUANTIZATION_FULL_RANGE;
            if (syscall::ioctl(this->sensor_fd, VIDIOC_SUBDEV_S_FMT, &fmt) < 0) {
                throw std::system_error(errno, std::generic_category(), "ioctl VIDIOC_SUBDEV_S_FMT failed.");
            }
        }
        if (this->mode == Mode::Single) {
            this->depth = std::vector<float>(width * height);
            this->confidence = std::vector<float>(width * height);
        } else {
            this->depth = std::vector<float>(width * height * 2);
            this->confidence = std::vector<float>(width * height * 2);
        }
    } catch (...) {
        if (this->csi_fd < 0) {
            syscall::close(this->csi_fd);
        }
        if (this->sensor_fd < 0) {
            syscall::close(this->sensor_fd);
        }
        throw;
    }

    auto format = this->camera.get_format();
    fprintf(stderr, "format: %c%c%c%c\n", format & 0xff, (format >> 8) & 0xff, (format >> 16) & 0xff, (format >> 24) & 0xff);
    fprintf(stderr, "size: %dx%d\n", width, height);
    auto [sizeimage, bytesperline] = this->camera.get_bytes();
    fprintf(stderr, "sizeimage: %d\n", sizeimage);
    fprintf(stderr, "bytesperline: %d\n", bytesperline);
}

BO548::~BO548() {
    if (this->csi_fd >= 0) {
        syscall::close(this->csi_fd);
    }
    if (this->sensor_fd >= 0) {
        syscall::close(this->sensor_fd);
    }
}

void BO548::stream_on() {
    this->camera.stream_on();
}

void BO548::stream_off() {
    this->camera.stream_off();
}

std::pair<float*, float*> BO548::get_frame() {
    const auto [width, height] = this->get_size();
    const auto [sizeimage, bytesperline] = this->camera.get_bytes();
    const auto [ptr, idx] = this->camera.dequeue();
    {
        const auto phase0 = static_cast<uint8_t*>(ptr) + bytesperline * height * 0;
        const auto phase1 = static_cast<uint8_t*>(ptr) + bytesperline * height * 1;
        const auto phase2 = static_cast<uint8_t*>(ptr) + bytesperline * height * 2;
        const auto phase3 = static_cast<uint8_t*>(ptr) + bytesperline * height * 3;
        compute_depth_confidence_from_y12p_neon<true, Rotation::Zero>(
                this->depth.data(), this->confidence.data(), phase0, phase1, phase2, phase3, width, height, bytesperline,
                90'000'000);
    }
    if (this->mode == Mode::Double) {
        const auto phase0 = static_cast<uint8_t*>(ptr) + bytesperline * 2405 + bytesperline * height * 0;
        const auto phase1 = static_cast<uint8_t*>(ptr) + bytesperline * 2405 + bytesperline * height * 1;
        const auto phase2 = static_cast<uint8_t*>(ptr) + bytesperline * 2405 + bytesperline * height * 2;
        const auto phase3 = static_cast<uint8_t*>(ptr) + bytesperline * 2405 + bytesperline * height * 3;
        compute_depth_confidence_from_y12p_neon<true, Rotation::Zero>(
                this->depth.data() + width * height, this->confidence.data() + width * height, phase0, phase1, phase2, phase3,
                width, height, bytesperline, 15'000'000);
    }
    this->camera.enqueue(idx);
    return {this->depth.data(), this->confidence.data()};
}

std::pair<uint32_t, uint32_t> BO548::get_size() const {
    return {640, 480};
}

std::pair<uint32_t, uint32_t> BO548::get_bytes() const {
    return this->camera.get_bytes();
}

void* BO548::get_rawframe() {
    if (this->locked_index) {
        this->camera.enqueue(this->locked_index.value());
        this->locked_index = std::nullopt;
    }
    const auto [ptr, idx] = this->camera.dequeue();
    this->locked_index = idx;
    return ptr;
}

} // namespace tofcam
