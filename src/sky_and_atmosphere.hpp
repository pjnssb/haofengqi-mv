#pragma once

#include "audio_lyrics.hpp"
#include "hdr_canvas.hpp"
#include <algorithm>
#include <cmath>

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

// Starfield helper in HDR linear light
inline Vec3 starfield(float u, float v, float t, float air_energy, float density = 1.0f) {
    Vec3 acc{};
    for (int layer = 0; layer < 2; ++layer) {
        float scale = (layer == 0) ? 72.0f : 140.0f;
        float gx = u * scale + layer * 17.3f;
        float gy = v * scale + layer * 9.1f;
        float ix = std::floor(gx);
        float iy = std::floor(gy);
        float rnd = hash21(ix, iy);
        if (rnd > 1.0f - 0.032f * density) {
            float cx = ix + 0.18f + 0.64f * hash21(ix + 5.1f, iy);
            float cy = iy + 0.18f + 0.64f * hash21(ix, iy + 8.7f);
            float dx = gx - cx;
            float dy = gy - cy;
            float d2 = dx * dx + dy * dy;
            float twinkle = 0.65f + 0.35f * std::sin(t * (2.2f + rnd * 4.5f) + rnd * 60.0f);
            float core = std::exp(-d2 * (layer == 0 ? 52.0f : 70.0f));
            float tint = hash21(ix + 31.0f, iy + 19.0f);
            Vec3 sc = mix3(Vec3{0.62f, 0.80f, 1.15f}, Vec3{1.15f, 0.94f, 0.72f}, tint);
            float boost = (0.75f + air_energy * 0.85f) * twinkle * (layer == 0 ? 1.55f : 0.85f);
            acc += sc * (core * boost);
        }
    }
    return acc;
}

