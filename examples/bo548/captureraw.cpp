#include <bo548.hpp>
#include <chrono>
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

int main(int argc, char** argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <device> <csi> <sensor> <directory>\n", argv[0]);
        return 0;
    }
    const char* devnode = argv[1];
    const char* csinode = argv[2];
    const char* sensornode = argv[3];
    const char* directory = argv[4];
    auto camera = tofcam::BO548(devnode, csinode, sensornode, true, true, tofcam::MemType::DMABUF, tofcam::Mode::Single);
    const auto [width, height] = camera.get_size();
    const auto [sizeimage, bytesperline] = camera.get_bytes();
    const uint32_t bytesplane = bytesperline * height;
    std::vector<std::vector<uint8_t>> rawframes;

    camera.stream_on();
    for (int i = 0; i < 8; i++) {
        const auto ptr = camera.get_rawframe();

        rawframes.emplace_back(bytesplane);
        const auto phase0 = static_cast<uint8_t*>(ptr) + bytesplane * 0;
        std::memcpy(rawframes.back().data(), phase0, bytesplane);

        rawframes.emplace_back(bytesplane);
        const auto phase1 = static_cast<uint8_t*>(ptr) + bytesplane * 1;
        std::memcpy(rawframes.back().data(), phase1, bytesplane);

        rawframes.emplace_back(bytesplane);
        const auto phase2 = static_cast<uint8_t*>(ptr) + bytesplane * 2;
        std::memcpy(rawframes.back().data(), phase2, bytesplane);

        rawframes.emplace_back(bytesplane);
        const auto phase3 = static_cast<uint8_t*>(ptr) + bytesplane * 3;
        std::memcpy(rawframes.back().data(), phase3, bytesplane);
    }
    camera.stream_off();

    for (int i = 0; i < rawframes.size(); i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/frame_%03d.bin", directory, i);
        save_bytes(path, rawframes[i].data(), bytesplane);
    }
}
