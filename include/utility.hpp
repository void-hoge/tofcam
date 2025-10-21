#pragma once

#include <cstdint>
#include <cstddef>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

namespace tofcam {

void unpack_y12p(
    int16_t *dst, const void *src,
    const uint32_t width, const uint32_t height,
    const uint32_t bytesperline);

template<bool EnableConfidence=true>
void compute_depth_confidence(
    float *depth, float *confidence,
    const int16_t *frame0, const int16_t *frame1,
    const int16_t *frame2, const int16_t *frame3,
    const uint32_t num_pixels, const float modfreq_hz);

#if defined(__ARM_NEON)

template<bool EnableConfidence=true>
void compute_depth_confidence_from_y12p_neon(
    float *depth, float *confidence,
    const void *frame0, const void *frame1, const void *frame2, const void *frame3,
    const uint32_t width, const uint32_t height, const uint32_t bytesperline,
    const float modfreq_hz);

#endif

}
