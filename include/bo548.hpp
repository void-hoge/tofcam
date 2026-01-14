#pragma once

#include <camera.hpp>
#include <optional>

namespace tofcam {

enum class Mode {
    Single,
    Double,
};

class BO548 {
  public:
    BO548(const char* device, const char* csi_device, const char* sensor_device, const bool vflip = true,
          const bool hflip = true, const int exposure = 1000, const MemType memtype = MemType::DMABUF,
          const Mode mode = Mode::Single);

    ~BO548();

    void stream_on();

    void stream_off();

    std::pair<float*, float*> get_frame();

    std::pair<uint32_t, uint32_t> get_size() const;

    void set_exposure(const int exposure);

  private:
    Camera camera;
    Mode mode;
    int csi_fd = -1;
    int sensor_fd = -1;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<float> depth;
    std::vector<float> confidence;
};

} // namespace tofcam
