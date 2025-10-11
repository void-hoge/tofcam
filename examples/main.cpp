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
    std::vector unpacked(4, std::vector<int16_t>(width * height, 0));
    std::vector<float> depth(width * height, 0.0f);
    std::vector<float> amplitude(width * height, 0.0f);
    std::vector<int16_t> raw(width * height, 0);

    std::ofstream depth_ofst("tofcam_depth.csv");
    std::ofstream amplitude_ofst("tofcam_amplitude.csv");
    std::ofstream ofst("tofcam.csv");

    camera.stream_on();
    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < 4; j++) {
            const auto [data, index] = camera.dequeue();
            tofcam::unpack_y12p(unpacked[j].data(), data, width, height, bytesperline);
            camera.enqueue(index);
        }
        tofcam::compute_depth_amplitude(
            depth.data(), amplitude.data(), raw.data(),
            unpacked[0].data(), unpacked[1].data(), unpacked[2].data(), unpacked[3].data(),
            width * height, 75'000'000);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                depth_ofst << depth[y * width + x];
                amplitude_ofst << amplitude[y * width + x];
                ofst << depth[y * width + x] << ", " << amplitude[y * width + x] << ", " << raw[y * width + x] << "\n";
                if (x < width - 1) {
                    depth_ofst << ", ";
                    amplitude_ofst << ", ";
                }
            }
            depth_ofst << "\n";
            amplitude_ofst << "\n";
        }
    }
    camera.stream_off();
}
