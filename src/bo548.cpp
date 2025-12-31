#include <bo548.hpp>
#include <linux/videodev2.h>
#include <syscall.hpp>
#include <utility.hpp>

namespace tofcam {

BO548::BO548(const char* device, const MemType memtype) : camera(device, 4, memtype) {
    auto format = this->camera.get_format();
    fprintf(stderr, "format: %c%c%c%c\n", format & 0xff, (format >> 8) & 0xff, (format >> 16) & 0xff, (format >> 24) & 0xff);
    auto [width, height] = this->camera.get_size();
    fprintf(stderr, "size: %dx%d\n", width, height);
    auto [sizeimage, bytesperline] = this->camera.get_bytes();
    fprintf(stderr, "sizeimage: %d\n", sizeimage);
    fprintf(stderr, "bytesperline: %d\n", bytesperline);
    this->depth = std::vector<float>(width * height);
    this->confidence = std::vector<float>(width * height);
}

BO548::~BO548() {}

void BO548::stream_on() {
    this->camera.stream_on();
}

void BO548::stream_off() {
    this->camera.stream_off();
}

std::pair<float*, float*> BO548::get_frame() {
    constexpr uint32_t width = 640;
    constexpr uint32_t height = 480;
    auto [sizeimage, bytesperline] = this->camera.get_bytes();
    auto [ptr, idx] = this->camera.dequeue();
    auto phase0 = static_cast<uint8_t*>(ptr) + bytesperline * height * 0;
    auto phase1 = static_cast<uint8_t*>(ptr) + bytesperline * height * 1;
    auto phase2 = static_cast<uint8_t*>(ptr) + bytesperline * height * 2;
    auto phase3 = static_cast<uint8_t*>(ptr) + bytesperline * height * 3;
    compute_depth_confidence_from_y12p_neon<true, Rotation::Zero>(
            this->depth.data(), this->confidence.data(), phase0, phase1, phase2, phase3, width, height, bytesperline,
            90'000'000);
    this->camera.enqueue(idx);
    return {this->depth.data(), this->confidence.data()};
}

std::pair<uint32_t, uint32_t> BO548::get_size() const {
    return {640, 480};
}

} // namespace tofcam
