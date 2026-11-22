#pragma once

#include "storyboard.hpp"
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
constexpr int BAR_H = 0;  // 100% full-bleed 16:9 1920x1080 (zero black bars!)
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
    std::vector<Vec3> atmo;       // current shot AW x AH
    std::vector<Vec3> atmo_prev;  // outgoing shot AW x AH (during transitions)
    std::vector<Vec3> bloom_a;    // BW x BH
    std::vector<Vec3> bloom_b;    // BW x BH
    std::vector<Vec3> streak_a;   // BW x BH
    std::vector<uint8_t> rgb_out;

    cairo_surface_t* scene_surf = nullptr;
    cairo_t* scene_cr = nullptr;
    cairo_surface_t* glow_surf = nullptr;
    cairo_t* glow_cr = nullptr;

    cairo_surface_t* scene_prev_surf = nullptr;
    cairo_t* scene_prev_cr = nullptr;
    cairo_surface_t* glow_prev_surf = nullptr;
    cairo_t* glow_prev_cr = nullptr;

    cairo_surface_t* overlay_surf = nullptr;
    cairo_t* overlay_cr = nullptr;

    float srgb_to_lin[256]{};
    uint8_t lin_to_srgb[4096]{};
    std::vector<float> kern6;
    std::vector<float> kern11;
    std::vector<float> kern_streak;

    HdrCanvas()
        : atmo(AW * AH),
          atmo_prev(AW * AH),
          bloom_a(BW * BH),
          bloom_b(BW * BH),
          streak_a(BW * BH),
          rgb_out(W * H * 3) {
        scene_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
        scene_cr = cairo_create(scene_surf);
        glow_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, AW, AH);
        glow_cr = cairo_create(glow_surf);

        scene_prev_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
        scene_prev_cr = cairo_create(scene_prev_surf);
        glow_prev_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, AW, AH);
        glow_prev_cr = cairo_create(glow_prev_surf);

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

        float ssum = 0.0f;
        for (int k = -32; k <= 32; k += 2) {
            float w = std::exp(-std::abs(k) * 0.088f);
            kern_streak.push_back(w);
            ssum += w;
        }
        for (float& w : kern_streak) w /= ssum;
    }

    ~HdrCanvas() {
        auto kill = [](cairo_t*& cr, cairo_surface_t*& sf) {
            if (cr) { cairo_destroy(cr); cr = nullptr; }
            if (sf) { cairo_surface_destroy(sf); sf = nullptr; }
        };
        kill(scene_cr, scene_surf);
        kill(glow_cr, glow_surf);
        kill(scene_prev_cr, scene_prev_surf);
        kill(glow_prev_cr, glow_prev_surf);
        kill(overlay_cr, overlay_surf);
    }

    static void clear_cairo(cairo_t* cr) {
        cairo_save(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
        cairo_paint(cr);
        cairo_restore(cr);
    }

    void clear_layers(bool dual) {
        std::fill(atmo.begin(), atmo.end(), Vec3{});
        clear_cairo(scene_cr);
        clear_cairo(glow_cr);
        clear_cairo(overlay_cr);
        if (dual) {
            std::fill(atmo_prev.begin(), atmo_prev.end(), Vec3{});
            clear_cairo(scene_prev_cr);
            clear_cairo(glow_prev_cr);
        }
    }

    inline Vec3 sample_cairo_linear(const uint8_t* px, float alpha_scale = 1.0f) const {
        uint8_t a = px[3];
        if (a == 0) return {};
        if (a == 255) {
            return {srgb_to_lin[px[2]] * alpha_scale, srgb_to_lin[px[1]] * alpha_scale, srgb_to_lin[px[0]] * alpha_scale};
        }
        float al = a * (1.0f / 255.0f);
        float inv_al = 255.0f / static_cast<float>(a);
        int ir = std::min(255, static_cast<int>(px[2] * inv_al));
        int ig = std::min(255, static_cast<int>(px[1] * inv_al));
        int ib = std::min(255, static_cast<int>(px[0] * inv_al));
        return {srgb_to_lin[ir] * al * alpha_scale, srgb_to_lin[ig] * al * alpha_scale, srgb_to_lin[ib] * al * alpha_scale};
    }

    inline void over_cairo(Vec3& dst, const uint8_t* px) const {
        uint8_t sa = px[3];
        if (sa == 0) return;
        float inv_a = 1.0f - sa * (1.0f / 255.0f);
        Vec3 s = sample_cairo_linear(px, 1.0f);
        dst.r = dst.r * inv_a + s.r;
        dst.g = dst.g * inv_a + s.g;
        dst.b = dst.b * inv_a + s.b;
    }

    void compose(int frame_idx, float exposure, float grain_amt, float flare_amt,
                 float picture_fade, float trans_u, TransType trans_type) {
        const bool dual = (trans_u < 0.999f);
        cairo_surface_flush(scene_surf);
        cairo_surface_flush(glow_surf);
        cairo_surface_flush(overlay_surf);
        if (dual) {
            cairo_surface_flush(scene_prev_surf);
            cairo_surface_flush(glow_prev_surf);
        }

        const uint8_t* scene_px = cairo_image_surface_get_data(scene_surf);
        const int scene_stride = cairo_image_surface_get_stride(scene_surf);
        const uint8_t* glow_px = cairo_image_surface_get_data(glow_surf);
        const int glow_stride = cairo_image_surface_get_stride(glow_surf);

        const uint8_t* scene_prev_px = dual ? cairo_image_surface_get_data(scene_prev_surf) : nullptr;
        const int scene_prev_stride = dual ? cairo_image_surface_get_stride(scene_prev_surf) : 0;
        const uint8_t* glow_prev_px = dual ? cairo_image_surface_get_data(glow_prev_surf) : nullptr;
        const int glow_prev_stride = dual ? cairo_image_surface_get_stride(glow_prev_surf) : 0;

        const uint8_t* ov_px = cairo_image_surface_get_data(overlay_surf);
        const int ov_stride = cairo_image_surface_get_stride(overlay_surf);

        auto pixel_blend_u = [&](float xn, float yn) -> float {
            if (!dual) return 1.0f;
            if (trans_type == TransType::MIST_WIPE) {
                float edge = trans_u * 1.36f - 0.18f;
                float u_pos = xn * 0.85f + yn * 0.15f;
                return 1.0f - smoothstep(edge - 0.18f, edge + 0.18f, u_pos);
            }
            return trans_u;
        };

        // 1. Build BW x BH optical bloom & flare buffer (linearly blended if transitioning)
        #pragma omp parallel for schedule(static)
        for (int y = 0; y < BH; ++y) {
            int ay0 = y * 2;
            const float yn = static_cast<float>(y) / static_cast<float>(BH);
            const uint8_t* g0 = glow_px + ay0 * glow_stride;
            const uint8_t* g1 = glow_px + (ay0 + 1) * glow_stride;
            const uint8_t* gp0 = dual ? (glow_prev_px + ay0 * glow_prev_stride) : nullptr;
            const uint8_t* gp1 = dual ? (glow_prev_px + (ay0 + 1) * glow_prev_stride) : nullptr;
            for (int x = 0; x < BW; ++x) {
                int ax0 = x * 2;
                float u = dual ? pixel_blend_u(static_cast<float>(x) / static_cast<float>(BW), yn) : 1.0f;
                Vec3 sum = atmo[ay0 * AW + ax0] + atmo[ay0 * AW + ax0 + 1] +
                           atmo[(ay0 + 1) * AW + ax0] + atmo[(ay0 + 1) * AW + ax0 + 1];
                sum *= 0.25f;

                Vec3 gsum{};
                const uint8_t* rows[2] = {g0, g1};
                for (int dy = 0; dy < 2; ++dy) {
                    for (int dx = 0; dx < 2; ++dx) {
                        const uint8_t* p = rows[dy] + (ax0 + dx) * 4;
                        if (p[3] > 0) {
                            gsum.b += srgb_to_lin[p[0]] * 2.0f;
                            gsum.g += srgb_to_lin[p[1]] * 2.0f;
                            gsum.r += srgb_to_lin[p[2]] * 2.0f;
                        }
                    }
                }
                gsum *= 0.25f;

                if (dual) {
                    Vec3 psum = atmo_prev[ay0 * AW + ax0] + atmo_prev[ay0 * AW + ax0 + 1] +
                                atmo_prev[(ay0 + 1) * AW + ax0] + atmo_prev[(ay0 + 1) * AW + ax0 + 1];
                    psum *= 0.25f;
                    sum = mix3(psum, sum, u);

                    Vec3 pgsum{};
                    const uint8_t* prows[2] = {gp0, gp1};
                    for (int dy = 0; dy < 2; ++dy) {
                        for (int dx = 0; dx < 2; ++dx) {
                            const uint8_t* p = prows[dy] + (ax0 + dx) * 4;
                            if (p[3] > 0) {
                                pgsum.b += srgb_to_lin[p[0]] * 2.0f;
                                pgsum.g += srgb_to_lin[p[1]] * 2.0f;
                                pgsum.r += srgb_to_lin[p[2]] * 2.0f;
                            }
                        }
                    }
                    pgsum *= 0.25f;
                    gsum = mix3(pgsum, gsum, u);
                }

                float lum = 0.2126f * sum.r + 0.7152f * sum.g + 0.0722f * sum.b;
                Vec3 bright{};
                if (lum > 0.68f) {
                    float scale = (lum - 0.68f) / (lum + 1e-4f);
                    bright = sum * (scale * 0.85f);
                }
                bright += gsum;
                bloom_a[y * BW + x] = bright;
                streak_a[y * BW + x] = (lum > 1.20f) ? (bright * 0.55f) : (gsum * 0.18f);
            }
        }

        // 2. Separable Gaussian blur passes
        auto blur_pass = [&](int radius, const std::vector<float>& kern) {
            #pragma omp parallel for schedule(static)
            for (int y = 0; y < BH; ++y) {
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
            for (int y = 0; y < BH; ++y) {
                Vec3* dst_row = bloom_a.data() + y * BW;
                for (int x = 0; x < BW; ++x) {
                    Vec3 acc{};
                    for (int k = -radius; k <= radius; ++k) {
                        int sy = std::clamp(y + k, 0, BH - 1);
                        acc += bloom_b[sy * BW + x] * kern[k + radius];
                    }
                    dst_row[x] = acc;
                }
            }
        };
        blur_pass(6, kern6);
        blur_pass(11, kern11);

        // 3. Horizontal anamorphic lens streak
        if (flare_amt > 0.01f) {
            #pragma omp parallel for schedule(static)
            for (int y = 0; y < BH; ++y) {
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
            for (int y = 0; y < BH; ++y) {
                int y0 = std::max(0, y - 1);
                int y1 = std::min(BH - 1, y + 1);
                for (int x = 0; x < BW; ++x) {
                    streak_a[y * BW + x] = (bloom_b[y0 * BW + x] * 0.25f +
                                            bloom_b[y * BW + x] * 0.50f +
                                            bloom_b[y1 * BW + x] * 0.25f) * flare_amt;
                }
            }
        }

        // Transition optical light-leak or bloom-flash pulse
        float leak_pulse = 0.0f;
        Vec3 leak_col{};
        if (dual && trans_type == TransType::LIGHT_LEAK) {
            float s = std::sin(trans_u * static_cast<float>(M_PI));
            leak_pulse = s * s * 0.36f;
            leak_col = {1.15f, 0.52f, 0.18f};
        } else if (dual && trans_type == TransType::BLOOM_FLASH) {
            float s = std::sin(trans_u * static_cast<float>(M_PI));
            leak_pulse = s * s * s * 0.44f;
            leak_col = {0.95f, 0.88f, 0.72f};
        }

        // 4. Full-bleed 1920 x 1080 HDR compositing + ACES + sRGB + shadow-safe 35mm grain
        const float inv_w = 1.0f / static_cast<float>(W);
        const float inv_h = 1.0f / static_cast<float>(H);
        const uint32_t frame_seed = hash_u32(static_cast<uint32_t>(frame_idx) * 747796405U + 2891336453U);

        #pragma omp parallel for schedule(static)
        for (int y = 0; y < H; ++y) {
            const uint8_t* sc_row = scene_px + y * scene_stride;
            const uint8_t* scp_row = dual ? (scene_prev_px + y * scene_prev_stride) : nullptr;
            const uint8_t* ov_row = ov_px + y * ov_stride;
            uint8_t* dst = rgb_out.data() + y * W * 3;

            float yn = static_cast<float>(y) * inv_h;
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
                float xn = static_cast<float>(x) * inv_w;
                float u = dual ? pixel_blend_u(xn, yn) : 1.0f;

                float axf = (x + 0.5f) * 0.5f - 0.5f;
                int ax0 = std::clamp(static_cast<int>(std::floor(axf)), 0, AW - 1);
                int ax1 = std::min(ax0 + 1, AW - 1);
                float fx = std::clamp(axf - ax0, 0.0f, 1.0f);

                Vec3 col = mix3(mix3(atmo[ay0 * AW + ax0], atmo[ay0 * AW + ax1], fx),
                                mix3(atmo[ay1 * AW + ax0], atmo[ay1 * AW + ax1], fx), fy);

                const uint8_t* g_px = glow_px + ay0 * glow_stride + ax0 * 4;
                if (g_px[3] > 0) {
                    col.r += srgb_to_lin[g_px[2]] * 1.15f;
                    col.g += srgb_to_lin[g_px[1]] * 1.15f;
                    col.b += srgb_to_lin[g_px[0]] * 1.15f;
                }
                over_cairo(col, sc_row + x * 4);

                if (dual) {
                    Vec3 pcol = mix3(mix3(atmo_prev[ay0 * AW + ax0], atmo_prev[ay0 * AW + ax1], fx),
                                     mix3(atmo_prev[ay1 * AW + ax0], atmo_prev[ay1 * AW + ax1], fx), fy);
                    const uint8_t* gp_px = glow_prev_px + ay0 * glow_prev_stride + ax0 * 4;
                    if (gp_px[3] > 0) {
                        pcol.r += srgb_to_lin[gp_px[2]] * 1.15f;
                        pcol.g += srgb_to_lin[gp_px[1]] * 1.15f;
                        pcol.b += srgb_to_lin[gp_px[0]] * 1.15f;
                    }
                    over_cairo(pcol, scp_row + x * 4);
                    col = mix3(pcol, col, u);
                }

                float bxf = (x + 0.5f) * 0.25f - 0.5f;
                int bx0 = std::clamp(static_cast<int>(std::floor(bxf)), 0, BW - 1);
                int bx1 = std::min(bx0 + 1, BW - 1);
                float gfx = std::clamp(bxf - bx0, 0.0f, 1.0f);

                Vec3 bl = mix3(
                    mix3(bloom_a[by0 * BW + bx0], bloom_a[by0 * BW + bx1], gfx),
                    mix3(bloom_a[by1 * BW + bx0], bloom_a[by1 * BW + bx1], gfx),
                    gfy);
                col += bl * 0.56f;

                if (flare_amt > 0.01f) {
                    Vec3 st = mix3(
                        mix3(streak_a[by0 * BW + bx0], streak_a[by0 * BW + bx1], gfx),
                        mix3(streak_a[by1 * BW + bx0], streak_a[by1 * BW + bx1], gfx),
                        gfy);
                    col += st * 0.62f;
                }

                if (leak_pulse > 0.002f) {
                    float edge_bias = 0.45f + 0.55f * (1.0f - xn * 0.6f);
                    col += leak_col * (leak_pulse * edge_bias);
                }

                float nx = (x - W * 0.5f) * inv_w * 2.0f;
                float r2 = nx * nx * 0.20f + ny * ny * 0.24f;
                float vig = std::max(0.56f, 1.0f - r2 * 0.34f);
                col *= (vig * exposure * picture_fade);

                col.r = aces_film(col.r);
                col.g = aces_film(col.g);
                col.b = aces_film(col.b);

                // Composite crisp full-bleed Pango typography overlay
                const uint8_t* op = ov_row + x * 4;
                if (op[3] > 0) {
                    over_cairo(col, op);
                }

                int ir = std::clamp(static_cast<int>(col.r * 4095.0f), 0, 4095);
                int ig = std::clamp(static_cast<int>(col.g * 4095.0f), 0, 4095);
                int ib = std::clamp(static_cast<int>(col.b * 4095.0f), 0, 4095);
                int r8 = lin_to_srgb[ir];
                int g8 = lin_to_srgb[ig];
                int b8 = lin_to_srgb[ib];

                if (grain_amt > 0.0f) {
                    uint32_t h = hash_u32(frame_seed ^ (static_cast<uint32_t>(y) * 73856093U) ^ (static_cast<uint32_t>(x) * 19349663U));
                    float lum8 = (0.2126f * r8 + 0.7152f * g8 + 0.0722f * b8) * (1.0f / 255.0f);
                    float g_amp = grain_amt * (0.22f + 0.78f * lum8) * 255.0f;
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
