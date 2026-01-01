#pragma once

#include <camera.hpp>
#include <optional>

namespace tofcam {

class BO410 {
  public:
    BO410(const char* device, const char* subdevice, const int range, const MemType memtype = MemType::DMABUF);

    ~BO410();

    void stream_on();

    void stream_off();

    std::pair<float*, float*> get_frame();

  private:
    Camera camera;
    int subfd = -1;
    int range = 2000;
    std::vector<float> depth;
    std::vector<float> confidence;
};

} // namespace tofcam
