#include <tofcam.hpp>
#include <utility.hpp>
#include <cstdlib>
#include <cstdio>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <device>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* device = argv[1];

    auto camera = tofcam::Camera(device, 8);

    const auto [width, height] = camera.get_size();
    const auto [sizeimage, bytesperline] = camera.get_bytes();
    std::vector<std::vector<uint16_t>> unpacked(4, std::vector<uint16_t>(width * height, 0));
    
    camera.stream_on();
    for (int i = 0; i < 30 * 5; i++) {
        for (int j = 0; j < 4; j++) {
            const auto [data, index] = camera.dequeue();
            tofcam::unpack_y12p(unpacked[j].data(), data, width, height, bytesperline);
            camera.enqueue(index);
        }
    }
    camera.stream_off();
}
