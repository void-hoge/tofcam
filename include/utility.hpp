#pragma once

#include <cstdint>
#include <cstddef>

namespace tofcam {

void unpack_y12p(uint16_t *dst, const void* src, const uint32_t width, const uint32_t height, const uint32_t bytesperline);

}
