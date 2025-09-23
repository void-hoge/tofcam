#include "utility.hpp"

namespace tofcam {

void unpack_y12p(uint16_t *dst, const void* src, const uint32_t width, const uint32_t height, const uint32_t bytesperline) {
    const uint32_t num_pairs = width / 2;
    for (size_t y = 0; y < height; y++) {
        const uint8_t* line = static_cast<const uint8_t*>(src) + y * bytesperline;
        for (size_t x = 0; x < num_pairs; x++) {
            const uint16_t b0 = line[x * 3 + 0];
            const uint16_t b1 = line[x * 3 + 1];
            const uint16_t b2 = line[x * 3 + 2];
            const uint16_t p0 = (b0 | ((b1 & 0xf) << 8)) & 0xfff;
            const uint16_t p1 = ((b1 >> 4) | (b2 << 4)) & 0xfff;
            dst[y * height + x * 2 + 0] = p0;
            dst[y * height + x * 2 + 1] = p1;
        }
    }
}

}
