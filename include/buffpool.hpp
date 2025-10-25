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
    ~MmapBufferPool() noexcept;

    MmapBufferPool(MmapBufferPool&& other) noexcept;
    MmapBufferPool& operator=(MmapBufferPool&& other) noexcept;
    MmapBufferPool(const MmapBufferPool&) = delete;
    MmapBufferPool& operator=(const MmapBufferPool&) = delete;
    
    void* sync_start(const uint32_t index) override;
    int sync_end(const uint32_t index) override;

  private:
    // addr, length
    std::vector<std::pair<void*, uint32_t>> buffers;
};

class DmaBufferPool final : public BufferPool {
  public:
    DmaBufferPool(const char* allocator, const uint32_t num_buffers, const uint32_t length);
    ~DmaBufferPool() noexcept;

    DmaBufferPool(DmaBufferPool&& other) noexcept;
    DmaBufferPool& operator=(DmaBufferPool&& other) noexcept;
    DmaBufferPool(const DmaBufferPool&) = delete;
    DmaBufferPool& operator=(const DmaBufferPool&) = delete;

    void* sync_start(const uint32_t index) override;
    int sync_end(const uint32_t index) override;

  private:
    int fd = -1;
    // addr, length, fd
    std::vector<std::tuple<void*, uint32_t, int>> buffers;
};

} // namespace tofcam
