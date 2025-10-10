#include "utility.hpp"
#include <cmath>
#include <cstdio>

namespace tofcam {

void unpack_y12p(int16_t *dst, const void *src,
                 const uint32_t width, const uint32_t height,
                 const uint32_t bytesperline) {
    const uint32_t num_pairs = width / 2;
    for (size_t y = 0; y < height; y++) {
        const uint8_t* line = static_cast<const uint8_t*>(src) + y * bytesperline;
        for (size_t x = 0; x < num_pairs; x++) {
            const uint16_t b0 = line[x * 3 + 0];
            const uint16_t b1 = line[x * 3 + 1];
            const uint16_t b2 = line[x * 3 + 2];
            const uint16_t p0 = (b0 << 4) | (b2 & 0x0F);
            const uint16_t p1 = (b1 << 4) | (b2 >> 4);
            dst[y * width + x * 2 + 0] = (p0 | -(p0 & 0x400));
            dst[y * width + x * 2 + 1] = (p1 | -(p1 & 0x400));
        }
    }
}

static inline float approx_atan2(const int16_t y, const int16_t x) {
    constexpr float PI = M_PI;
    constexpr float HPI = M_PI / 2;
    constexpr float ALPHA = 1.05839816339f;
    constexpr float BETA = 0.273f;
    if (x == 0 && y == 0) return 0.0f;
    const int16_t ax = std::abs(x);
    const int16_t ay = std::abs(y);
    const bool swap = (ay > ax);
    const int16_t amax = swap ? ay : ax;
    const int16_t amin = swap ? ax : ay;
    const float t = (float)amin/amax;
    const float a = t * (ALPHA - BETA * t);
    float theta = swap ? (HPI - a) : a;
    if (x < 0) theta = PI - theta;
    if (y < 0) theta = -theta;
    return theta;
}

void compute_depth_amplitude(float *depth, float *amplitude, int16_t* raw,
                             const int16_t *frame0, const int16_t *frame1,
                             const int16_t *frame2, const int16_t *frame3,
                             const uint32_t num_pixels, const float modfreq_hz) {
    constexpr float C = 3e8; // speed of light (300,000,000 m/s)
    const float range = C / (2.0f * modfreq_hz) * 1000.0f;
    const float bias = 0.5f * range;
    const float scale = bias / M_PI;

    for (size_t i = 0; i < num_pixels; i++) {
        const int16_t I0 = frame0[i];
        const int16_t I1 = frame1[i];
        const int16_t I2 = frame2[i];
        const int16_t I3 = frame3[i];
        const int16_t num = I3 - I1;
        const int16_t den = I0 - I2;
        raw[i] = I0 + I1 + I2 + I3;
        amplitude[i] = std::sqrt(den * den + num * num);
#if 0
        const float phase = std::atan2(num, den);
#else
        const float phase = approx_atan2(num, den);
#endif
        depth[i] = (phase >= M_PI || ((den == 0) && (num == 0))) ? 0.0f : phase * scale + bias;
    }
}

}
