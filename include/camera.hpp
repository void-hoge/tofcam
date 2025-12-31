#pragma once

#include <buffpool.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace tofcam {

enum class MemType {
    MMAP,
    DMABUF,
};

class Camera {
  public:
    Camera(const char* device, const uint32_t num_buffers, const MemType memtype = MemType::MMAP);
    ~Camera() noexcept;

    Camera(Camera&& other) noexcept;
    Camera& operator=(Camera&& other) noexcept;
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    void stream_on();

    void stream_off();

    std::pair<void*, uint32_t> dequeue();

    void enqueue(const uint32_t index);

    // {width, height}
    std::pair<uint32_t, uint32_t> get_size() const;

    // {sizeimage, bytesperline}
    std::pair<uint32_t, uint32_t> get_bytes() const;

    uint32_t get_format() const;

  private:
    uint32_t memorytype;
    int fd = -1;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t sizeimage = 0;
    uint32_t bytesperline = 0;
    uint32_t pixelformat = 0;

    std::unique_ptr<BufferPool> buffers;
};

} // namespace tofcam
