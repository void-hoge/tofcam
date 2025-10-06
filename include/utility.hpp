#pragma once

#include <cstdint>
#include <cstddef>

namespace tofcam {

void unpack_y12p(int16_t *dst, const void *src,
                 const uint32_t width, const uint32_t height,
                 const uint32_t bytesperline);

void compute_depth_amplitude(float *depth, float *amplitude, int16_t *raw,
                             const int16_t* frame0, const int16_t *frame1,
                             const int16_t *frame2, const int16_t *frame3,
                             const uint32_t num_pixels, const float modfreq_hz);

}
