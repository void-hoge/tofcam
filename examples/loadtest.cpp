#include <tofcam.hpp>
#include <utility.hpp>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <chrono>

int main(int argc, char* argv[]) {
        if (argc != 2) {
        fprintf(stderr, "usage: %s <device>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* device = argv[1];

    auto camera = tofcam::Camera(device, 8);

    const auto [width, height] = camera.get_size();
    const auto [sizeimage, bytesperline] = camera.get_bytes();
    std::vector unpacked(4, std::vector<int16_t>(width * height, 0));
    std::vector<float> depth(width * height, 0.0f);
    std::vector<float> amplitude(width * height, 0.0f);
    std::vector<int16_t> raw(width * height, 0);

    camera.stream_on();
    for (int i = 0; i < 30 * 100; i++) {
        for (int j = 0; j < 4; j++) {
            const auto [data, index] = camera.dequeue();
            tofcam::unpack_y12p(unpacked[j].data(), data, width, height, bytesperline);
            camera.enqueue(index);
        }
        tofcam::compute_depth_amplitude(
            depth.data(), amplitude.data(), raw.data(),
            unpacked[0].data(), unpacked[1].data(), unpacked[2].data(), unpacked[3].data(),
            width * height, 75'000'000);
    }
    camera.stream_off();
}
