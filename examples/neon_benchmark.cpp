#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fakecam.hpp>
#include <fstream>
#include <iostream>
#include <system_error>
#include <utility.hpp>
#include <vector>

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
    if (argc != 5) {
        fprintf(stderr, "usage: %s <source> <width> <height> <bytesperline>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* dir = argv[1];
    const uint32_t width = std::stoi(argv[2]);
    const uint32_t height = std::stoi(argv[3]);
    const uint32_t bytesperline = std::stoi(argv[4]);
    constexpr uint32_t ITER = 30 * 1000;
    auto camera = tofcam::FakeCamera(dir, width, height, bytesperline, 8);
    std::vector<float> depth(width * height, 0.0f);
    std::vector<float> confidence(width * height, 0.0f);
    camera.stream_on();
    auto timer = Timer();
    for (int i = 0; i < ITER; i++) {
        std::pair<void*, uint32_t> frames[4];
        for (int j = 0; j < 4; j++) {
            frames[j] = camera.dequeue();
        }
        tofcam::compute_depth_confidence_from_y12p_neon<false, tofcam::Rotation::Zero>(
                depth.data(), confidence.data(), frames[0].first, frames[1].first, frames[2].first, frames[3].first, width,
                height, bytesperline, 75'000'000);
        for (const auto& [data, index] : frames) {
            camera.enqueue(index);
        }
    }
    auto proctime = timer.elapsed_us();
    printf("%u us (%.2f rawframes/s)\n", proctime, (double)ITER * 1'000'000 * 4 / proctime);
    camera.stream_off();
}
