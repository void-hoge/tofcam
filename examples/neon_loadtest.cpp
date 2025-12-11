#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <tofcam.hpp>
#include <utility.hpp>
#include <vector>

int main() {
    constexpr int range = 2000;
    constexpr bool enableConfidence = false;
    auto camera = tofcam::Camera("/dev/video0", "/dev/v4l-subdev2", 8, range, tofcam::MemType::DMABUF);
    const auto [width, height] = camera.get_size();
    const auto [sizeimage, bytesperline] = camera.get_bytes();
    std::vector<float> depth(width * height, 0.0f);
    std::vector<float> amplitude(width * height, 0.0f);

    camera.stream_on();
    for (int i = 0; i < 30 * 10; i++) {
        std::pair<void*, uint32_t> frames[4];
        for (int j = 0; j < 4; j++) {
            frames[j] = camera.dequeue();
        }
        if (range == 4000) {
            tofcam::compute_depth_confidence_from_y12p_neon<enableConfidence, tofcam::Rotation::Quarter>(
                depth.data(), amplitude.data(), frames[0].first, frames[1].first, frames[2].first, frames[3].first, width,
                height, bytesperline, range);
        } else {
            tofcam::compute_depth_confidence_from_y12p_neon<enableConfidence, tofcam::Rotation::Zero>(
                depth.data(), amplitude.data(), frames[0].first, frames[1].first, frames[2].first, frames[3].first, width,
                height, bytesperline, range);
        }
        fprintf(stderr, "%7.2f\n", depth[90 * width + 120]);
        for (const auto& [data, index] : frames) {
            camera.enqueue(index);
        }
    }
    camera.stream_off();
}
