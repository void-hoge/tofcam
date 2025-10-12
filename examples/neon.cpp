#include <tofcam.hpp>
#include <fakecam.hpp>
#include <utility.hpp>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <system_error>
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <device>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* device = argv[1];

    auto camera = tofcam::FakeCamera(device);

    const auto [width, height] = camera.get_size();
    const auto [sizeimage, bytesperline] = camera.get_bytes();
    std::vector<float> depth(width * height, 0.0f);

    std::ofstream depth_ofst("neon_depth.csv");
    std::ofstream ofst("neon.csv");

    camera.stream_on();
    for (int i = 0; i < 1; i++) {
        std::pair<void*, uint32_t> frames[4];
        for (int j = 0; j < 4; j++) {
            frames[j] = camera.dequeue();
        }
        tofcam::compute_depth_from_y12p_neon(
            depth.data(),
            frames[0].first, frames[1].first, frames[2].first, frames[3].first,
            width, height, bytesperline, 75'000'000);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                depth_ofst << depth[y * width + x];
                ofst << depth[y * width + x] << "\n";
                if (x < width - 1) {
                    depth_ofst << ", ";
                }
            }
            depth_ofst << "\n";
        }
        for (const auto& [data, index]: frames) {
            camera.enqueue(index);
        }
    }
    camera.stream_off();
}
