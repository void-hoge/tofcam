#include <bo548.hpp>
#include <cstring>
#include <fstream>
#include <vector>

void save_bytes(const char* filename, const void* ptr, const size_t size) {
    FILE* fp = fopen(filename, "wb");
    if (fp == nullptr) {
        throw std::system_error(errno, std::generic_category(), "Failed to open the file.");
    }
    size_t written = fwrite(ptr, 1, size, fp);
    if (fclose(fp) != 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to close the file.");
    }
    if (written != size) {
        throw std::runtime_error("Failed to save.");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <device> <csi> <sensor> <directory>\n", argv[0]);
        return 0;
    }
    const char* devnode = argv[1];
    const char* csinode = argv[2];
    const char* sensornode = argv[3];
    const char* directory = argv[4];
    std::vector<std::vector<float>> depth_frames;
    std::vector<std::vector<float>> confidence_frames;
    auto camera = tofcam::BO548(devnode, csinode, sensornode, true, true, tofcam::MemType::DMABUF, tofcam::Mode::Double);
    const auto [width, height] = camera.get_size();
    camera.stream_on();
    for (int i = 0; i < 35; i++) {
        auto [depth, confidence] = camera.get_frame();
        depth_frames.emplace_back(width * height);
        std::memcpy(depth_frames.back().data(), ((float*)depth) + width * height, sizeof(float) * width * height);
        confidence_frames.emplace_back(width * height);
        std::memcpy(confidence_frames.back().data(), ((float*)confidence) + width * height, sizeof(float) * width * height);
        fprintf(stderr, "captured: %4d\n", i);
    }
    camera.stream_off();
    for (int i = 0; i < depth_frames.size(); i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/depth_%03d.bin", directory, i);
        save_bytes(path, depth_frames[i].data(), sizeof(float) * width * height);
    }
    for (int i = 0; i < confidence_frames.size(); i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/confidence_%03d.bin", directory, i);
        save_bytes(path, confidence_frames[i].data(), sizeof(float) * width * height);
    }
}
