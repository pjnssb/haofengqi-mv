#pragma once

#include "audio_lyrics.hpp"
#include "hdr_canvas.hpp"
#include "storyboard.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace mv {

inline float fractf(float x) { return x - std::floor(x); }

inline float hash21(float x, float y) {
    uint32_t ix = static_cast<uint32_t>(static_cast<int32_t>(std::floor(x)) + 16384);
    uint32_t iy = static_cast<uint32_t>(static_cast<int32_t>(std::floor(y)) + 16384);
    uint32_t h = hash_u32(ix * 374761393U + iy * 668265263U);
    return static_cast<float>(h & 0xFFFFU) * (1.0f / 65536.0f);
}

inline float vnoise(float x, float y) {
    float ix = std::floor(x);
    float iy = std::floor(y);
    float fx = x - ix;
    float fy = y - iy;
    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uy = fy * fy * (3.0f - 2.0f * fy);
    float a = hash21(ix, iy);
    float b = hash21(ix + 1.0f, iy);
    float c = hash21(ix, iy + 1.0f);
    float d = hash21(ix + 1.0f, iy + 1.0f);
    return a + (b - a) * ux + (c - a) * uy + (a - b - c + d) * ux * uy;
}

inline float fbm3(float x, float y) {
    float v = 0.0f;
    float amp = 0.5f;
    for (int i = 0; i < 3; ++i) {
        v += amp * vnoise(x, y);
        float nx = x * 1.97f - y * 0.42f;
        float ny = x * 0.42f + y * 1.97f;
        x = nx + 13.7f;
        y = ny + 7.3f;
        amp *= 0.5f;
    }
    return v;
}

inline float fbm4(float x, float y) {
    float v = 0.0f;
    float amp = 0.5f;
    for (int i = 0; i < 4; ++i) {
        v += amp * vnoise(x, y);
        float nx = x * 1.92f - y * 0.52f;
        float ny = x * 0.52f + y * 1.92f;
        x = nx + 11.3f;
        y = ny + 5.9f;
        amp *= 0.5f;
    }
    return v;
}

inline Vec3 starfield(float u, float v, float t, float air_energy, float density = 1.0f) {
    Vec3 acc{};
    for (int layer = 0; layer < 2; ++layer) {
        float scale = (layer == 0) ? 70.0f : 135.0f;
        float gx = u * scale + layer * 17.3f;
        float gy = v * scale * 0.5625f + layer * 9.1f;
        float ix = std::floor(gx);
        float iy = std::floor(gy);
        float rnd = hash21(ix, iy);
        if (rnd > 1.0f - 0.034f * density) {
            float cx = ix + 0.18f + 0.64f * hash21(ix + 5.1f, iy);
            float cy = iy + 0.18f + 0.64f * hash21(ix, iy + 8.7f);
            float dx = gx - cx;
            float dy = gy - cy;
            float d2 = dx * dx + dy * dy;
            float twinkle = 0.65f + 0.35f * std::sin(t * (2.2f + rnd * 4.5f) + rnd * 60.0f);
            float core = std::exp(-d2 * (layer == 0 ? 52.0f : 70.0f));
            float tint = hash21(ix + 31.0f, iy + 19.0f);
            Vec3 sc = mix3(Vec3{0.62f, 0.80f, 1.15f}, Vec3{1.15f, 0.94f, 0.72f}, tint);
            float boost = (0.75f + air_energy * 0.85f) * twinkle * (layer == 0 ? 1.50f : 0.85f);
            acc += sc * (core * boost);
        }
    }
    return acc;
}

inline Vec3 eval_burning_sky(float sx, float sy, float t) {
    Vec3 top{0.055f, 0.022f, 0.068f};
    Vec3 mid{0.42f, 0.105f, 0.072f};
    Vec3 low{0.92f, 0.32f, 0.09f};
    Vec3 base = (sy < 0.55f) ? mix3(top, mid, sy / 0.55f) : mix3(mid, low, (sy - 0.55f) / 0.45f);
    float warp = fbm3(sx * 3.2f + t * 0.04f, sy * 2.6f);
    float cloud = fbm4(sx * 4.5f + warp * 0.85f, sy * 3.8f - t * 0.02f);
    float lit = smoothstep(0.42f, 0.80f, cloud) * (0.28f + 0.72f * sy);
    Vec3 ember = mix3(Vec3{1.15f, 0.22f, 0.07f}, Vec3{1.45f, 0.68f, 0.20f}, sy);
    Vec3 dark_cloud{0.042f, 0.020f, 0.052f};
    Vec3 c = mix3(base, dark_cloud, smoothstep(0.42f, 0.72f, 1.0f - cloud) * 0.68f);
    c += ember * lit * 0.82f;
    float d_sun = std::hypot((sx - 0.52f) * 1.77f, (sy - 0.64f) * 1.5f);
    c += Vec3{1.25f, 0.66f, 0.22f} * std::exp(-d_sun * 3.6f) * 0.55f;
    return c;
}

