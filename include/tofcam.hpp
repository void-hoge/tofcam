#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <memory>
#include <buffpool.hpp>

namespace tofcam {

class Camera {
  public:
    ~Camera();

    Camera(const char* device, const uint32_t num_buffers);

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

    int fd = -1;
    uint32_t sizeimage = 0;
    uint32_t bytesperline = 0;

    std::unique_ptr<MmapBufferPool> buffers;

    void reset() noexcept;
};

} // namespace tofcam
