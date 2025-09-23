#include <tofcam.hpp>
#include <utility.hpp>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <system_error>

void save_bytes(const char *filename, const void *addr, const size_t size) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        throw std::system_error(errno, std::generic_category(), "Failed to open file.");
    }
    const size_t written = fwrite(addr, 1, size, fp);
    fclose(fp);
    if (written != size) {
        throw std::system_error(errno, std::generic_category(), "Failed to write all data.");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <device> <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* device = argv[1];
    const char* outdir = argv[2];

    auto camera = tofcam::Camera(device, 8);

    const auto [width, height] = camera.get_size();
    const auto [sizeimage, bytesperline] = camera.get_bytes();
    std::vector<std::vector<uint16_t>> unpacked(4, std::vector<uint16_t>(width * height, 0));

    std::vector<float> depth(width * height, 0.0f);
    std::vector<float> amplitude(width * height, 0.0f);
    
    camera.stream_on();
    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < 4; j++) {
            const auto [data, index] = camera.dequeue();
            tofcam::unpack_y12p(unpacked[j].data(), data, width, height, bytesperline);
            camera.enqueue(index);
        }
        tofcam::compute_depth_amplitude(depth.data(), amplitude.data(),
                                        unpacked[0].data(), unpacked[1].data(),
                                        unpacked[2].data(), unpacked[3].data(),
                                        width * height, 37.5e6 /* 75MHz */);
        char filename[256] = {};
        snprintf(filename, 256, "%s/depth_%03d.raw", outdir, i);
        save_bytes(filename, depth.data(), width * height * sizeof(float));
        snprintf(filename, 256, "%s/amplitude_%03d.raw", outdir, i);
        save_bytes(filename, amplitude.data(), width * height * sizeof(float));
    }
    camera.stream_off();

    {
        printf("depth\n");
        float min = depth[0];
        float max = depth[0];
        for (int i = 0; i < width * height; i++) {
            min = (min > depth[i]) ? depth[i] : min;
            max = (max < depth[i]) ? depth[i] : max;
        }
        printf("min: %f, max: %f\n", min, max);
    }
    {
        printf("amplitude\n");
        float min = amplitude[0];
        float max = amplitude[0];
        for (int i = 0; i < width * height; i++) {
            min = (min > amplitude[i]) ? amplitude[i] : min;
            max = (max < amplitude[i]) ? amplitude[i] : max;
        }
        printf("min: %f, max: %f\n", min, max);
    }
}
