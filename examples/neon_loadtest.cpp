#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <tofcam.hpp>
#include <utility.hpp>
#include <vector>

int main() {
    auto camera = tofcam::Camera("/dev/video0", 8, tofcam::MemType::DMABUF);

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
        tofcam::compute_depth_confidence_from_y12p_neon<true>(
                depth.data(), amplitude.data(), frames[0].first, frames[1].first, frames[2].first, frames[3].first, width,
                height, bytesperline, 2000);
        for (const auto& [data, index] : frames) {
            camera.enqueue(index);
        }
    }
    camera.stream_off();
}
