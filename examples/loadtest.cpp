#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <tofcam.hpp>
#include <utility.hpp>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <device>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* device = argv[1];

    auto camera = tofcam::Camera(device, 8, tofcam::MemType::DMABUF);

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
        tofcam::compute_depth_confidence<true>(
                depth.data(), amplitude.data(), unpacked[0].data(), unpacked[1].data(), unpacked[2].data(), unpacked[3].data(),
                width * height, 2000);
    }
    camera.stream_off();
}
