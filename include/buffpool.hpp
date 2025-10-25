#pragma once

#include <cstdint>
#include <vector>
#include <utility>

namespace tofcam {

class BufferPool {
public:
    virtual ~BufferPool() = default;
    virtual void* sync_start(const uint32_t i) = 0;
    virtual void sync_end(const uint32_t i) = 0;
};

class MmapBufferPool final : public BufferPool {
public:
    MmapBufferPool(const int fd, const uint32_t num_buffers);

    void* sync_start(const uint32_t i) override;
    void sync_end(const uint32_t i) override;
private:
    // addr, length
    std::vector<std::pair<void*, uint32_t>> buffers;
};

}
