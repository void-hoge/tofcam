#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fakecam.hpp>
#include <utility.hpp>

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
    std::vector unpacked(4, std::vector<int16_t>(width * height, 0));
    std::vector<float> depth(width * height, 0.0f);
    std::vector<float> amplitude(width * height, 0.0f);
    camera.stream_on();
    auto timer = Timer();
    for (int i = 0; i < ITER; i++) {
        for (int j = 0; j < 4; j++) {
            const auto [data, index] = camera.dequeue();
            tofcam::unpack_y12p(unpacked[j].data(), data, width, height, bytesperline);
            camera.enqueue(index);
        }
        tofcam::compute_depth_confidence<true>(
                depth.data(), amplitude.data(), unpacked[0].data(), unpacked[1].data(), unpacked[2].data(), unpacked[3].data(),
                width * height, 75'000'000);
    }
    auto proctime = timer.elapsed_us();
    printf("%u us (%.2f rawframes/s)\n", proctime, (double)ITER * 1'000'000 * 4 / proctime);
    camera.stream_off();
}