inline void render_atmosphere(HdrCanvas& canvas, double time_sec, const AudioFrame& af, int act_id) {
    const float t = static_cast<float>(time_sec);
    const int y_min = BAR_H / 2;
    const int y_max = AH - BAR_H / 2;
    const float active_h = static_cast<float>(y_max - y_min);

    #pragma omp parallel for schedule(static)
    for (int y = y_min; y < y_max; ++y) {
        const float yn = (y - y_min) / active_h;  // 0 at top of active picture, 1 at bottom
        for (int x = 0; x < AW; ++x) {
            const float xn = static_cast<float>(x) / static_cast<float>(AW);  // 0..1
            const float aspect_x = (xn - 0.5f) * 2.388f;
            const float aspect_y = (yn - 0.5f);
            Vec3 col{};

            if (act_id == 0) {
                // Prologue: blurred night train window glass & drifting tunnel bokeh
                float blur_grad = 1.0f - yn;
                col = mix3(Vec3{0.014f, 0.025f, 0.044f}, Vec3{0.032f, 0.062f, 0.098f}, blur_grad * 0.7f);
                // Vertical subtle rain condensation streaks
                float streak = vnoise(xn * 90.0f, yn * 12.0f - t * 0.35f);
                col += Vec3{0.020f, 0.038f, 0.055f} * std::pow(streak, 3.0f) * 0.45f;

                // Passing tunnel bokeh orbs
                for (int i = 0; i < 14; ++i) {
                    float fi = static_cast<float>(i);
                    float speed = 0.14f + 0.18f * hash21(fi, 2.0f);
                    float bx = fractf(hash21(fi, 5.0f) - t * speed);
                    float by = 0.18f + 0.64f * hash21(fi, 9.0f);
                    float rad = 0.028f + 0.055f * hash21(fi, 13.0f);
                    float dx = (xn - bx) * 2.388f;
                    float dy = yn - by;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    float orb = smoothstep(rad, rad * 0.25f, dist);
                    bool warm = (i % 3 == 0);
                    Vec3 bc = warm ? Vec3{1.10f, 0.54f, 0.18f} : Vec3{0.24f, 0.62f, 0.92f};
                    col += bc * (orb * 0.28f * (0.6f + af.mid * 0.7f));
                }
            } else if (act_id == 1) {
                // Act I: Storm rain crosswalk + wet asphalt reflections + red->green shift at 36.4s
                const float horizon = 0.63f;
                float green_t = smoothstep(36.1f, 36.8f, t);
                Vec3 sig_col = mix3(Vec3{1.45f, 0.16f, 0.18f}, Vec3{0.18f, 1.45f, 0.72f}, green_t);
                Vec3 neon_left{0.95f, 0.22f, 0.42f};
                Vec3 lamp_mid{0.35f, 0.68f, 0.95f};

                if (yn < horizon) {
                    float sy = yn / horizon;
                    float cloud = fbm3(xn * 3.8f + t * 0.04f, sy * 2.4f);
                    col = mix3(Vec3{0.012f, 0.022f, 0.042f}, Vec3{0.042f, 0.072f, 0.108f}, sy * 0.85f);
                    col += Vec3{0.028f, 0.048f, 0.072f} * cloud * (0.7f + 0.4f * sy);
                } else {
                    // Wet asphalt with concentric raindrop ripples & specular neon reflections
                    float ry = (yn - horizon) / (1.0f - horizon);
                    float mirror_y = horizon - (yn - horizon) * 0.95f;
                    float road_tex = vnoise(xn * 220.0f, yn * 140.0f);
                    col = mix3(Vec3{0.018f, 0.028f, 0.042f}, Vec3{0.008f, 0.012f, 0.020f}, ry);
                    col *= (0.75f + 0.45f * road_tex);

                    // Concentric rain ripples distorting reflections
                    float ripple_dx = 0.0f;
                    float ripple_ring = 0.0f;
                    for (int r = 0; r < 6; ++r) {
                        float fr = static_cast<float>(r);
                        float phase = fractf(t * 0.85f + fr * 0.19f);
                        float rx = 0.15f + 0.70f * hash21(std::floor(t * 0.85f + fr * 0.19f), fr * 7.0f);
                        float ry_c = 0.70f + 0.24f * hash21(fr * 3.0f, std::floor(t * 0.85f + fr * 0.19f));
                        float dx = (xn - rx) * 2.388f;
                        float dy = (yn - ry_c) * 2.1f;
                        float d = std::sqrt(dx * dx + dy * dy);
                        float ring = std::sin((d - phase * 0.38f) * 75.0f) * std::exp(-d * 14.0f) * (1.0f - phase);
                        ripple_dx += ring * 0.015f;
                        ripple_ring += std::max(0.0f, ring) * 0.14f;
                    }
                    float u_r = xn + ripple_dx;
                    // Vertical reflection columns for shop neon, streetlight, and crosswalk signal
                    auto reflect_col = [&](float lx, float width, Vec3 c, float strength) {
                        float d = std::abs(u_r - lx) / width;
                        float streak = std::exp(-d * d * (1.0f + 1.5f * ry));
                        float chop = 0.55f + 0.45f * std::sin(mirror_y * 95.0f + road_tex * 6.0f + t * 4.0f);
                        return c * (streak * chop * strength * (1.0f - 0.35f * ry));
                    };
                    col += reflect_col(0.14f, 0.055f, neon_left, 0.48f);
                    col += reflect_col(0.38f, 0.045f, lamp_mid, 0.35f);
                    col += reflect_col(0.76f, 0.032f, sig_col, 0.65f + 0.25f * green_t);
                    col += Vec3{0.25f, 0.38f, 0.52f} * ripple_ring;
                }
                // Soft atmospheric light halos in fog
                float d_sig = std::hypot((xn - 0.76f) * 2.388f, yn - 0.36f);
                col += sig_col * std::exp(-d_sig * 4.5f) * (0.28f + 0.15f * af.mid);
                float d_neon = std::hypot((xn - 0.14f) * 2.388f, yn - 0.46f);
                col += neon_left * std::exp(-d_neon * 3.8f) * 0.24f;
            } else if (act_id == 2) {
                // Act II: Midnight apartment exterior -> warm amber corner window & nocturnal sky
                float night_grad = 1.0f - yn;
                col = mix3(Vec3{0.016f, 0.024f, 0.042f}, Vec3{0.008f, 0.013f, 0.028f}, night_grad);
                float mist = fbm3(xn * 4.5f + t * 0.02f, yn * 3.2f);
                col += Vec3{0.018f, 0.028f, 0.048f} * mist * 0.5f;

                // Soft warm amber volumetric spill around the protagonist's room window at (0.56, 0.52)
                float dx = (xn - 0.56f) * 2.388f;
                float dy = (yn - 0.50f);
                float d = std::sqrt(dx * dx + dy * dy);
                float glow = std::exp(-d * 3.2f) * (0.75f + 0.30f * af.mid);
                col += Vec3{1.15f, 0.64f, 0.22f} * glow * 0.65f;
            } else if (act_id == 3) {
                // Act III: 06:15 AM misty morning station viaduct & pale peach sunrise haze
                float sy = yn;
                Vec3 zenith{0.028f, 0.062f, 0.125f};
                Vec3 mid_sky{0.105f, 0.195f, 0.295f};
                Vec3 dawn_band{0.78f, 0.46f, 0.28f};
                col = mix3(zenith, mid_sky, smoothstep(0.0f, 0.58f, sy));
                float east = std::exp(-std::pow((xn - 0.78f) * 1.45f, 2.0f)) * smoothstep(0.28f, 0.72f, sy) * smoothstep(0.96f, 0.58f, sy);
                col += dawn_band * east * (0.68f + 0.25f * af.all);

                // Soft slanted atmospheric morning shafts through eastern mist
                float diag = (xn - 0.72f) * 1.65f + (yn - 0.15f) * 0.95f;
                float shaft1 = std::exp(-std::pow((diag + 0.22f) * 4.5f, 2.0f));
                float shaft2 = std::exp(-std::pow((diag - 0.10f) * 5.2f, 2.0f));
                float shaft3 = std::exp(-std::pow((diag - 0.42f) * 3.8f, 2.0f));
                float fog_noise = 0.65f + 0.35f * vnoise(xn * 6.0f + t * 0.05f, yn * 4.0f);
                float shaft_mask = smoothstep(0.92f, 0.10f, yn) * smoothstep(0.08f, 0.75f, xn);
                col += Vec3{0.48f, 0.39f, 0.32f} * ((shaft1 * 0.55f + shaft2 * 0.85f + shaft3 * 0.45f) * fog_noise * shaft_mask * 0.26f);
            } else if (act_id == 4) {
                // Act IV: Chorus 1 — Rising twilight skyline + starry sky + warm horizon glow
                float sy = yn;
                Vec3 zenith{0.012f, 0.022f, 0.065f};
                Vec3 mid{0.055f, 0.095f, 0.210f};
                Vec3 horizon_c{0.48f, 0.26f, 0.20f};
                col = mix3(zenith, mid, smoothstep(0.0f, 0.55f, sy));
                float hor_glow = std::exp(-std::pow((sy - 0.66f) * 5.0f, 2.0f));
                col += horizon_c * hor_glow * (0.75f + 0.40f * af.bass);

                // Milky Way subtle dust band & starfield in upper sky
                if (sy < 0.65f) {
                    float mw_d = std::abs((yn - 0.35f) - (xn - 0.5f) * 0.45f);
                    float mw = std::exp(-mw_d * 5.0f) * fbm3(xn * 5.0f + t * 0.015f, yn * 4.0f);
                    col += Vec3{0.10f, 0.18f, 0.38f} * mw * 0.65f;
                    col += starfield(xn, yn, t, af.air, 1.35f) * (1.0f - smoothstep(0.38f, 0.65f, sy));
                }
            } else if (act_id == 5) {
                // Interlude: Quiet blue-hour rooftop mirror puddle & glowing memory reflection
                const float horizon = 0.64f;
                float sy = (yn < horizon) ? (yn / horizon) : ((yn - horizon) / (1.0f - horizon));
                float sky_y = (yn < horizon) ? sy : (1.0f - sy * 0.92f);
                Vec3 sky_c = mix3(Vec3{0.018f, 0.032f, 0.072f}, Vec3{0.088f, 0.145f, 0.245f}, sky_y);
                sky_c += Vec3{0.42f, 0.28f, 0.18f} * std::exp(-std::pow((sky_y - 0.95f) * 6.0f, 2.0f)) * 0.65f;
                if (sky_y < 0.72f) {
                    sky_c += starfield(xn, sky_y * horizon, t, af.air * 0.4f, 0.75f) * 0.55f;
                }
                if (yn >= horizon) {
                    // Calm puddle darkening + subtle ripple ring around center x=0.5
                    float dx = (xn - 0.50f) * 2.388f;
                    float dy = (yn - 0.74f) * 2.6f;
                    float d = std::sqrt(dx * dx + dy * dy);
                    float ripple = std::sin(d * 52.0f - t * 2.6f) * std::exp(-d * 3.8f);
                    sky_c = sky_c * 0.68f + Vec3{0.95f, 0.70f, 0.36f} * std::max(0.0f, ripple) * 0.18f;
                }
                col = sky_c;
            } else if (act_id == 6) {
                // Act V: Harbor night bay + warm lantern terrace & sea bridge reflection
                const float horizon = 0.59f;
                if (yn < horizon) {
                    float sy = yn / horizon;
                    col = mix3(Vec3{0.014f, 0.022f, 0.048f}, Vec3{0.075f, 0.068f, 0.105f}, sy);
                    col += starfield(xn, yn, t, af.air, 0.85f) * (1.0f - sy * 0.8f) * 0.75f;
                    // Warm terrace ambiance on upper left/center
                    float warm_haze = std::exp(-std::hypot((xn - 0.45f) * 1.4f, yn - 0.42f) * 1.8f);
                    col += Vec3{0.35f, 0.18f, 0.08f} * warm_haze * (0.45f + 0.35f * af.mid);
                } else {
                    float ry = (yn - horizon) / (1.0f - horizon);
                    float wave = vnoise(xn * 26.0f + t * 0.4f, ry * 42.0f - t * 0.8f);
                    col = mix3(Vec3{0.025f, 0.036f, 0.062f}, Vec3{0.010f, 0.016f, 0.030f}, ry);
                    // Broken vertical reflections of bridge & terrace festoon lights
                    for (int k = 0; k < 7; ++k) {
                        float lx = 0.14f + k * 0.12f;
                        float d = std::abs(xn - lx) / (0.018f + 0.015f * ry);
                        float glint = std::exp(-d * d) * smoothstep(0.42f, 0.78f, wave);
                        col += Vec3{1.10f, 0.66f, 0.25f} * glint * (0.38f + 0.25f * af.mid);
                    }
                }
            } else if (act_id == 7) {
                // Act VI: Wind over bay (155..170) -> Open Sea Swells & Moon/Sun Glitter Path (170..184)
                float sail_t = smoothstep(169.5f, 171.2f, t);
                const float horizon = 0.56f;
                if (yn < horizon) {
                    float sy = yn / horizon;
                    float wind_cloud = fbm4(xn * 3.5f - t * 0.28f, sy * 4.5f);
                    Vec3 sky_top = mix3(Vec3{0.018f, 0.042f, 0.095f}, Vec3{0.016f, 0.048f, 0.125f}, sail_t);
                    Vec3 sky_bot = mix3(Vec3{0.18f, 0.26f, 0.38f}, Vec3{0.22f, 0.42f, 0.64f}, sail_t);
                    col = mix3(sky_top, sky_bot, sy);
                    col += Vec3{0.35f, 0.45f, 0.62f} * smoothstep(0.45f, 0.85f, wind_cloud) * 0.42f;
                    // Horizon sunrise/moonlight beacon at x=0.72
                    float d_sun = std::hypot((xn - 0.72f) * 2.0f, (sy - 0.94f) * 1.2f);
                    col += Vec3{1.35f, 0.92f, 0.48f} * std::exp(-d_sun * 3.8f) * (0.55f + 0.35f * sail_t);
                } else {
                    float ry = (yn - horizon) / (1.0f - horizon);
                    float perspective_z = 1.0f / (ry + 0.06f);
                    float w1 = std::sin(xn * 18.0f + perspective_z * 2.2f - t * 3.4f);
                    float w2 = std::sin(xn * 34.0f - perspective_z * 3.5f - t * 2.5f);
                    float slope = (w1 * 0.6f + w2 * 0.4f);
                    Vec3 deep_sea{0.012f, 0.048f, 0.115f};
                    Vec3 crest_sea{0.055f, 0.195f, 0.365f};
                    col = mix3(deep_sea, crest_sea, 0.5f + 0.5f * slope);
                    // Specular glitter path from horizon light at x=0.72
                    float path_w = 0.045f + 0.14f * ry;
                    float path_d = std::abs(xn - 0.72f) / path_w;
                    float glitter = std::exp(-path_d * path_d) * std::pow(std::max(0.0f, slope * 0.5f + 0.5f), 4.0f);
                    col += Vec3{1.65f, 1.15f, 0.62f} * glitter * (0.75f + 0.65f * af.bass);
                    // Horizon atmospheric mist blend
                    col = mix3(Vec3{0.24f, 0.38f, 0.54f}, col, smoothstep(0.0f, 0.14f, ry));
                }
            } else if (act_id == 8) {
                // Act VII: 184.4..195.8 = Burning Sunset Sky; 195.8..203.0 = Macro Human Eye Reflection!
                float eye_blend = smoothstep(195.6f, 196.6f, t);

                // Compute Burning Sunset Sky (rich vermilion-amber clouds over deep indigo tops)
                auto eval_burning_sky = [&](float sx, float sy) {
                    Vec3 top{0.055f, 0.022f, 0.068f};
                    Vec3 mid{0.42f, 0.105f, 0.072f};
                    Vec3 low{0.92f, 0.32f, 0.09f};
                    Vec3 base = (sy < 0.55f)
                        ? mix3(top, mid, sy / 0.55f)
                        : mix3(mid, low, (sy - 0.55f) / 0.45f);

                    float warp = fbm3(sx * 3.2f + t * 0.04f, sy * 2.6f);
                    float cloud = fbm4(sx * 4.5f + warp * 0.85f, sy * 3.8f - t * 0.02f);
                    float lit = smoothstep(0.42f, 0.80f, cloud) * (0.28f + 0.72f * sy);
                    Vec3 ember = mix3(Vec3{1.15f, 0.22f, 0.07f}, Vec3{1.45f, 0.68f, 0.20f}, sy);
                    Vec3 dark_cloud{0.042f, 0.020f, 0.052f};
                    Vec3 c = mix3(base, dark_cloud, smoothstep(0.42f, 0.72f, 1.0f - cloud) * 0.68f);
                    c += ember * lit * 0.82f;
                    float d_sun = std::hypot((sx - 0.52f) * 2.2f, (sy - 0.62f) * 1.7f);
                    c += Vec3{1.25f, 0.66f, 0.22f} * std::exp(-d_sun * 3.8f) * 0.55f;
                    return c;
                };

                if (eye_blend < 0.999f) {
                    if (yn < 0.76f) {
                        col = eval_burning_sky(xn, yn / 0.76f);
                    } else {
                        // Dark wet exit pavement reflecting sunset embers cleanly
                        float py = (yn - 0.76f) / 0.24f;
                        Vec3 refl = eval_burning_sky(xn, 1.0f - py * 0.7f);
                        float tex = vnoise(xn * 90.0f, yn * 120.0f);
                        col = refl * (0.14f + 0.14f * tex) * (1.0f - 0.45f * py);
                    }
                }

                if (eye_blend > 0.001f) {
                    // Macro Eye Shader centered at (0.5, 0.5) with jewel-like saturation
                    float ex = aspect_x * 1.05f;
                    float ey = aspect_y * 1.92f;
                    ex += 0.012f * std::sin(t * 0.9f);
                    ey += 0.006f * std::cos(t * 1.3f);

                    float lid_x = ex / 0.80f;
                    float upper_lid = 0.39f * (1.0f - lid_x * lid_x * 0.84f);
                    float lower_lid = -0.34f * (1.0f - lid_x * lid_x * 0.88f);
                    float inside_eye = smoothstep(upper_lid + 0.022f, upper_lid - 0.018f, ey) *
                                       smoothstep(lower_lid - 0.022f, lower_lid + 0.018f, ey) *
                                       smoothstep(0.94f, 0.72f, std::abs(lid_x));

                    float r = std::sqrt(ex * ex + ey * ey);
                    float ang = std::atan2(ey, ex);

                    // Soft natural sclera with upper eyelid ambient shadow
                    float lid_ao = smoothstep(upper_lid, upper_lid - 0.18f, ey);
                    Vec3 sclera = Vec3{0.28f, 0.25f, 0.27f} * (0.38f + 0.62f * lid_ao);
                    sclera = mix3(Vec3{0.16f, 0.075f, 0.085f}, sclera, smoothstep(0.88f, 0.50f, std::abs(lid_x)));

                    float iris_r = 0.34f;
                    float pupil_r = 0.132f - 0.010f * af.bass;
                    float iris_mask = smoothstep(iris_r + 0.010f, iris_r - 0.010f, r);
                    float pupil_mask = smoothstep(pupil_r + 0.008f, pupil_r - 0.006f, r);

                    // Deep amber-hazel inner ring fading into rich sapphire-indigo radial fibers
                    float fiber1 = vnoise(std::cos(ang) * 16.0f + r * 30.0f, std::sin(ang) * 16.0f + r * 30.0f);
                    float fiber2 = vnoise(ang * 32.0f, r * 48.0f);
                    float fiber = 0.55f * fiber1 + 0.45f * fiber2;
                    float iris_norm = clamp01((r - pupil_r) / (iris_r - pupil_r));
                    Vec3 inner_iris{0.44f, 0.21f, 0.06f};
                    Vec3 outer_iris{0.045f, 0.095f, 0.22f};
                    Vec3 iris_col = mix3(inner_iris, outer_iris, smoothstep(0.12f, 0.78f, iris_norm));
                    iris_col *= (0.45f + 1.05f * fiber);
                    float limbus = smoothstep(0.75f, 0.98f, iris_norm);
                    iris_col = mix3(iris_col, Vec3{0.012f, 0.022f, 0.055f}, limbus * 0.88f);

                    Vec3 eye_c = mix3(sclera, iris_col, iris_mask);
                    eye_c = mix3(eye_c, Vec3{0.006f, 0.006f, 0.012f}, pupil_mask);

                    if (iris_mask > 0.01f) {
                        // Crisp curved cornea reflection of burning sunset horizon inside pupil & iris
                        float refl_u = clamp01(0.5f + ex * 1.55f);
                        float refl_v = clamp01(0.54f + ey * 1.55f);
                        Vec3 sky_refl = eval_burning_sky(refl_u, refl_v);
                        float peak = 0.58f + 0.45f * smoothstep(198.5f, 200.4f, t) * (1.0f + 0.35f * af.onset);
                        float fresnel = 0.16f + 0.28f * std::pow(iris_norm, 2.0f);
                        eye_c += sky_refl * (fresnel * 0.24f * peak) * iris_mask;

                        // Jewel-like crisp specular catchlight on upper-left cornea
                        float cl = std::exp(-std::hypot((ex + 0.11f) * 16.0f, (ey + 0.09f) * 18.0f));
                        eye_c += Vec3{1.85f, 1.35f, 0.85f} * cl * (0.90f * peak) * iris_mask;
                    }

                    // Warm muted skin lighting around eye socket
                    Vec3 skin_bg = Vec3{0.026f, 0.014f, 0.018f} * (1.15f - 0.35f * std::hypot(ex * 0.7f, ey));
                    float tear_line = std::exp(-std::abs(ey - lower_lid) * 65.0f) * smoothstep(0.78f, 0.15f, std::abs(lid_x));
                    Vec3 eye_final = mix3(skin_bg, eye_c, inside_eye);
                    eye_final += Vec3{0.55f, 0.34f, 0.22f} * tear_line * 0.32f;
                    col = mix3(col, eye_final, eye_blend);
                }
            } else if (act_id == 9) {
                // Act VIII: 203..218 Peaks & Azure Sea -> 218..231.8 Milky Way Skyline -> 231.8..239 Silent Cobalt Sea!
                float chorus_t = smoothstep(217.6f, 218.6f, t);
                float silence_t = smoothstep(231.5f, 232.3f, t);

                if (silence_t < 0.999f) {
                    const float horizon = 0.62f;
                    if (yn < horizon) {
                        float sy = yn / horizon;
                        Vec3 dusk_top = mix3(Vec3{0.015f, 0.036f, 0.088f}, Vec3{0.008f, 0.016f, 0.052f}, chorus_t);
                        Vec3 dusk_low = mix3(Vec3{0.18f, 0.38f, 0.62f}, Vec3{0.10f, 0.22f, 0.44f}, chorus_t);
                        col = mix3(dusk_top, dusk_low, sy);
                        float mw_dist = std::abs((yn - 0.36f) + (xn - 0.5f) * 0.52f);
                        float mw = std::exp(-mw_dist * 4.5f) * fbm4(xn * 4.5f - t * 0.02f, yn * 4.0f);
                        col += Vec3{0.18f, 0.38f, 0.78f} * mw * (0.35f + 0.95f * chorus_t);
                        col += starfield(xn, yn, t, af.air, 0.9f + 1.1f * chorus_t) * (1.0f - sy * 0.6f);
                    } else {
                        float ry = (yn - horizon) / (1.0f - horizon);
                        float ripple = vnoise(xn * 28.0f + t * 0.25f, ry * 40.0f - t * 0.6f);
                        col = mix3(Vec3{0.055f, 0.185f, 0.380f}, Vec3{0.016f, 0.062f, 0.155f}, ry);
                        col += Vec3{0.09f, 0.24f, 0.46f} * smoothstep(0.45f, 0.85f, ripple) * (0.45f + 0.45f * af.bass);
                    }
                }
                if (silence_t > 0.001f) {
                    // Sudden crystalline silent cobalt sea with perspective ocean ripples!
                    const float horizon = 0.58f;
                    Vec3 sil{};
                    if (yn < horizon) {
                        float sy = yn / horizon;
                        sil = mix3(Vec3{0.008f, 0.022f, 0.065f}, Vec3{0.088f, 0.235f, 0.465f}, std::pow(sy, 1.35f));
                        sil += starfield(xn, yn, t * 0.6f, 0.15f, 0.7f) * (1.0f - sy);
                    } else {
                        float ry = (yn - horizon) / (1.0f - horizon);
                        float pz = 1.0f / (ry + 0.08f);
                        float w1 = vnoise(xn * 18.0f + t * 0.18f, pz * 2.4f - t * 0.45f);
                        float w2 = std::sin(pz * 3.8f - t * 1.1f + xn * 9.0f) * 0.5f + 0.5f;
                        float calm_w = smoothstep(0.35f, 0.85f, 0.65f * w1 + 0.35f * w2);
                        sil = mix3(Vec3{0.058f, 0.185f, 0.380f}, Vec3{0.012f, 0.048f, 0.135f}, ry);
                        float path = std::exp(-std::pow((xn - 0.50f) * 4.2f, 2.0f));
                        sil += Vec3{0.28f, 0.58f, 0.88f} * (path * calm_w * 0.22f * (1.0f - 0.35f * ry));
                    }
                    col = mix3(col, sil, silence_t);
                }
            } else {
                // Act 10: Epilogue (239.0 .. 265.54) — Deep serene indigo dawn sky so white serif text reads cleanly
                const float horizon = 0.58f;
                if (yn < horizon) {
                    float sy = yn / horizon;
                    col = mix3(Vec3{0.008f, 0.018f, 0.048f}, Vec3{0.062f, 0.135f, 0.265f}, sy);
                    float dawn = std::exp(-std::pow((sy - 0.98f) * 9.0f, 2.0f)) * std::exp(-std::pow((xn - 0.5f) * 1.8f, 2.0f));
                    col += Vec3{0.58f, 0.42f, 0.24f} * dawn * 0.42f;
                    col += starfield(xn, yn, t * 0.4f, 0.1f, 0.5f) * (1.0f - sy * 0.85f) * 0.45f;
                } else {
                    float ry = (yn - horizon) / (1.0f - horizon);
                    float pz = 1.0f / (ry + 0.08f);
                    float w = vnoise(xn * 16.0f + t * 0.15f, pz * 2.2f - t * 0.35f);
                    col = mix3(Vec3{0.038f, 0.112f, 0.225f}, Vec3{0.010f, 0.032f, 0.082f}, ry);
                    col += Vec3{0.26f, 0.38f, 0.54f} * std::exp(-std::pow((xn - 0.5f) * 3.8f, 2.0f)) * smoothstep(0.45f, 0.85f, w) * 0.18f;
                }
            }

            canvas.atmo[y * AW + x] = col;
        }
    }
}

}  // namespace mv
