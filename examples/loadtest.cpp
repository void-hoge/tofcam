#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <tofcam.hpp>
#include <utility.hpp>
#include <vector>

int main() {
    constexpr int range = 2000;
    constexpr bool enableConfidence = true;
    constexpr int modfreq_hz = range == 2000 ? 75'000'000 : 37'500'000;
    auto camera = tofcam::Camera("/dev/video0", "/dev/v4l-subdev2", 8, range, tofcam::MemType::DMABUF);
    const auto [width, height] = camera.get_size();
    const auto [sizeimage, bytesperline] = camera.get_bytes();
    std::vector unpacked(4, std::vector<int16_t>(width * height, 0));
    std::vector<float> depth(width * height, 0.0f);
    std::vector<float> amplitude(width * height, 0.0f);

    camera.stream_on();
    for (int i = 0; i < 30 * 10; i++) {
        for (int j = 0; j < 4; j++) {
            const auto [data, index] = camera.dequeue();
            tofcam::unpack_y12p(unpacked[j].data(), data, width, height, bytesperline);
            camera.enqueue(index);
        }
        if (range == 4000) {
            tofcam::compute_depth_confidence<enableConfidence, tofcam::Rotation::Quarter>(
                    depth.data(), amplitude.data(), unpacked[0].data(), unpacked[1].data(), unpacked[2].data(),
                    unpacked[3].data(), width * height, modfreq_hz);
        } else {
            tofcam::compute_depth_confidence<enableConfidence, tofcam::Rotation::Zero>(
                    depth.data(), amplitude.data(), unpacked[0].data(), unpacked[1].data(), unpacked[2].data(),
                    unpacked[3].data(), width * height, modfreq_hz);
        }
    }
    camera.stream_off();
}
