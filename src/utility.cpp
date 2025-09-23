#include "utility.hpp"
#include <cmath>

namespace tofcam {

void unpack_y12p(uint16_t *dst, const void *src,
                 const uint32_t width, const uint32_t height,
                 const uint32_t bytesperline) {
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

void compute_depth_amplitude(float *depth, float *amplitude,
                             const uint16_t* frame0, const uint16_t *frame1,
                             const uint16_t *frame2, const uint16_t *frame3,
                             const uint32_t num_pixels, const float modfreq_hz) {
    constexpr float C = 3e8; // speed of light (300,000,000 m/s)

    const float factor = C / (4.0f * static_cast<float>(M_PI) * modfreq_hz);

    for (size_t i = 0; i < num_pixels; i++) {
        const int16_t I0 = static_cast<int16_t>(frame0[i]);
        const int16_t I1 = static_cast<int16_t>(frame1[i]);
        const int16_t I2 = static_cast<int16_t>(frame2[i]);
        const int16_t I3 = static_cast<int16_t>(frame3[i]);
        const int32_t num = static_cast<int32_t>(I3 - I1);
        const int32_t den = static_cast<int32_t>(I0 - I2);
        amplitude[i] = std::sqrt(den * den + num * num);
        const float phase = std::atan2(num, den);
        depth[i] = phase;
    }
}

}
