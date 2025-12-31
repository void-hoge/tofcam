#include <bo548.hpp>
#include <cstring>
#include <vector>
#include <fstream>

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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <directory>\n", argv[0]);
        return 0;
    }
    const char* directory = argv[1];
    std::vector<std::vector<float>> depth_frames;
    std::vector<std::vector<float>> confidence_frames;
    auto camera = tofcam::BO548("/dev/video0", tofcam::MemType::MMAP);
    const auto [width, height] = camera.get_size();
    camera.stream_on();
    for (int i = 0; i < 30; i++) {
        auto [depth, confidence] = camera.get_frame();
        depth_frames.emplace_back(std::vector<float>(width * height));
        std::memcpy(depth_frames.back().data(), depth, sizeof(float) * width * height);
        confidence_frames.emplace_back(std::vector<float>(width * height));
        std::memcpy(confidence_frames.back().data(), confidence, sizeof(float) * width * height);
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
