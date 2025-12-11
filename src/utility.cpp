#include "utility.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <numbers>

// #define NEON_APPROX_DIV

namespace tofcam {

void unpack_y12p(int16_t* dst, const void* src, const uint32_t width, const uint32_t height, const uint32_t bytesperline) {
    const uint32_t num_pairs = width / 2;
    for (uint32_t y = 0; y < height; y++) {
        const uint8_t* line = static_cast<const uint8_t*>(src) + y * bytesperline;
        for (uint32_t x = 0; x < num_pairs; x++) {
            const uint16_t b0 = line[x * 3 + 0];
            const uint16_t b1 = line[x * 3 + 1];
            const uint16_t b2 = line[x * 3 + 2];
            const uint16_t p0 = (b0 << 4) | (b2 & 0x0F);
            const uint16_t p1 = (b1 << 4) | (b2 >> 4);
            dst[y * width + x * 2 + 0] = (int16_t)(p0 << 5) >> 5;
            dst[y * width + x * 2 + 1] = (int16_t)(p1 << 5) >> 5;
        }
    }
}

static inline float approx_atan2(const int16_t y, const int16_t x) {
    constexpr float PI = std::numbers::pi_v<float>;
    constexpr float HPI = std::numbers::pi_v<float> / 2;
    constexpr float QPI = std::numbers::pi_v<float> / 4;
    constexpr float R = 0.273f;
    if (x == 0 && y == 0)
        return 0.0f;
    const int16_t ax = std::abs(int(x));
    const int16_t ay = std::abs(int(y));
    const bool swap = (ay > ax);
    const int16_t amax = swap ? ay : ax;
    const int16_t amin = swap ? ax : ay;
    const float t = float(amin) / float(amax);
    const float a = t * (QPI + R - R * t);
    float theta = swap ? (HPI - a) : a;
    if (x < 0)
        theta = PI - theta;
    if (y < 0)
        theta = -theta;
    return theta;
}

template <bool EnableConfidence, Rotation rotation>
void compute_depth_confidence(
        float* depth, float* confidence, const int16_t* frame0, const int16_t* frame1, const int16_t* frame2,
        const int16_t* frame3, const uint32_t num_pixels, const float modfreq_hz) {
    static constexpr float C = 3e8;
    const float range = C / (2.0f * modfreq_hz) * 1000.0f;
    const float bias = 0.5f * range;
    const float scale = bias * std::numbers::inv_pi_v<float>;

    for (uint32_t i = 0; i < num_pixels; i++) {
        const int16_t I0 = frame0[i];
        const int16_t I1 = frame1[i];
        const int16_t I2 = frame2[i];
        const int16_t I3 = frame3[i];
        const int16_t sin = I3 - I1;
        const int16_t cos = I0 - I2;
        if constexpr (EnableConfidence) {
            confidence[i] = std::sqrt(float(cos) * cos + float(sin) * sin) * 8.0f;
        }
        int16_t y, x;
        if constexpr (rotation == Rotation::Zero) {
            y = sin;
            x = cos;
        } else if constexpr (rotation == Rotation::Quarter) {
            y = cos;
            x = -sin;
        } else if constexpr (rotation == Rotation::Half) {
            y = -sin;
            x = -cos;
        } else {
            y = -cos;
            x = sin;
        }
#if 0
        const float phase = std::atan2(y, x);
#else
        const float phase = approx_atan2(y, x);
#endif
        depth[i] = (phase >= std::numbers::pi_v<float> || ((y == 0) && (x == 0))) ? 0.0f : phase * scale + bias;
    }
}

template void compute_depth_confidence<true, Rotation::Zero>(
        float*, float*, const int16_t*, const int16_t*, const int16_t*, const int16_t*, const uint32_t, const float);
template void compute_depth_confidence<true, Rotation::Quarter>(
        float*, float*, const int16_t*, const int16_t*, const int16_t*, const int16_t*, const uint32_t, const float);
template void compute_depth_confidence<true, Rotation::Half>(
        float*, float*, const int16_t*, const int16_t*, const int16_t*, const int16_t*, const uint32_t, const float);
template void compute_depth_confidence<true, Rotation::ThreeQuarters>(
        float*, float*, const int16_t*, const int16_t*, const int16_t*, const int16_t*, const uint32_t, const float);
template void compute_depth_confidence<false, Rotation::Zero>(
        float*, float*, const int16_t*, const int16_t*, const int16_t*, const int16_t*, const uint32_t, const float);
template void compute_depth_confidence<false, Rotation::Quarter>(
        float*, float*, const int16_t*, const int16_t*, const int16_t*, const int16_t*, const uint32_t, const float);
template void compute_depth_confidence<false, Rotation::Half>(
        float*, float*, const int16_t*, const int16_t*, const int16_t*, const int16_t*, const uint32_t, const float);
template void compute_depth_confidence<false, Rotation::ThreeQuarters>(
        float*, float*, const int16_t*, const int16_t*, const int16_t*, const int16_t*, const uint32_t, const float);

#if defined(__ARM_NEON)

static inline void unpack_y12p_s16x8x2(const uint8x8x3_t& b, int16x8_t& p0, int16x8_t& p1) {
    // p0 = (b0 << 4) | (b2 & 0x0F);
    uint8x8_t b2lo = vand_u8(b.val[2], vdup_n_u8(0x0F));
    uint16x8_t p0u = vorrq_u16(vshll_n_u8(b.val[0], 4), vmovl_u8(b2lo));
    // p1 = (b1 << 4) | (b2 >> 4);
    uint8x8_t b2hi = vshr_n_u8(b.val[2], 4);
    uint16x8_t p1u = vorrq_u16(vshll_n_u8(b.val[1], 4), vmovl_u8(b2hi));
    // sign extension
    p0 = vshrq_n_s16(vshlq_n_s16(vreinterpretq_s16_u16(p0u), 5), 5);
    p1 = vshrq_n_s16(vshlq_n_s16(vreinterpretq_s16_u16(p1u), 5), 5);
}

static inline void approx_atan2x8(const int16x8_t& y, const int16x8_t& x, float32x4_t& thetalo, float32x4_t& thetahi) {
    const float32x4_t vPI = vdupq_n_f32(1.0f);
    const float32x4_t vHalfPI = vdupq_n_f32(0.5f);
    const float32x4_t vQuadPI = vdupq_n_f32(0.25f);
    const float32x4_t vR = vdupq_n_f32(0.273 * std::numbers::inv_pi_v<float>);
    const float32x4_t vT = vaddq_f32(vQuadPI, vR);
    const int16x8_t vZ = vdupq_n_s16(0);

    const int16x8_t ay = vabsq_s16(y);
    const int16x8_t ax = vabsq_s16(x);

    const uint16x8_t swap = vcgtq_s16(ay, ax);
    const int16x8_t amax = vbslq_s16(swap, ay, ax);
    const int16x8_t amin = vbslq_s16(swap, ax, ay);

    const float32x4_t amaxlo = vcvtq_f32_s32(vmovl_s16(vget_low_s16(amax)));
    const float32x4_t amaxhi = vcvtq_f32_s32(vmovl_s16(vget_high_s16(amax)));
    const float32x4_t aminlo = vcvtq_f32_s32(vmovl_s16(vget_low_s16(amin)));
    const float32x4_t aminhi = vcvtq_f32_s32(vmovl_s16(vget_high_s16(amin)));

#if defined(NEON_APPROX_DIV)
    float32x4_t rlo = vrecpeq_f32(amaxlo);
    rlo = vmulq_f32(vrecpsq_f32(amaxlo, rlo), rlo);
    float32x4_t rhi = vrecpeq_f32(amaxhi);
    rhi = vmulq_f32(vrecpsq_f32(amaxhi, rhi), rhi);
    const float32x4_t tlo = vmulq_f32(aminlo, rlo);
    const float32x4_t thi = vmulq_f32(aminhi, rhi);
#else
    const float32x4_t tlo = vdivq_f32(aminlo, amaxlo);
    const float32x4_t thi = vdivq_f32(aminhi, amaxhi);
#endif

    const float32x4_t alo = vmulq_f32(tlo, vfmsq_f32(vT, vR, tlo));
    const float32x4_t ahi = vmulq_f32(thi, vfmsq_f32(vT, vR, thi));

    const uint32x4_t swaplo = vmovl_u16(vget_low_u16(swap));
    const uint32x4_t swaphi = vmovl_u16(vget_high_u16(swap));
    thetalo = vbslq_f32(vcgtq_u32(swaplo, vdupq_n_u32(0)), vsubq_f32(vHalfPI, alo), alo);
    thetahi = vbslq_f32(vcgtq_u32(swaphi, vdupq_n_u32(0)), vsubq_f32(vHalfPI, ahi), ahi);

    const uint16x8_t xneg = vcltq_s16(x, vZ);
    const uint32x4_t xneglo = vmovl_u16(vget_low_u16(xneg));
    const uint32x4_t xneghi = vmovl_u16(vget_high_u16(xneg));
    thetalo = vbslq_f32(vcgtq_u32(xneglo, vdupq_n_u32(0)), vsubq_f32(vPI, thetalo), thetalo);
    thetahi = vbslq_f32(vcgtq_u32(xneghi, vdupq_n_u32(0)), vsubq_f32(vPI, thetahi), thetahi);

    const uint16x8_t yneg = vcltq_s16(y, vZ);
    const uint32x4_t yneglo = vmovl_u16(vget_low_u16(yneg));
    const uint32x4_t yneghi = vmovl_u16(vget_high_u16(yneg));
    thetalo = vbslq_f32(vcgtq_u32(yneglo, vdupq_n_u32(0)), vnegq_f32(thetalo), thetalo);
    thetahi = vbslq_f32(vcgtq_u32(yneghi, vdupq_n_u32(0)), vnegq_f32(thetahi), thetahi);

    const uint16x8_t xzero = vceqq_s16(x, vZ);
    const uint16x8_t yzero = vceqq_s16(y, vZ);
    const uint16x8_t bzero = vandq_u16(xzero, yzero);
    const uint32x4_t bzerolo = vmovl_u16(vget_low_u16(bzero));
    const uint32x4_t bzerohi = vmovl_u16(vget_high_u16(bzero));
    thetalo = vbslq_f32(vorrq_u32(vcgtq_u32(bzerolo, vdupq_n_u32(0)), vcgeq_f32(thetalo, vPI)), vnegq_f32(vPI), thetalo);
    thetahi = vbslq_f32(vorrq_u32(vcgtq_u32(bzerohi, vdupq_n_u32(0)), vcgeq_f32(thetahi, vPI)), vnegq_f32(vPI), thetahi);
}

template <bool EnableConfidence, Rotation rotation>
void compute_depth_confidence_from_y12p_neon(
        float* depth, float* confidence, const void* frame0, const void* frame1, const void* frame2, const void* frame3,
        const uint32_t width, const uint32_t height, const uint32_t bytesperline, const float modfreq_hz) {
    static constexpr float C = 3e8;
    const float range = C / (2.0f * modfreq_hz) * 1000.0f;
    const float bias = 0.5f * range;
    const float scale = bias;
    const float32x4_t vBias = vdupq_n_f32(bias);
    const float32x4_t vScale = vdupq_n_f32(scale);
    const float32x4_t vConfScale = vdupq_n_f32(8.0f);

    for (uint32_t y = 0; y < height; y++) {
        const uint8_t* line0 = static_cast<const uint8_t*>(frame0) + y * bytesperline;
        const uint8_t* line1 = static_cast<const uint8_t*>(frame1) + y * bytesperline;
        const uint8_t* line2 = static_cast<const uint8_t*>(frame2) + y * bytesperline;
        const uint8_t* line3 = static_cast<const uint8_t*>(frame3) + y * bytesperline;

        for (uint32_t x = 0; x < width; x += 16) {
            const uint32_t offset = x / 2 * 3;
            const uint8x8x3_t b0 = vld3_u8(line0 + offset);
            const uint8x8x3_t b2 = vld3_u8(line2 + offset);
            const uint8x8x3_t b3 = vld3_u8(line3 + offset);
            const uint8x8x3_t b1 = vld3_u8(line1 + offset);
            int16x8_t p0[2], p1[2], p2[2], p3[2];
            unpack_y12p_s16x8x2(b0, p0[0], p0[1]);
            unpack_y12p_s16x8x2(b1, p1[0], p1[1]);
            unpack_y12p_s16x8x2(b2, p2[0], p2[1]);
            unpack_y12p_s16x8x2(b3, p3[0], p3[1]);
            float32x4x2_t depthlo;
            float32x4x2_t depthhi;
            float32x4x2_t amplo;
            float32x4x2_t amphi;
            for (uint32_t i = 0; i < 2; i++) {
                const int16x8_t cos = vsubq_s16(p0[i], p2[i]);
                const int16x8_t sin = vsubq_s16(p3[i], p1[i]);
                int16x8_t vy, vx;
                if constexpr (rotation == Rotation::Zero) {
                    vy = sin;
                    vx = cos;
                } else if constexpr (rotation == Rotation::Quarter) {
                    vy = cos;
                    vx = vnegq_s16(sin);
                } else if constexpr (rotation == Rotation::Half) {
                    vy = vnegq_s16(sin);
                    vx = vnegq_s16(cos);
                } else {
                    vy = vnegq_s16(cos);
                    vx = sin;
                }
                approx_atan2x8(vy, vx, depthlo.val[i], depthhi.val[i]);
                depthlo.val[i] = vfmaq_f32(vBias, depthlo.val[i], vScale);
                depthhi.val[i] = vfmaq_f32(vBias, depthhi.val[i], vScale);
                if constexpr (EnableConfidence) {
                    const float32x4_t xlo = vcvtq_f32_s32(vmovl_s16(vget_low_s16(vx)));
                    const float32x4_t xhi = vcvtq_f32_s32(vmovl_s16(vget_high_s16(vx)));
                    const float32x4_t ylo = vcvtq_f32_s32(vmovl_s16(vget_low_s16(vy)));
                    const float32x4_t yhi = vcvtq_f32_s32(vmovl_s16(vget_high_s16(vy)));
                    amplo.val[i] = vmulq_f32(vsqrtq_f32(vaddq_f32(vmulq_f32(xlo, xlo), vmulq_f32(ylo, ylo))), vConfScale);
                    amphi.val[i] = vmulq_f32(vsqrtq_f32(vaddq_f32(vmulq_f32(xhi, xhi), vmulq_f32(yhi, yhi))), vConfScale);
                }
            }
            vst2q_f32(depth + y * width + x + 0, depthlo);
            vst2q_f32(depth + y * width + x + 8, depthhi);
            if constexpr (EnableConfidence) {
                vst2q_f32(confidence + y * width + x + 0, amplo);
                vst2q_f32(confidence + y * width + x + 8, amphi);
            }
        }
    }
}

template void compute_depth_confidence_from_y12p_neon<true, Rotation::Zero>(
        float*, float*, const void*, const void*, const void*, const void*, const uint32_t, const uint32_t, const uint32_t,
        const float);
template void compute_depth_confidence_from_y12p_neon<true, Rotation::Quarter>(
        float*, float*, const void*, const void*, const void*, const void*, const uint32_t, const uint32_t, const uint32_t,
        const float);
template void compute_depth_confidence_from_y12p_neon<true, Rotation::Half>(
        float*, float*, const void*, const void*, const void*, const void*, const uint32_t, const uint32_t, const uint32_t,
        const float);
template void compute_depth_confidence_from_y12p_neon<true, Rotation::ThreeQuarters>(
        float*, float*, const void*, const void*, const void*, const void*, const uint32_t, const uint32_t, const uint32_t,
        const float);
template void compute_depth_confidence_from_y12p_neon<false, Rotation::Zero>(
        float*, float*, const void*, const void*, const void*, const void*, const uint32_t, const uint32_t, const uint32_t,
        const float);
template void compute_depth_confidence_from_y12p_neon<false, Rotation::Quarter>(
        float*, float*, const void*, const void*, const void*, const void*, const uint32_t, const uint32_t, const uint32_t,
        const float);
template void compute_depth_confidence_from_y12p_neon<false, Rotation::Half>(
        float*, float*, const void*, const void*, const void*, const void*, const uint32_t, const uint32_t, const uint32_t,
        const float);
template void compute_depth_confidence_from_y12p_neon<false, Rotation::ThreeQuarters>(
        float*, float*, const void*, const void*, const void*, const void*, const uint32_t, const uint32_t, const uint32_t,
        const float);

#endif

} // namespace tofcam