inline void render_atmosphere_shot(std::vector<Vec3>& dst, SceneKind kind, double time_sec,
                                   float prog, const AudioFrame& af) {
    const float t = static_cast<float>(time_sec);
    constexpr float ASPECT = 16.0f / 9.0f;

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < AH; ++y) {
        const float yn = static_cast<float>(y) / static_cast<float>(AH);
        const float aspect_y = yn - 0.5f;
        for (int x = 0; x < AW; ++x) {
            const float xn = static_cast<float>(x) / static_cast<float>(AW);
            const float aspect_x = (xn - 0.5f) * ASPECT;
            Vec3 col{};

            switch (kind) {
            case SceneKind::METRO_WINDOW: {
                // Shot 01: Full-bleed rain-beaded panoramic metro window & fast bokeh trails
                col = mix3(Vec3{0.012f, 0.024f, 0.046f}, Vec3{0.032f, 0.064f, 0.105f}, 1.0f - yn);
                float streak = vnoise(xn * 96.0f, yn * 10.0f - t * 0.35f);
                col += Vec3{0.022f, 0.042f, 0.064f} * std::pow(streak, 2.8f) * 0.45f;
                for (int i = 0; i < 16; ++i) {
                    float fi = static_cast<float>(i);
                    float spd = 0.16f + 0.22f * hash21(fi, 2.0f);
                    float bx = fractf(hash21(fi, 5.0f) - t * spd);
                    float by = 0.10f + 0.80f * hash21(fi, 9.0f);
                    float rad = 0.025f + 0.055f * hash21(fi, 13.0f);
                    float dist = std::hypot((xn - bx) * ASPECT, yn - by);
                    float orb = smoothstep(rad, rad * 0.25f, dist);
                    Vec3 bc = (i % 3 == 0) ? Vec3{1.10f, 0.55f, 0.18f} : Vec3{0.22f, 0.60f, 0.94f};
                    col += bc * (orb * 0.30f * (0.65f + af.mid * 0.65f));
                }
                break;
            }
            case SceneKind::RAIN_SKYSCRAPERS_UP: {
                // Shot 02: Looking UP from wet stairs into stormy neon city sky
                float cloud = fbm4(xn * 3.8f + t * 0.06f, yn * 2.8f - prog * 0.4f);
                col = mix3(Vec3{0.010f, 0.020f, 0.042f}, Vec3{0.055f, 0.088f, 0.145f}, yn);
                col += Vec3{0.045f, 0.068f, 0.115f} * cloud * 0.8f;
                // Rising city neon glow from street level below
                float neon_l = std::exp(-std::hypot((xn - 0.16f) * ASPECT, yn - 0.88f) * 2.4f);
                float neon_r = std::exp(-std::hypot((xn - 0.84f) * ASPECT, yn - 0.84f) * 2.2f);
                col += Vec3{0.92f, 0.22f, 0.42f} * neon_l * 0.32f;
                col += Vec3{0.20f, 0.62f, 0.95f} * neon_r * 0.36f;
                break;
            }
            case SceneKind::PUDDLE_NEON_CLOSEUP: {
                // Shot 03: Low puddle macro (horizon high at 0.22!) reflecting neon & concentric rain ripples
                const float hor = 0.22f;
                if (yn < hor) {
                    float sy = yn / hor;
                    col = mix3(Vec3{0.014f, 0.024f, 0.048f}, Vec3{0.042f, 0.064f, 0.105f}, sy);
                } else {
                    float ry = (yn - hor) / (1.0f - hor);
                    float tex = vnoise(xn * 200.0f, yn * 130.0f);
                    col = mix3(Vec3{0.020f, 0.030f, 0.052f}, Vec3{0.008f, 0.012f, 0.022f}, ry) * (0.75f + 0.45f * tex);
                    float ripple_dx = 0.0f, ripple_ring = 0.0f;
                    for (int r = 0; r < 7; ++r) {
                        float fr = static_cast<float>(r);
                        float ph = fractf(t * 0.92f + fr * 0.17f);
                        float rx = 0.14f + 0.72f * hash21(std::floor(t * 0.92f + fr * 0.17f), fr * 7.0f);
                        float ry_c = 0.36f + 0.56f * hash21(fr * 3.0f, std::floor(t * 0.92f + fr * 0.17f));
                        float d = std::hypot((xn - rx) * ASPECT, (yn - ry_c) * 1.9f);
                        float ring = std::sin((d - ph * 0.42f) * 72.0f) * std::exp(-d * 12.0f) * (1.0f - ph);
                        ripple_dx += ring * 0.016f;
                        ripple_ring += std::max(0.0f, ring) * 0.16f;
                    }
                    float u_r = xn + ripple_dx;
                    auto ref_col = [&](float lx, float w, Vec3 c, float str) {
                        float d = std::abs(u_r - lx) / w;
                        float chop = 0.55f + 0.45f * std::sin(ry * 70.0f + tex * 5.0f + t * 4.0f);
                        return c * (std::exp(-d * d * (1.0f + 1.2f * ry)) * chop * str);
                    };
                    col += ref_col(0.24f, 0.08f, Vec3{0.96f, 0.24f, 0.46f}, 0.58f);
                    col += ref_col(0.55f, 0.07f, Vec3{0.30f, 0.72f, 1.00f}, 0.48f);
                    col += ref_col(0.80f, 0.05f, Vec3{1.15f, 0.22f, 0.22f}, 0.55f);
                    col += Vec3{0.32f, 0.46f, 0.65f} * ripple_ring;
                }
                break;
            }
            case SceneKind::HANDS_TICKET_MACRO: {
                // Shot 04: Shallow DOF rainy street bokeh behind cupped hands holding creased ticket
                col = mix3(Vec3{0.010f, 0.018f, 0.036f}, Vec3{0.025f, 0.042f, 0.072f}, yn);
                for (int i = 0; i < 18; ++i) {
                    float fi = static_cast<float>(i);
                    float bx = hash21(fi, 4.1f) + 0.03f * std::sin(t * 0.4f + fi);
                    float by = hash21(fi, 9.7f) + 0.02f * std::cos(t * 0.5f + fi);
                    float rad = 0.035f + 0.065f * hash21(fi, 1.9f);
                    float d = std::hypot((xn - bx) * ASPECT, yn - by);
                    float ring = smoothstep(rad, rad * 0.7f, d) * (0.4f + 0.6f * smoothstep(rad * 0.2f, rad * 0.85f, d));
                    Vec3 c = (i % 3 == 0) ? Vec3{1.10f, 0.62f, 0.22f}
                           : ((i % 3 == 1) ? Vec3{0.26f, 0.68f, 0.96f} : Vec3{0.92f, 0.26f, 0.45f});
                    col += c * (ring * 0.22f * (0.7f + 0.4f * af.mid));
                }
                break;
            }
            case SceneKind::CROSSWALK_WAIT_CARS:
            case SceneKind::CROSSWALK_GREEN_STRIDE: {
                bool is_green = (kind == SceneKind::CROSSWALK_GREEN_STRIDE);
                float hor = is_green ? 0.44f : 0.58f;
                float g_fade = is_green ? 1.0f : smoothstep(35.8f, 36.7f, t);
                Vec3 sig_col = mix3(Vec3{1.40f, 0.16f, 0.18f}, Vec3{0.18f, 1.45f, 0.72f}, g_fade);
                if (yn < hor) {
                    float sy = yn / hor;
                    float cloud = fbm3(xn * 3.6f + t * 0.05f, sy * 2.2f);
                    col = mix3(Vec3{0.012f, 0.022f, 0.044f}, Vec3{0.042f, 0.072f, 0.112f}, sy);
                    col += Vec3{0.028f, 0.048f, 0.075f} * cloud * 0.6f;
                } else {
                    float ry = (yn - hor) / (1.0f - hor);
                    float tex = vnoise(xn * 220.0f, yn * 140.0f);
                    col = mix3(Vec3{0.018f, 0.028f, 0.044f}, Vec3{0.008f, 0.012f, 0.022f}, ry) * (0.75f + 0.45f * tex);
                    float chop = 0.55f + 0.45f * std::sin(ry * 75.0f + tex * 5.0f + t * 4.5f);
                    float sig_x = is_green ? 0.80f : 0.76f;
                    col += sig_col * std::exp(-std::pow((xn - sig_x) / 0.055f, 2.0f)) * chop * (0.55f + 0.25f * g_fade);
                    col += Vec3{0.92f, 0.22f, 0.42f} * std::exp(-std::pow((xn - 0.14f) / 0.06f, 2.0f)) * chop * 0.42f;
                }
                col += sig_col * std::exp(-std::hypot((xn - (is_green ? 0.80f : 0.76f)) * ASPECT, yn - 0.34f) * 3.6f) * (0.28f + 0.18f * g_fade);
                break;
            }
            case SceneKind::ROOM_DESK_LAMP:
            case SceneKind::ROOM_PILLOW_WINDOW: {
                // Full-bleed cozy apartment room: warm tungsten right/center + cool rain night window
                bool pillow_view = (kind == SceneKind::ROOM_PILLOW_WINDOW);
                float lamp_x = pillow_view ? 0.82f : 0.68f;
                float lamp_y = pillow_view ? 0.50f : 0.44f;
                // Night sky through wide panoramic window
                Vec3 night_sky = mix3(Vec3{0.014f, 0.026f, 0.055f}, Vec3{0.038f, 0.068f, 0.118f}, yn);
                night_sky += starfield(xn, yn, t, af.air * 0.5f, 0.6f) * 0.45f;
                // Distant city bokeh through window glass
                for (int i = 0; i < 10; ++i) {
                    float bx = 0.08f + 0.84f * hash21(i, 3.3f);
                    float by = 0.28f + 0.45f * hash21(i, 8.1f);
                    float d = std::hypot((xn - bx) * ASPECT, yn - by);
                    night_sky += Vec3{0.95f, 0.68f, 0.32f} * smoothstep(0.032f, 0.006f, d) * 0.35f;
                }
                // Warm interior amber tungsten ambient spill
                float d_lamp = std::hypot((xn - lamp_x) * ASPECT, yn - lamp_y);
                float warm_spill = std::exp(-d_lamp * 1.45f) * (0.85f + 0.20f * af.mid);
                col = night_sky + Vec3{1.15f, 0.62f, 0.22f} * warm_spill * (pillow_view ? 0.58f : 0.85f);
                if (pillow_view) {
                    // Cool silver-blue moonlight beam falling gently toward pillow on left
                    float moon_beam = std::exp(-std::pow((xn - 0.30f) * 1.8f + (yn - 0.55f) * 0.9f, 2.0f));
                    col += Vec3{0.18f, 0.30f, 0.52f} * moon_beam * 0.36f;
                }
                break;
            }
            case SceneKind::VIADUCT_EXPRESS_TRAIN:
            case SceneKind::PLATFORM_WIND_WAIT: {
                // Dawn steel-blue morning mist + peach sunrise glow + soft slanted shafts
                Vec3 zenith{0.025f, 0.058f, 0.118f};
                Vec3 mid_sky{0.105f, 0.195f, 0.305f};
                col = mix3(zenith, mid_sky, smoothstep(0.0f, 0.62f, yn));
                float east_x = (kind == SceneKind::VIADUCT_EXPRESS_TRAIN) ? 0.80f : 0.72f;
                float east = std::exp(-std::pow((xn - east_x) * 1.35f, 2.0f)) * smoothstep(0.24f, 0.70f, yn) * smoothstep(0.98f, 0.55f, yn);
                col += Vec3{0.84f, 0.48f, 0.28f} * east * (0.72f + 0.24f * af.all);

                float diag = (xn - east_x) * 1.55f + (yn - 0.16f) * 0.95f;
                float shafts = std::exp(-std::pow((diag + 0.22f) * 4.4f, 2.0f)) * 0.55f +
                               std::exp(-std::pow((diag - 0.10f) * 5.0f, 2.0f)) * 0.85f +
                               std::exp(-std::pow((diag - 0.42f) * 3.8f, 2.0f)) * 0.45f;
                float fog = 0.65f + 0.35f * vnoise(xn * 5.0f + t * 0.05f, yn * 4.0f);
                col += Vec3{0.46f, 0.38f, 0.30f} * shafts * fog * smoothstep(0.92f, 0.12f, yn) * 0.24f;
                break;
            }
            case SceneKind::MOUNTAIN_WIND_SWEEP: {
                // High aerial sweep over misty ridges with golden tailwind clouds
                float cloud = fbm4(xn * 3.8f - t * 0.16f, yn * 3.2f);
                col = mix3(Vec3{0.022f, 0.052f, 0.125f}, Vec3{0.18f, 0.32f, 0.52f}, yn);
                float hor_sun = std::exp(-std::pow((yn - 0.62f) * 4.2f, 2.0f));
                col += Vec3{0.92f, 0.56f, 0.28f} * hor_sun * (0.55f + 0.40f * af.bass);
                col += Vec3{0.45f, 0.58f, 0.78f} * smoothstep(0.45f, 0.82f, cloud) * 0.32f;
                break;
            }
            case SceneKind::SKYLINE_BLUEPRINT_RISE:
            case SceneKind::ROOFTOP_CATCH_STAR: {
                bool roof = (kind == SceneKind::ROOFTOP_CATCH_STAR);
                Vec3 zenith{0.010f, 0.020f, 0.062f};
                Vec3 mid{0.055f, 0.098f, 0.215f};
                col = mix3(zenith, mid, smoothstep(0.0f, 0.72f, yn));
                float hor_y = roof ? 0.88f : 0.72f;
                col += Vec3{0.52f, 0.28f, 0.22f} * std::exp(-std::pow((yn - hor_y) * 4.5f, 2.0f)) * (0.65f + 0.35f * af.bass);
                float mw_d = std::abs((yn - 0.34f) - (xn - 0.5f) * 0.42f);
                float mw = std::exp(-mw_d * 4.2f) * fbm3(xn * 5.0f + t * 0.02f, yn * 4.0f);
                col += Vec3{0.14f, 0.26f, 0.56f} * mw * (roof ? 0.95f : 0.55f);
                col += starfield(xn, yn, t, af.air, roof ? 1.5f : 1.2f) * (1.0f - smoothstep(hor_y - 0.22f, hor_y, yn));
                break;
            }
            case SceneKind::INTERLUDE_APPROACH:
            case SceneKind::INTERLUDE_HAND_LIGHT: {
                const float hor = (kind == SceneKind::INTERLUDE_HAND_LIGHT) ? 0.52f : 0.58f;
                float sy = (yn < hor) ? (yn / hor) : ((yn - hor) / (1.0f - hor));
                float sky_y = (yn < hor) ? sy : (1.0f - sy * 0.92f);
                Vec3 sc = mix3(Vec3{0.016f, 0.030f, 0.070f}, Vec3{0.088f, 0.145f, 0.245f}, sky_y);
                sc += Vec3{0.44f, 0.28f, 0.18f} * std::exp(-std::pow((sky_y - 0.95f) * 6.0f, 2.0f)) * 0.65f;
                if (sky_y < 0.74f) sc += starfield(xn, sky_y * hor, t, af.air * 0.35f, 0.75f) * 0.50f;
                if (yn >= hor) {
                    float d = std::hypot((xn - 0.50f) * ASPECT, (yn - (hor + 0.14f)) * 2.2f);
                    float ripple = std::sin(d * 54.0f - t * 2.5f) * std::exp(-d * 3.6f);
                    sc = sc * 0.68f + Vec3{0.95f, 0.70f, 0.36f} * std::max(0.0f, ripple) * 0.20f;
                }
                col = sc;
                break;
            }
            case SceneKind::HARBOR_FIREFLIES_WALK:
            case SceneKind::PAPER_BIRD_BAY_GLIDE:
            case SceneKind::BOARDWALK_LANTERNS:
            case SceneKind::GLASSES_TOAST_CLOSE:
            case SceneKind::BAY_BRIDGE_PANORAMA:
            case SceneKind::OBSERVATION_BLUEPRINT: {
                float hor = (kind == SceneKind::PAPER_BIRD_BAY_GLIDE) ? 0.45f
                          : ((kind == SceneKind::GLASSES_TOAST_CLOSE) ? 0.62f : 0.56f);
                if (yn < hor) {
                    float sy = yn / hor;
                    col = mix3(Vec3{0.012f, 0.022f, 0.048f}, Vec3{0.078f, 0.068f, 0.112f}, sy);
                    col += starfield(xn, yn, t, af.air, 0.85f) * (1.0f - sy * 0.75f) * 0.70f;
                    if (kind == SceneKind::BOARDWALK_LANTERNS || kind == SceneKind::GLASSES_TOAST_CLOSE) {
                        float warm = std::exp(-std::hypot((xn - 0.5f) * 1.2f, yn - 0.45f) * 1.8f);
                        col += Vec3{0.38f, 0.19f, 0.08f} * warm * (0.55f + 0.35f * af.mid);
                    }
                } else {
                    float ry = (yn - hor) / (1.0f - hor);
                    float pz = 1.0f / (ry + 0.08f);
                    float wave = vnoise(xn * 22.0f + t * 0.35f, pz * 2.8f - t * 0.75f);
                    col = mix3(Vec3{0.026f, 0.038f, 0.068f}, Vec3{0.010f, 0.016f, 0.032f}, ry);
                    for (int k = 0; k < 7; ++k) {
                        float lx = 0.14f + k * 0.12f;
                        float d = std::abs(xn - lx) / (0.020f + 0.018f * ry);
                        float glint = std::exp(-d * d) * smoothstep(0.42f, 0.78f, wave);
                        col += Vec3{1.10f, 0.68f, 0.25f} * glint * (0.40f + 0.25f * af.mid);
                    }
                }
                break;
            }
            case SceneKind::BRIDGE_DECK_SPEED:
            case SceneKind::SEA_RAIL_DAWN_STATION:
            case SceneKind::CAPE_LIGHTHOUSE_GALE:
            case SceneKind::SAILBOAT_GALE_ACTION: {
                float hor = (kind == SceneKind::SAILBOAT_GALE_ACTION) ? 0.48f : 0.56f;
                if (yn < hor) {
                    float sy = yn / hor;
                    float wind_cloud = fbm4(xn * 3.8f - t * 0.30f, sy * 4.2f);
                    Vec3 top = (kind == SceneKind::SEA_RAIL_DAWN_STATION) ? Vec3{0.020f, 0.048f, 0.110f} : Vec3{0.016f, 0.046f, 0.125f};
                    Vec3 bot = (kind == SceneKind::SEA_RAIL_DAWN_STATION) ? Vec3{0.28f, 0.36f, 0.48f} : Vec3{0.20f, 0.42f, 0.66f};
                    col = mix3(top, bot, sy);
                    col += Vec3{0.35f, 0.46f, 0.64f} * smoothstep(0.45f, 0.85f, wind_cloud) * 0.44f;
                    float sx = (kind == SceneKind::CAPE_LIGHTHOUSE_GALE) ? 0.26f : 0.72f;
                    float d_sun = std::hypot((xn - sx) * ASPECT, (sy - 0.94f) * 1.2f);
                    col += Vec3{1.32f, 0.88f, 0.46f} * std::exp(-d_sun * 3.6f) * 0.68f;
                    if (kind == SceneKind::CAPE_LIGHTHOUSE_GALE) {
                        // Sweeping volumetric lighthouse searchlight beam
                        float beam_ang = -0.18f + 0.14f * std::sin(t * 1.1f);
                        float dx = xn - 0.22f;
                        float dy = yn - 0.36f;
                        if (dx > 0.0f) {
                            float perp = std::abs(dy - dx * beam_ang) / (0.015f + dx * 0.18f);
                            col += Vec3{1.35f, 1.05f, 0.65f} * std::exp(-perp * perp) * std::exp(-dx * 1.4f) * 0.55f;
                        }
                    }
                } else {
                    float ry = (yn - hor) / (1.0f - hor);
                    float pz = 1.0f / (ry + 0.07f);
                    float w1 = std::sin(xn * 18.0f + pz * 2.2f - t * 3.6f);
                    float w2 = std::sin(xn * 34.0f - pz * 3.5f - t * 2.6f);
                    float slope = w1 * 0.6f + w2 * 0.4f;
                    col = mix3(Vec3{0.012f, 0.048f, 0.118f}, Vec3{0.058f, 0.205f, 0.380f}, 0.5f + 0.5f * slope);
                    float gx = (kind == SceneKind::CAPE_LIGHTHOUSE_GALE) ? 0.26f : 0.72f;
                    float path_w = 0.048f + 0.15f * ry;
                    float path_d = std::abs(xn - gx) / path_w;
                    float glitter = std::exp(-path_d * path_d) * std::pow(std::max(0.0f, slope * 0.5f + 0.5f), 4.0f);
                    col += Vec3{1.55f, 1.10f, 0.58f} * glitter * (0.72f + 0.60f * af.bass);
                    col = mix3(Vec3{0.24f, 0.38f, 0.56f}, col, smoothstep(0.0f, 0.12f, ry));
                }
                break;
            }
            case SceneKind::SUNSET_EXIT_PANORAMA:
            case SceneKind::SUNSET_CROSSWALK_PACK: {
                float hor = (kind == SceneKind::SUNSET_EXIT_PANORAMA) ? 0.74f : 0.62f;
                if (yn < hor) {
                    col = eval_burning_sky(xn, yn / hor, t);
                } else {
                    float py = (yn - hor) / (1.0f - hor);
                    Vec3 refl = eval_burning_sky(xn, 1.0f - py * 0.65f, t);
                    float tex = vnoise(xn * 120.0f, yn * 140.0f);
                    col = refl * (0.16f + 0.16f * tex) * (1.0f - 0.42f * py);
                    if (kind == SceneKind::SUNSET_CROSSWALK_PACK) {
                        float chop = 0.55f + 0.45f * std::sin(py * 68.0f + tex * 5.0f + t * 4.0f);
                        col += Vec3{0.18f, 1.45f, 0.72f} * std::exp(-std::pow((xn - 0.80f) / 0.055f, 2.0f)) * chop * 0.62f;
                    }
                }
                break;
            }
            case SceneKind::MACRO_EYE_REFLECTION: {
                // Full-bleed painterly Macro Eye (16:9 aspect)
                float ex = aspect_x * 1.18f + 0.012f * std::sin(t * 0.9f);
                float ey = aspect_y * 1.72f + 0.006f * std::cos(t * 1.3f);
                float lid_x = ex / 0.80f;
                float upper_lid = 0.39f * (1.0f - lid_x * lid_x * 0.84f);
                float lower_lid = -0.34f * (1.0f - lid_x * lid_x * 0.88f);
                float inside_eye = smoothstep(upper_lid + 0.022f, upper_lid - 0.018f, ey) *
                                   smoothstep(lower_lid - 0.022f, lower_lid + 0.018f, ey) *
                                   smoothstep(0.94f, 0.72f, std::abs(lid_x));

                float r = std::hypot(ex, ey);
                float ang = std::atan2(ey, ex);
                float lid_ao = smoothstep(upper_lid, upper_lid - 0.18f, ey);
                Vec3 sclera = Vec3{0.28f, 0.25f, 0.27f} * (0.38f + 0.62f * lid_ao);
                sclera = mix3(Vec3{0.16f, 0.075f, 0.085f}, sclera, smoothstep(0.88f, 0.50f, std::abs(lid_x)));

                float iris_r = 0.34f;
                float pupil_r = 0.132f - 0.010f * af.bass;
                float iris_mask = smoothstep(iris_r + 0.010f, iris_r - 0.010f, r);
                float pupil_mask = smoothstep(pupil_r + 0.008f, pupil_r - 0.006f, r);

                float fiber1 = vnoise(std::cos(ang) * 16.0f + r * 30.0f, std::sin(ang) * 16.0f + r * 30.0f);
                float fiber2 = vnoise(ang * 32.0f, r * 48.0f);
                float fiber = 0.55f * fiber1 + 0.45f * fiber2;
                float iris_norm = clamp01((r - pupil_r) / (iris_r - pupil_r));
                Vec3 iris_col = mix3(Vec3{0.44f, 0.21f, 0.06f}, Vec3{0.045f, 0.095f, 0.22f}, smoothstep(0.12f, 0.78f, iris_norm));
                iris_col *= (0.45f + 1.05f * fiber);
                iris_col = mix3(iris_col, Vec3{0.012f, 0.022f, 0.055f}, smoothstep(0.75f, 0.98f, iris_norm) * 0.88f);

                Vec3 eye_c = mix3(sclera, iris_col, iris_mask);
                eye_c = mix3(eye_c, Vec3{0.006f, 0.006f, 0.012f}, pupil_mask);
                if (iris_mask > 0.01f) {
                    Vec3 sky_refl = eval_burning_sky(clamp01(0.5f + ex * 1.55f), clamp01(0.54f + ey * 1.55f), t);
                    float peak = 0.58f + 0.45f * smoothstep(198.5f, 200.4f, t) * (1.0f + 0.35f * af.onset);
                    float fresnel = 0.16f + 0.28f * std::pow(iris_norm, 2.0f);
                    eye_c += sky_refl * (fresnel * 0.24f * peak) * iris_mask;
                    float cl = std::exp(-std::hypot((ex + 0.11f) * 16.0f, (ey + 0.09f) * 18.0f));
                    eye_c += Vec3{1.85f, 1.35f, 0.85f} * cl * (0.90f * peak) * iris_mask;
                }
                Vec3 skin_bg = Vec3{0.026f, 0.014f, 0.018f} * (1.15f - 0.35f * std::hypot(ex * 0.7f, ey));
                float tear_line = std::exp(-std::abs(ey - lower_lid) * 65.0f) * smoothstep(0.78f, 0.15f, std::abs(lid_x));
                col = mix3(skin_bg, eye_c, inside_eye) + Vec3{0.55f, 0.34f, 0.22f} * tear_line * 0.32f;
                break;
            }
            case SceneKind::MOUNTAIN_BEACONS:
            case SceneKind::PASS_REVEALS_SEA:
            case SceneKind::COSMIC_SKYLINE_SOAR:
            case SceneKind::MEMORY_TIME_MONTAGE: {
                bool cosmic = (kind == SceneKind::COSMIC_SKYLINE_SOAR || kind == SceneKind::MEMORY_TIME_MONTAGE);
                const float hor = cosmic ? 0.62f : 0.58f;
                if (yn < hor) {
                    float sy = yn / hor;
                    col = mix3(Vec3{0.008f, 0.016f, 0.052f}, cosmic ? Vec3{0.09f, 0.20f, 0.42f} : Vec3{0.16f, 0.36f, 0.58f}, sy);
                    float mw_d = std::abs((yn - 0.34f) + (xn - 0.5f) * 0.50f);
                    float mw = std::exp(-mw_d * 4.4f) * fbm4(xn * 4.4f - t * 0.02f, yn * 3.8f);
                    col += Vec3{0.18f, 0.38f, 0.78f} * mw * (cosmic ? 1.15f : 0.45f);
                    col += starfield(xn, yn, t, af.air, cosmic ? 1.7f : 0.95f) * (1.0f - sy * 0.55f);
                } else {
                    float ry = (yn - hor) / (1.0f - hor);
                    float pz = 1.0f / (ry + 0.08f);
                    float ripple = vnoise(xn * 22.0f + t * 0.25f, pz * 2.5f - t * 0.55f);
                    col = mix3(Vec3{0.055f, 0.185f, 0.380f}, Vec3{0.016f, 0.062f, 0.155f}, ry);
                    col += Vec3{0.10f, 0.26f, 0.48f} * smoothstep(0.45f, 0.85f, ripple) * (0.45f + 0.45f * af.bass);
                }
                break;
            }
            case SceneKind::SILENT_COBALT_SAIL:
            case SceneKind::DAWN_SEA_HORIZON:
            case SceneKind::EPILOGUE_STARS_SEA: {
                bool epi = (kind == SceneKind::EPILOGUE_STARS_SEA);
                bool dawn = (kind == SceneKind::DAWN_SEA_HORIZON);
                const float hor = 0.54f;
                if (yn < hor) {
                    float sy = yn / hor;
                    Vec3 top = Vec3{0.008f, 0.018f, 0.048f};
                    Vec3 bot = epi ? Vec3{0.065f, 0.145f, 0.285f} : (dawn ? Vec3{0.11f, 0.25f, 0.45f} : Vec3{0.088f, 0.235f, 0.465f});
                    col = mix3(top, bot, std::pow(sy, 1.25f));
                    if (dawn || epi) {
                        float g = std::exp(-std::pow((sy - 0.98f) * 8.0f, 2.0f)) * std::exp(-std::pow((xn - 0.5f) * 1.8f, 2.0f));
                        col += Vec3{0.62f, 0.44f, 0.25f} * g * (dawn ? 0.52f : 0.36f);
                    }
                    col += starfield(xn, yn, t * 0.5f, 0.15f, epi ? 0.55f : 0.75f) * (1.0f - sy * 0.85f);
                } else {
                    float ry = (yn - hor) / (1.0f - hor);
                    float pz = 1.0f / (ry + 0.08f);
                    float w1 = vnoise(xn * 18.0f + t * 0.18f, pz * 2.4f - t * 0.45f);
                    float w2 = std::sin(pz * 3.8f - t * 1.1f + xn * 9.0f) * 0.5f + 0.5f;
                    float calm_w = smoothstep(0.35f, 0.85f, 0.65f * w1 + 0.35f * w2);
                    col = mix3(epi ? Vec3{0.038f, 0.115f, 0.230f} : Vec3{0.058f, 0.185f, 0.380f},
                               Vec3{0.010f, 0.035f, 0.095f}, ry);
                    float path = std::exp(-std::pow((xn - 0.50f) * 4.0f, 2.0f));
                    col += Vec3{0.28f, 0.55f, 0.85f} * (path * calm_w * 0.22f * (1.0f - 0.35f * ry));
                }
                break;
            }
            }  // switch

            dst[y * AW + x] = col;
        }
    }
}

}  // namespace mv
