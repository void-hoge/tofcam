#include <cstdio>
#include <cstdlib>
#include <fakecam.hpp>
#include <fstream>
#include <iostream>
#include <system_error>
#include <tofcam.hpp>
#include <utility.hpp>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <device>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* device = argv[1];
    constexpr uint32_t ITER = 60;

    auto camera = tofcam::Camera(device, 8, tofcam::MemType::MMAP);

    const auto [width, height] = camera.get_size();
    const auto [sizeimage, bytesperline] = camera.get_bytes();
    std::vector<std::vector<float>> depth;
    std::vector<std::vector<float>> amplitude;

    camera.stream_on();
    for (int i = 0; i < ITER; i++) {
        std::pair<void*, uint32_t> frames[4];
        for (int j = 0; j < 4; j++) {
            frames[j] = camera.dequeue();
        }
        depth.emplace_back(width * height, 0.0f);
        amplitude.emplace_back(width * height, 0.0f);
        tofcam::compute_depth_confidence_from_y12p_neon<false>(
                depth.back().data(), amplitude.back().data(), frames[0].first, frames[1].first, frames[2].first,
                frames[3].first, width, height, bytesperline, 75'000'000);
        for (const auto& [data, index] : frames) {
            camera.enqueue(index);
        }
    }
    camera.stream_off();

    fprintf(stderr, "saving frames\n");
    std::ofstream depth_ofst("neon_depth.csv");
    std::ofstream amplitude_ofst("neon_amplitude.csv");
    std::ofstream ofst("neon.csv");
    for (int i = 0; i < ITER; i++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                depth_ofst << depth[i][y * width + x];
                amplitude_ofst << amplitude[i][y * width + x];
                ofst << depth[i][y * width + x] << ", " << amplitude[i][y * width + x] << "\n";
                if (x < width - 1) {
                    depth_ofst << ", ";
                    amplitude_ofst << ", ";
                }
            }
            depth_ofst << "\n";
            amplitude_ofst << "\n";
        }
    }
}
