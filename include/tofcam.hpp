#pragma once

#include <vector>
#include <utility>
#include <cstdint>

namespace tofcam {

class Camera {
public:
    ~Camera();

    Camera(const char* device, const uint32_t buffer_count);

    void stream_on();

    void stream_off();

    std::pair<uint32_t, uint32_t> dequeue();

    void enqueue(const uint32_t index);
private:
    int fd = -1;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t sizeimage = 0;
    std::vector<std::pair<void*, uint32_t>> buffers;

    void reset() noexcept;
};

}
