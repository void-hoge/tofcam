#include <tofcam.hpp>
#include <fakecam.hpp>
#include <utility.hpp>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <system_error>
#include <iostream>
#include <fstream>
#include <chrono>

class Timer {
public:
    Timer() : start(std::chrono::system_clock::now()) {}
    uint32_t elapsed_us() {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - this->start).count();
    }
private:
    std::chrono::system_clock::time_point start;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <device>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* device = argv[1];
    constexpr uint32_t ITER = 30 * 1000;

    auto camera = tofcam::FakeCamera(device);

    const auto [width, height] = camera.get_size();
    const auto [sizeimage, bytesperline] = camera.get_bytes();
    std::vector unpacked(4, std::vector<int16_t>(width * height, 0));
    std::vector<float> depth(width * height, 0.0f);
    std::vector<float> confidence(width * height, 0.0f);
    camera.stream_on();
    auto timer = Timer();
    for (int i = 0; i < ITER; i++) {
        std::pair<void*, uint32_t> frames[4];
        for (int j = 0; j < 4; j++) {
            frames[j] = camera.dequeue();
        }
        tofcam::compute_depth_confidence_from_y12p_neon<false>(
            depth.data(), confidence.data(),
            frames[0].first, frames[1].first, frames[2].first, frames[3].first,
            width, height, bytesperline, 75'000'000);
        for (const auto& [data, index]: frames) {
            camera.enqueue(index);
        }
    }
    auto proctime = timer.elapsed_us();
    printf("%u us (%.2f rawframes/s)\n", proctime, (double)ITER * 1'000'000 * 4/proctime);
    camera.stream_off();
}
