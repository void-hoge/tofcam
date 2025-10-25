#pragma once

#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

namespace tofcam {

class BufferPool {
  public:
    virtual ~BufferPool() = default;

    virtual void* sync_start(const uint32_t index) = 0;
    virtual int sync_end(const uint32_t index) = 0;
};

class MmapBufferPool final : public BufferPool {
  public:
    MmapBufferPool(const int fd, const uint32_t num_buffers);

    void* sync_start(const uint32_t index) override;
    int sync_end(const uint32_t index) override;

  private:
    // addr, length
    std::vector<std::pair<void*, uint32_t>> buffers;
};

class DmaBufferPool final : public BufferPool {
  public:
    DmaBufferPool(const char* allocator, const uint32_t num_buffers, const uint32_t length);

    void* sync_start(const uint32_t index) override;
    int sync_end(const uint32_t index) override;

  private:
    int fd = -1;
    // addr, length, fd
    std::vector<std::tuple<void*, uint32_t, int>> buffers;
};

} // namespace tofcam
