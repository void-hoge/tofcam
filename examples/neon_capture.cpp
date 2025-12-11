#include <cstdio>
#include <cstdlib>
#include <fakecam.hpp>
#include <fstream>
#include <iostream>
#include <system_error>
#include <tofcam.hpp>
#include <utility.hpp>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <destination>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* dstdir = argv[1];
    constexpr uint32_t ITER = 60;
    constexpr int range = 2000;
    constexpr bool enableConfidence = true;
    constexpr int modfreq_hz = range == 2000 ? 75'000'000 : 37'500'000;
    auto camera = tofcam::Camera("/dev/video0", "/dev/v4l-subdev2", 8, range, tofcam::MemType::DMABUF);
    const auto [width, height] = camera.get_size();
    const auto [sizeimage, bytesperline] = camera.get_bytes();
    std::vector<std::vector<float>> depth;
    std::vector<std::vector<float>> amplitude;

    camera.stream_on();
    for (int i = 0; i < ITER; i++) {
        std::pair<void*, uint32_t> frames[4];
        for (int j = 0; j < 4; j++) {
            frames[j] = camera.dequeue();
        }
        depth.emplace_back(width * height, 0.0f);
        amplitude.emplace_back(width * height, 0.0f);
        if (range == 4000) {
            tofcam::compute_depth_confidence_from_y12p_neon<enableConfidence, tofcam::Rotation::Quarter>(
                depth.back().data(), amplitude.back().data(), frames[0].first, frames[1].first, frames[2].first,
                frames[3].first, width, height, bytesperline, modfreq_hz);
        } else {
            tofcam::compute_depth_confidence_from_y12p_neon<enableConfidence, tofcam::Rotation::Zero>(
                depth.back().data(), amplitude.back().data(), frames[0].first, frames[1].first, frames[2].first,
                frames[3].first, width, height, bytesperline, modfreq_hz);
        }
        for (const auto& [data, index] : frames) {
            camera.enqueue(index);
        }
    }
    camera.stream_off();

    fprintf(stderr, "saving frames\n");
    auto save_float_array = [&](const std::vector<float>& data, const char* filename) {
        FILE* fp = fopen(filename, "wb");
        if (fp == nullptr) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Failed to open %s", filename);
            throw std::runtime_error(msg);
        }
        fwrite(data.data(), sizeof(float), data.size(), fp);
        fclose(fp);
    };
    for (int i = 0; i < ITER; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/depth_%03d.bin", dstdir, i);
        save_float_array(depth[i], path);
        snprintf(path, sizeof(path), "%s/amplitude_%03d.bin", dstdir, i);
        save_float_array(amplitude[i], path);
    }
}
