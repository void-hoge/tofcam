#pragma once

#include <vector>
#include <utility>
#include <cstdint>

namespace tofcam {
class FakeCamera {
public:
    ~FakeCamera() = default;

    FakeCamera(const char* dir, const uint32_t max_frames);

    void stream_on();

    void stream_off();

    std::pair<void*, uint32_t> dequeue();

    void enqueue(const uint32_t index);

    // {width, height}
    std::pair<uint32_t, uint32_t> get_size() const;

    // {sizeimage, bytesperline}
    std::pair<uint32_t, uint32_t> get_bytes() const;

private:
    static constexpr uint32_t WIDTH = 240;
    static constexpr uint32_t HEIGHT = 180;
    static constexpr uint32_t BYTESPERLINE = 368;
    static constexpr uint32_t SIZEIMAGE = BYTESPERLINE * HEIGHT;

    uint32_t index = 0;

    std::vector<std::vector<uint8_t>> frames;

    void reset() noexcept;

    void load_frames(const char* dir, const uint32_t max_frames);
};

}
