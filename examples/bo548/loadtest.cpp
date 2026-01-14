#include <bo548.hpp>

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <device> <csi> <sensor> <directory>\n", argv[0]);
        return 0;
    }
    const char* devnode = argv[1];
    const char* csinode = argv[2];
    const char* sensornode = argv[3];
    const char* directory = argv[4];
    const uint32_t iter = 30 * 100;
    auto camera = tofcam::BO548(devnode, csinode, sensornode, true, true, 1000, tofcam::MemType::DMABUF, tofcam::Mode::Double);
    const auto [width, height] = camera.get_size();
    camera.stream_on();

    for (int i = 0; i < iter; i++) {
        auto [depth, confidence] = camera.get_frame();
    }
    camera.stream_off();
}
