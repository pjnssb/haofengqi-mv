#pragma once

#include <algorithm>
#include <cairo/cairo.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <omp.h>
#include <vector>

namespace mv {

constexpr int W = 1920;
constexpr int H = 1080;
constexpr int BAR_H = 138;  // 1920 x 804 => 2.388:1 CinemaScope active frame
constexpr int AW = 960;
constexpr int AH = 540;
constexpr int BW = 480;
constexpr int BH = 270;

struct Vec3 {
    float r = 0.0f, g = 0.0f, b = 0.0f;

    Vec3 operator+(const Vec3& o) const { return {r + o.r, g + o.g, b + o.b}; }
    Vec3 operator-(const Vec3& o) const { return {r - o.r, g - o.g, b - o.b}; }
    Vec3 operator*(float s) const { return {r * s, g * s, b * s}; }
    Vec3 operator*(const Vec3& o) const { return {r * o.r, g * o.g, b * o.b}; }
    Vec3& operator+=(const Vec3& o) { r += o.r; g += o.g; b += o.b; return *this; }
    Vec3& operator*=(float s) { r *= s; g *= s; b *= s; return *this; }
};

inline Vec3 mix3(const Vec3& a, const Vec3& b, float t) {
    float u = std::clamp(t, 0.0f, 1.0f);
    return {a.r + (b.r - a.r) * u, a.g + (b.g - a.g) * u, a.b + (b.b - a.b) * u};
}

inline float clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
inline float smoothstep(float a, float b, float x) {
    float t = clamp01((x - a) / (b - a + 1e-8f));
    return t * t * (3.0f - 2.0f * t);
}

// Fast ACES Filmic tone mapping curve (Narkowicz 2015)
inline float aces_film(float x) {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    float v = (x * (a * x + b)) / (x * (c * x + d) + e);
    return clamp01(v);
}

inline uint32_t hash_u32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

class HdrCanvas {
public:
    std::vector<Vec3> atmo;      // AW x AH linear HDR sky/ocean/environment
    std::vector<Vec3> bloom_a;   // BW x BH
    std::vector<Vec3> bloom_b;   // BW x BH
    std::vector<Vec3> streak_a;  // BW x BH horizontal anamorphic flare
    std::vector<uint8_t> rgb_out;

    cairo_surface_t* scene_surf = nullptr;
    cairo_t* scene_cr = nullptr;

    cairo_surface_t* glow_surf = nullptr;    // Half-res AW x AH emissive lights
    cairo_t* glow_cr = nullptr;

    cairo_surface_t* overlay_surf = nullptr; // Full-res 1920 x 1080 typography & cinema matte
    cairo_t* overlay_cr = nullptr;

    float srgb_to_lin[256]{};
    uint8_t lin_to_srgb[4096]{};
    std::vector<float> kern6;
    std::vector<float> kern11;
    std::vector<float> kern_streak;

    HdrCanvas()
        : atmo(AW * AH),
          bloom_a(BW * BH),
          bloom_b(BW * BH),
          streak_a(BW * BH),
          rgb_out(W * H * 3) {
        scene_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
        scene_cr = cairo_create(scene_surf);

        glow_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, AW, AH);
        glow_cr = cairo_create(glow_surf);

        overlay_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
        overlay_cr = cairo_create(overlay_surf);

        for (int i = 0; i < 256; ++i) {
            float s = i / 255.0f;
            srgb_to_lin[i] = (s <= 0.04045f) ? (s / 12.92f) : std::pow((s + 0.055f) / 1.055f, 2.4f);
        }
        for (int i = 0; i < 4096; ++i) {
            float l = i / 4095.0f;
            float s = (l <= 0.0031308f) ? (12.92f * l) : (1.055f * std::pow(l, 1.0f / 2.4f) - 0.055f);
            lin_to_srgb[i] = static_cast<uint8_t>(std::clamp(std::lround(s * 255.0f), 0L, 255L));
        }

        auto make_gauss = [](int r) {
            std::vector<float> k(2 * r + 1);
            float sum = 0.0f;
            for (int i = -r; i <= r; ++i) {
                float w = std::exp(-0.5f * (i * i) / (0.42f * r * r));
                k[i + r] = w;
                sum += w;
            }
            for (float& w : k) w /= sum;
            return k;
        };
        kern6 = make_gauss(6);
        kern11 = make_gauss(11);

        // Streak kernel step 2 from -32..32
        float ssum = 0.0f;
        for (int k = -32; k <= 32; k += 2) {
            float w = std::exp(-std::abs(k) * 0.088f);
            kern_streak.push_back(w);
            ssum += w;
        }
        for (float& w : kern_streak) w /= ssum;
    }

