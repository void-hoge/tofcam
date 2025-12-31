#pragma once

#include <camera.hpp>
#include <optional>

namespace tofcam {

class BO548 {
  public:
    BO548(const char* device, const MemType memtype = MemType::MMAP);

    ~BO548();

    void stream_on();

    void stream_off();

    std::pair<float*, float*> get_frame();

    std::pair<uint32_t, uint32_t> get_size() const;

  private:
    Camera camera;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<float> depth;
    std::vector<float> confidence;
};

} // namespace tofcam