    ~HdrCanvas() {
        if (scene_cr) cairo_destroy(scene_cr);
        if (scene_surf) cairo_surface_destroy(scene_surf);
        if (glow_cr) cairo_destroy(glow_cr);
        if (glow_surf) cairo_surface_destroy(glow_surf);
        if (overlay_cr) cairo_destroy(overlay_cr);
        if (overlay_surf) cairo_surface_destroy(overlay_surf);
    }

    void clear_layers() {
        std::fill(atmo.begin(), atmo.end(), Vec3{});

        auto clear_cairo = [](cairo_t* cr) {
            cairo_save(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
            cairo_paint(cr);
            cairo_restore(cr);
        };
        clear_cairo(scene_cr);
        clear_cairo(glow_cr);
        clear_cairo(overlay_cr);
    }

    void compose(int frame_idx, float exposure, float grain_amt, float flare_amt, float picture_fade) {
        cairo_surface_flush(scene_surf);
        cairo_surface_flush(glow_surf);
        cairo_surface_flush(overlay_surf);

        const uint8_t* scene_px = cairo_image_surface_get_data(scene_surf);
        const int scene_stride = cairo_image_surface_get_stride(scene_surf);
        const uint8_t* glow_px = cairo_image_surface_get_data(glow_surf);
        const int glow_stride = cairo_image_surface_get_stride(glow_surf);
        const uint8_t* ov_px = cairo_image_surface_get_data(overlay_surf);
        const int ov_stride = cairo_image_surface_get_stride(overlay_surf);

        const int by_min = BAR_H / 4;
        const int by_max = BH - BAR_H / 4;

        // 1. Downsample bright HDR atmo + emissive Cairo glow into BW x BH bloom buffer
        #pragma omp parallel for schedule(static)
        for (int y = by_min; y < by_max; ++y) {
            int ay0 = y * 2;
            const uint8_t* g0 = glow_px + ay0 * glow_stride;
            const uint8_t* g1 = glow_px + (ay0 + 1) * glow_stride;
            for (int x = 0; x < BW; ++x) {
                int ax0 = x * 2;
                Vec3 sum = atmo[ay0 * AW + ax0] + atmo[ay0 * AW + ax0 + 1] +
                           atmo[(ay0 + 1) * AW + ax0] + atmo[(ay0 + 1) * AW + ax0 + 1];
                sum *= 0.25f;

                float lum = 0.2126f * sum.r + 0.7152f * sum.g + 0.0722f * sum.b;
                Vec3 bright{};
                if (lum > 0.68f) {
                    float scale = (lum - 0.68f) / (lum + 1e-4f);
                    bright = sum * (scale * 0.85f);
                }

                Vec3 gsum{};
                const uint8_t* rows[2] = {g0, g1};
                for (int dy = 0; dy < 2; ++dy) {
                    for (int dx = 0; dx < 2; ++dx) {
                        const uint8_t* p = rows[dy] + (ax0 + dx) * 4;
                        if (p[3] > 0) {
                            gsum.b += srgb_to_lin[p[0]] * 2.1f;
                            gsum.g += srgb_to_lin[p[1]] * 2.1f;
                            gsum.r += srgb_to_lin[p[2]] * 2.1f;
                        }
                    }
                }
                bright += gsum * 0.25f;
                bloom_a[y * BW + x] = bright;
                streak_a[y * BW + x] = (lum > 1.25f) ? (bright * 0.55f) : (gsum * 0.16f);
            }
        }

        // 2. Fast precomputed separable Gaussian blur passes (inside active picture band)
        auto blur_pass = [&](int radius, const std::vector<float>& kern) {
            #pragma omp parallel for schedule(static)
            for (int y = by_min; y < by_max; ++y) {
                const Vec3* row = bloom_a.data() + y * BW;
                Vec3* dst_row = bloom_b.data() + y * BW;
                for (int x = 0; x < BW; ++x) {
                    Vec3 acc{};
                    for (int k = -radius; k <= radius; ++k) {
                        int sx = std::clamp(x + k, 0, BW - 1);
                        acc += row[sx] * kern[k + radius];
                    }
                    dst_row[x] = acc;
                }
            }
            #pragma omp parallel for schedule(static)
            for (int y = by_min; y < by_max; ++y) {
                Vec3* dst_row = bloom_a.data() + y * BW;
                for (int x = 0; x < BW; ++x) {
                    Vec3 acc{};
                    for (int k = -radius; k <= radius; ++k) {
                        int sy = std::clamp(y + k, by_min, by_max - 1);
                        acc += bloom_b[sy * BW + x] * kern[k + radius];
                    }
                    dst_row[x] = acc;
                }
            }
        };
        blur_pass(6, kern6);
        blur_pass(11, kern11);

        // 3. Horizontal anamorphic lens streak blur
        if (flare_amt > 0.01f) {
            #pragma omp parallel for schedule(static)
            for (int y = by_min; y < by_max; ++y) {
                const Vec3* row = streak_a.data() + y * BW;
                Vec3* dst_row = bloom_b.data() + y * BW;
                for (int x = 0; x < BW; ++x) {
                    Vec3 acc{};
                    int idx = 0;
                    for (int k = -32; k <= 32; k += 2) {
                        int sx = std::clamp(x + k, 0, BW - 1);
                        acc += row[sx] * kern_streak[idx++];
                    }
                    dst_row[x] = acc;
                }
            }
            #pragma omp parallel for schedule(static)
            for (int y = by_min; y < by_max; ++y) {
                int y0 = std::max(by_min, y - 1);
                int y1 = std::min(by_max - 1, y + 1);
                for (int x = 0; x < BW; ++x) {
                    streak_a[y * BW + x] = (bloom_b[y0 * BW + x] * 0.25f +
                                            bloom_b[y * BW + x] * 0.50f +
                                            bloom_b[y1 * BW + x] * 0.25f) * flare_amt;
                }
            }
        }

        // 4. Full-res 1920 x 1080 HDR compositing + ACES + sRGB + refined film grain
        const float inv_w = 1.0f / static_cast<float>(W);
        const float inv_h = 1.0f / static_cast<float>(H);
        const uint32_t frame_seed = hash_u32(static_cast<uint32_t>(frame_idx) * 747796405U + 2891336453U);

        #pragma omp parallel for schedule(static)
        for (int y = 0; y < H; ++y) {
            const bool in_matte = (y < BAR_H || y >= H - BAR_H);
            const uint8_t* sc_row = scene_px + y * scene_stride;
            const uint8_t* ov_row = ov_px + y * ov_stride;
            uint8_t* dst = rgb_out.data() + y * W * 3;

            float ayf = (y + 0.5f) * 0.5f - 0.5f;
            int ay0 = std::clamp(static_cast<int>(std::floor(ayf)), 0, AH - 1);
            int ay1 = std::min(ay0 + 1, AH - 1);
            float fy = std::clamp(ayf - ay0, 0.0f, 1.0f);

            float byf = (y + 0.5f) * 0.25f - 0.5f;
            int by0 = std::clamp(static_cast<int>(std::floor(byf)), 0, BH - 1);
            int by1 = std::min(by0 + 1, BH - 1);
            float gfy = std::clamp(byf - by0, 0.0f, 1.0f);

            float ny = (y - H * 0.5f) * inv_h * 2.0f;

            for (int x = 0; x < W; ++x) {
                Vec3 col{};
                bool add_grain = false;
                if (!in_matte) {
                    add_grain = true;
                    float axf = (x + 0.5f) * 0.5f - 0.5f;
                    int ax0 = std::clamp(static_cast<int>(std::floor(axf)), 0, AW - 1);
                    int ax1 = std::min(ax0 + 1, AW - 1);
                    float fx = std::clamp(axf - ax0, 0.0f, 1.0f);

                    const Vec3& a00 = atmo[ay0 * AW + ax0];
                    const Vec3& a10 = atmo[ay0 * AW + ax1];
                    const Vec3& a01 = atmo[ay1 * AW + ax0];
                    const Vec3& a11 = atmo[ay1 * AW + ax1];
                    col = mix3(mix3(a00, a10, fx), mix3(a01, a11, fx), fy);

                    const uint8_t* g_px = glow_px + ay0 * glow_stride + ax0 * 4;
                    if (g_px[3] > 0) {
                        col.r += srgb_to_lin[g_px[2]] * 1.15f;
                        col.g += srgb_to_lin[g_px[1]] * 1.15f;
                        col.b += srgb_to_lin[g_px[0]] * 1.15f;
                    }

                    const uint8_t* sp = sc_row + x * 4;
                    uint8_t sa = sp[3];
                    if (sa > 0) {
                        float alpha = sa * (1.0f / 255.0f);
                        float inv_a = 1.0f - alpha;
                        float sr = srgb_to_lin[sp[2]];
                        float sg = srgb_to_lin[sp[1]];
                        float sb = srgb_to_lin[sp[0]];
                        if (sa < 255) {
                            float inv_alpha = 255.0f / static_cast<float>(sa);
                            int ir = std::min(255, static_cast<int>(sp[2] * inv_alpha));
                            int ig = std::min(255, static_cast<int>(sp[1] * inv_alpha));
                            int ib = std::min(255, static_cast<int>(sp[0] * inv_alpha));
                            sr = srgb_to_lin[ir] * alpha;
                            sg = srgb_to_lin[ig] * alpha;
                            sb = srgb_to_lin[ib] * alpha;
                        }
                        col.r = col.r * inv_a + sr;
                        col.g = col.g * inv_a + sg;
                        col.b = col.b * inv_a + sb;
                    }

                    float bxf = (x + 0.5f) * 0.25f - 0.5f;
                    int bx0 = std::clamp(static_cast<int>(std::floor(bxf)), 0, BW - 1);
                    int bx1 = std::min(bx0 + 1, BW - 1);
                    float gfx = std::clamp(bxf - bx0, 0.0f, 1.0f);

                    Vec3 bl = mix3(
                        mix3(bloom_a[by0 * BW + bx0], bloom_a[by0 * BW + bx1], gfx),
                        mix3(bloom_a[by1 * BW + bx0], bloom_a[by1 * BW + bx1], gfx),
                        gfy);
                    col += bl * 0.58f;

                    if (flare_amt > 0.01f) {
                        Vec3 st = mix3(
                            mix3(streak_a[by0 * BW + bx0], streak_a[by0 * BW + bx1], gfx),
                            mix3(streak_a[by1 * BW + bx0], streak_a[by1 * BW + bx1], gfx),
                            gfy);
                        col += st * 0.65f;
                    }

                    float nx = (x - W * 0.5f) * inv_w * 2.0f;
                    float r2 = nx * nx * 0.28f + ny * ny * 0.36f;
                    float vig = std::max(0.45f, 1.0f - r2 * 0.42f);
                    col *= (vig * exposure * picture_fade);

                    col.r = aces_film(col.r);
                    col.g = aces_film(col.g);
                    col.b = aces_film(col.b);
                } else {
                    col = {0.0018f, 0.0022f, 0.0036f};
                }

                if (y == BAR_H || y == H - BAR_H - 1) {
                    col = mix3(col, Vec3{0.08f, 0.09f, 0.11f}, 0.42f);
                }

                const uint8_t* op = ov_row + x * 4;
                uint8_t oa = op[3];
                if (oa > 0) {
                    float alpha = oa * (1.0f / 255.0f);
                    float inv_a = 1.0f - alpha;
                    float orr = srgb_to_lin[op[2]];
                    float og = srgb_to_lin[op[1]];
                    float ob = srgb_to_lin[op[0]];
                    if (oa < 255) {
                        float inv_alpha = 255.0f / static_cast<float>(oa);
                        int ir = std::min(255, static_cast<int>(op[2] * inv_alpha));
                        int ig = std::min(255, static_cast<int>(op[1] * inv_alpha));
                        int ib = std::min(255, static_cast<int>(op[0] * inv_alpha));
                        orr = srgb_to_lin[ir] * alpha;
                        og = srgb_to_lin[ig] * alpha;
                        ob = srgb_to_lin[ib] * alpha;
                    }
                    col.r = col.r * inv_a + orr;
                    col.g = col.g * inv_a + og;
                    col.b = col.b * inv_a + ob;
                }

                int ir = std::clamp(static_cast<int>(col.r * 4095.0f), 0, 4095);
                int ig = std::clamp(static_cast<int>(col.g * 4095.0f), 0, 4095);
                int ib = std::clamp(static_cast<int>(col.b * 4095.0f), 0, 4095);
                int r8 = lin_to_srgb[ir];
                int g8 = lin_to_srgb[ig];
                int b8 = lin_to_srgb[ib];

                // Apply delicate 35mm organic film grain in sRGB space (shadow-safe, zero snowy blacks!)
                if (add_grain && grain_amt > 0.0f) {
                    uint32_t h = hash_u32(frame_seed ^ (static_cast<uint32_t>(y) * 73856093U) ^ (static_cast<uint32_t>(x) * 19349663U));
                    float lum8 = (0.2126f * r8 + 0.7152f * g8 + 0.0722f * b8) * (1.0f / 255.0f);
                    float g_amp = grain_amt * (0.25f + 0.75f * lum8) * 255.0f;
                    int gn = static_cast<int>((static_cast<float>(h & 0xFFFFU) * (1.0f / 32768.0f) - 1.0f) * g_amp);
                    r8 = std::clamp(r8 + gn, 0, 255);
                    g8 = std::clamp(g8 + gn, 0, 255);
                    b8 = std::clamp(b8 + gn, 0, 255);
                }

                dst[x * 3 + 0] = static_cast<uint8_t>(r8);
                dst[x * 3 + 1] = static_cast<uint8_t>(g8);
                dst[x * 3 + 2] = static_cast<uint8_t>(b8);
            }
        }
    }
};

}  // namespace mv
