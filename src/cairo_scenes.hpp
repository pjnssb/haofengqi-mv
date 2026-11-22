#pragma once

#include "audio_lyrics.hpp"
#include "hdr_canvas.hpp"
#include "sky_and_atmosphere.hpp"
#include "storyboard.hpp"
#include <algorithm>
#include <cairo/cairo.h>
#include <cmath>

namespace mv {

inline void draw_traveler(cairo_t* cr, cairo_t* gcr,
                          double fx, double fy, double sc,
                          double facing,
                          double walk_phase,
                          double walk_amp,
                          double head_bow,
                          double arm_reach,
                          double wind,
                          double r_rim, double g_rim, double b_rim, double rim_alpha,
                          double alpha = 1.0) {
    if (alpha <= 0.01) return;
    cairo_save(cr);
    cairo_translate(cr, fx, fy);
    cairo_scale(cr, sc * facing, sc);

    double stride = std::sin(walk_phase) * walk_amp;
    double knee_f = std::max(0.0, std::cos(walk_phase)) * walk_amp * 12.0;
    double knee_b = std::max(0.0, -std::cos(walk_phase)) * walk_amp * 12.0;
    double lean = 0.08 * walk_amp + 0.06 * wind;

    auto limb = [&](double x0, double y0, double x1, double y1, double x2, double y2, double w0, double w1) {
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_set_line_width(cr, w0);
        cairo_move_to(cr, x0, y0);
        cairo_line_to(cr, x1, y1);
        cairo_stroke(cr);
        cairo_set_line_width(cr, w1);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
    };

    cairo_set_source_rgba(cr, 0.032, 0.042, 0.068, alpha);

    double bl_hx = -2.0 + lean * 14.0, bl_hy = -68.0;
    double bl_kx = bl_hx - stride * 18.0 + 4.0, bl_ky = -36.0 - knee_b * 0.4;
    double bl_fx = bl_hx - stride * 32.0 - 2.0, bl_fy = -3.0 - knee_b * 0.65;
    limb(bl_hx, bl_hy, bl_kx, bl_ky, bl_fx, bl_fy, 13.0, 10.0);

    double fl_hx = 3.0 + lean * 14.0, fl_hy = -68.0;
    double fl_kx = fl_hx + stride * 18.0 + 5.0, fl_ky = -36.0 - knee_f * 0.4;
    double fl_fx = fl_hx + stride * 32.0 + 4.0, fl_fy = -3.0 - knee_f * 0.65;
    limb(fl_hx, fl_hy, fl_kx, fl_ky, fl_fx, fl_fy, 13.5, 10.0);

    auto shoe = [&](double sx, double sy) {
        cairo_move_to(cr, sx - 6, sy + 2);
        cairo_line_to(cr, sx + 12, sy + 2);
        cairo_curve_to(cr, sx + 15, sy - 4, sx + 6, sy - 7, sx - 5, sy - 5);
        cairo_close_path(cr);
        cairo_fill(cr);
    };
    shoe(bl_fx, bl_fy);
    shoe(fl_fx, fl_fy);

    // Worn canvas backpack + strap
    double bp_x = -14.0 + lean * 36.0, bp_y = -126.0;
    cairo_save(cr);
    cairo_translate(cr, bp_x, bp_y);
    cairo_rotate(cr, lean * 0.8);
    cairo_move_to(cr, -16, -2);
    cairo_curve_to(cr, -24, 14, -23, 38, -14, 46);
    cairo_line_to(cr, 2, 46);
    cairo_line_to(cr, 3, -2);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_set_line_width(cr, 2.2);
    double strap_sway = wind * 14.0 + std::sin(walk_phase * 1.4) * 3.0;
    cairo_move_to(cr, -16, 43);
    cairo_curve_to(cr, -20 - strap_sway * 0.4, 54, -22 - strap_sway, 62, -19 - strap_sway * 1.15, 70);
    cairo_stroke(cr);
    cairo_restore(cr);

    // Coat & torso with wind-blown hem
    double sh_x = 4.0 + lean * 46.0, sh_y = -132.0;
    double hem_f = wind * 22.0 + std::sin(walk_phase * 1.6 + 0.5) * 4.5 * (0.4 + wind);
    cairo_move_to(cr, sh_x - 12, sh_y);
    cairo_curve_to(cr, sh_x + 2, sh_y - 4, sh_x + 14, sh_y + 2, sh_x + 13, sh_y + 16);
    cairo_curve_to(cr, sh_x + 12, sh_y + 42, 14.0 + lean * 12.0, -76, 15.0 + stride * 7.0, -52);
    cairo_curve_to(cr, 2.0, -48, -12.0 - hem_f * 0.5, -49, -18.0 - hem_f, -53);
    cairo_curve_to(cr, -16.0 - hem_f * 0.3, -78, sh_x - 14, sh_y + 22, sh_x - 12, sh_y);
    cairo_close_path(cr);
    cairo_fill(cr);

    double hx = sh_x + 2.0 + head_bow * 5.0;
    double hy = sh_y - 16.0 + head_bow * 5.0;
    cairo_move_to(cr, sh_x - 7, sh_y + 2);
    cairo_line_to(cr, hx - 3, hy + 8);
    cairo_line_to(cr, hx + 6, hy + 8);
    cairo_line_to(cr, sh_x + 9, sh_y + 3);
    cairo_close_path(cr);
    cairo_fill(cr);

    if (wind > 0.22) {
        double wlen = 16.0 + wind * 24.0;
        double wave = std::sin(walk_phase * 2.2 + wind * 2.0) * (2.5 + wind * 3.5);
        cairo_move_to(cr, sh_x - 4, sh_y + 2);
        cairo_curve_to(cr, sh_x - 14 - wlen * 0.4, sh_y + 5 + wave * 0.4,
                       sh_x - 18 - wlen * 0.85, sh_y + 14 + wave,
                       sh_x - 22 - wlen, sh_y + 22 + wave * 1.1);
        cairo_curve_to(cr, sh_x - 16 - wlen * 0.7, sh_y + 18 + wave * 0.7,
                       sh_x - 11 - wlen * 0.3, sh_y + 10,
                       sh_x - 2, sh_y + 7);
        cairo_close_path(cr);
        cairo_fill(cr);
    }

    cairo_save(cr);
    cairo_translate(cr, hx, hy);
    cairo_rotate(cr, head_bow * 0.45);
    cairo_scale(cr, 1.0, 1.15);
    cairo_arc(cr, 0, 0, 10.5, 0, 2.0 * M_PI);
    cairo_fill(cr);
    cairo_move_to(cr, -9, -3);
    cairo_curve_to(cr, -14 - wind * 5.0, -10, -4, -14, 7, -9);
    cairo_curve_to(cr, 11, -5, 10, 1, 7, 3);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_restore(cr);

    double arm_sh_x = sh_x + 4.0, arm_sh_y = sh_y + 8.0;
    double elb_x, elb_y, hand_x, hand_y;
    if (arm_reach > 0.05) {
        double r_ang = -0.35 + arm_reach * 1.45;
        elb_x = arm_sh_x + std::cos(r_ang - 0.3) * 20.0;
        elb_y = arm_sh_y - std::sin(r_ang - 0.3) * 20.0;
        hand_x = elb_x + std::cos(r_ang + 0.15) * 22.0;
        hand_y = elb_y - std::sin(r_ang + 0.15) * 22.0;
    } else {
        double swing = -stride * 0.65;
        elb_x = arm_sh_x + std::sin(swing) * 18.0 + 3.0;
        elb_y = arm_sh_y + std::cos(swing) * 18.0;
        hand_x = elb_x + std::sin(swing + 0.25) * 18.0 + 4.0;
        hand_y = elb_y + std::cos(swing + 0.25) * 18.0;
    }
    cairo_set_source_rgba(cr, 0.038, 0.048, 0.078, alpha);
    limb(arm_sh_x, arm_sh_y, elb_x, elb_y, hand_x, hand_y, 8.2, 6.4);

    if (rim_alpha > 0.01) {
        cairo_set_source_rgba(cr, r_rim, g_rim, b_rim, rim_alpha * alpha);
        cairo_set_line_width(cr, 1.8);
        cairo_move_to(cr, sh_x + 11, sh_y + 2);
        cairo_curve_to(cr, sh_x + 14, sh_y + 30, 15.0 + lean * 12.0, -76, 15.0 + stride * 7.0, -54);
        cairo_stroke(cr);

        cairo_save(cr);
        cairo_translate(cr, hx, hy);
        cairo_rotate(cr, head_bow * 0.45);
        cairo_arc(cr, 0, 0, 10.6, -1.2, 0.7);
        cairo_stroke(cr);
        cairo_restore(cr);

        if (gcr) {
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_translate(gcr, fx, fy);
            cairo_scale(gcr, sc * facing, sc);
            cairo_set_source_rgba(gcr, r_rim, g_rim, b_rim, rim_alpha * 0.50 * alpha);
            cairo_set_line_width(gcr, 3.0);
            cairo_move_to(gcr, sh_x + 11, sh_y + 2);
            cairo_line_to(gcr, 15.0 + lean * 12.0, -76);
            cairo_stroke(gcr);
            cairo_restore(gcr);
        }
    }
    cairo_restore(cr);
}

inline void draw_paper_motif(cairo_t* cr, cairo_t* gcr, double cx, double cy, double size,
                             double angle, int mode, double flap, double alpha,
                             double r = 0.98, double g = 0.93, double b = 0.80) {
    if (alpha <= 0.01) return;
    auto draw_on = [&](cairo_t* c, double mult, double lw) {
        cairo_save(c);
        cairo_translate(c, cx, cy);
        cairo_rotate(c, angle);
        cairo_scale(c, size, size);
        if (mode == 0) {
            cairo_move_to(c, -1.0, -0.55);
            cairo_line_to(c, 1.0, -0.55);
            cairo_line_to(c, 1.0, 0.55);
            cairo_line_to(c, -1.0, 0.55);
            cairo_close_path(c);
            cairo_set_source_rgba(c, r, g, b, 0.88 * alpha * mult);
            cairo_fill(c);
            cairo_set_source_rgba(c, r * 0.65, g * 0.68, b * 0.72, 0.65 * alpha * mult);
            cairo_set_line_width(c, lw);
            cairo_move_to(c, -0.8, -0.4);
            cairo_line_to(c, 0.6, 0.5);
            cairo_move_to(c, -0.65, 0.15);
            cairo_line_to(c, 0.45, 0.15);
            cairo_stroke(c);
        } else if (mode == 1) {
            double wy = -0.55 + std::sin(flap) * 0.35;
            cairo_move_to(c, -0.9, 0.15);
            cairo_line_to(c, 0.0, -0.15);
            cairo_line_to(c, 1.0, 0.0);
            cairo_line_to(c, 0.15, 0.25);
            cairo_close_path(c);
            cairo_set_source_rgba(c, r, g, b, 0.92 * alpha * mult);
            cairo_fill(c);
            cairo_move_to(c, -0.1, -0.05);
            cairo_line_to(c, 0.35, wy - 0.55);
            cairo_line_to(c, 0.45, 0.05);
            cairo_close_path(c);
            cairo_set_source_rgba(c, r * 0.88, g * 0.90, b * 0.95, 0.84 * alpha * mult);
            cairo_fill(c);
        } else {
            cairo_move_to(c, -1.1, 0.15);
            cairo_line_to(c, 1.1, 0.15);
            cairo_line_to(c, 0.68, 0.55);
            cairo_line_to(c, -0.68, 0.55);
            cairo_close_path(c);
            cairo_set_source_rgba(c, r * 0.86, g * 0.88, b * 0.92, 0.92 * alpha * mult);
            cairo_fill(c);
            cairo_move_to(c, -0.05, 0.12);
            cairo_line_to(c, 0.05, -0.95);
            cairo_line_to(c, 0.72, 0.12);
            cairo_close_path(c);
            cairo_set_source_rgba(c, r, g, b, 0.96 * alpha * mult);
            cairo_fill(c);
            cairo_move_to(c, -0.75, 0.12);
            cairo_line_to(c, 0.02, -0.78);
            cairo_line_to(c, -0.05, 0.12);
            cairo_close_path(c);
            cairo_set_source_rgba(c, r * 0.78, g * 0.82, b * 0.90, 0.88 * alpha * mult);
            cairo_fill(c);
        }
        cairo_restore(c);
    };
    draw_on(cr, 1.0, 0.05);
    if (gcr) {
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        draw_on(gcr, 0.50, 0.07);
        cairo_restore(gcr);
    }
}

inline void draw_rain(cairo_t* cr, double t, double slant_dx, double intensity, int count) {
    if (intensity <= 0.02) return;
    cairo_save(cr);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    for (int i = 0; i < count; ++i) {
        float fi = static_cast<float>(i);
        float z = 0.4f + 0.6f * hash21(fi, 11.3f);
        float speed = 960.0f * z;
        float x0 = fractf(hash21(fi, 3.7f) + t * (slant_dx * 0.22f * z)) * (W + 300.0f) - 150.0f;
        float y0 = fractf(hash21(fi, 17.9f) + t * speed / static_cast<float>(H)) * H;
        float len = (28.0f + 46.0f * z) * (0.7f + 0.5f * static_cast<float>(intensity));
        float dx = static_cast<float>(slant_dx) * len * 0.48f;
        cairo_set_source_rgba(cr, 0.74, 0.86, 0.98, (0.12 + 0.24 * z) * intensity);
        cairo_set_line_width(cr, 0.7 + 0.9 * z);
        cairo_move_to(cr, x0, y0);
        cairo_line_to(cr, x0 + dx, y0 + len);
        cairo_stroke(cr);
    }
    cairo_restore(cr);
}

inline void draw_cable_bridge(cairo_t* cr, cairo_t* gcr, double y_deck, double t, double glow_boost) {
    cairo_save(cr);
    double pylon_xs[2] = {540.0, 1380.0};
    double pylon_h = 250.0;

    cairo_set_source_rgba(cr, 0.04, 0.06, 0.10, 0.96);
    cairo_rectangle(cr, 0, y_deck, W, 12.0);
    cairo_fill(cr);

    for (double px : pylon_xs) {
        cairo_set_source_rgba(cr, 0.045, 0.065, 0.11, 0.96);
        cairo_set_line_width(cr, 10.0);
        cairo_move_to(cr, px - 24.0, y_deck + 36.0);
        cairo_line_to(cr, px - 3.0, y_deck - pylon_h);
        cairo_line_to(cr, px + 3.0, y_deck - pylon_h);
        cairo_line_to(cr, px + 24.0, y_deck + 36.0);
        cairo_stroke(cr);

        for (int c = 1; c <= 12; ++c) {
            double frac = c / 13.0;
            double span = frac * 420.0;
            double cy_top = y_deck - pylon_h * (0.34 + 0.63 * frac);
            cairo_set_source_rgba(cr, 0.64, 0.78, 0.96, 0.18 + 0.15 * glow_boost);
            cairo_set_line_width(cr, 1.15);
            cairo_move_to(cr, px, cy_top);
            cairo_line_to(cr, px - span, y_deck);
            cairo_move_to(cr, px, cy_top);
            cairo_line_to(cr, px + span, y_deck);
            cairo_stroke(cr);
        }

        if (gcr) {
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_set_source_rgba(gcr, 1.0, 0.64, 0.30, 0.55 + 0.35 * glow_boost);
            cairo_arc(gcr, px, y_deck - pylon_h - 4.0, 5.5, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }
    }

    for (int i = 0; i < 28; ++i) {
        double dir = (i % 2 == 0) ? 1.0 : -1.0;
        double spd = 0.042 + 0.025 * hash21(i, 4.1f);
        double cx = fractf(hash21(i, 9.2f) + dir * t * spd) * W;
        double cy = y_deck + (dir > 0 ? 2.5 : 6.0);
        if (dir > 0) cairo_set_source_rgba(cr, 1.0, 0.88, 0.68, 0.85);
        else cairo_set_source_rgba(cr, 1.0, 0.32, 0.25, 0.80);
        cairo_arc(cr, cx, cy, 2.4, 0, 2.0 * M_PI);
        cairo_fill(cr);
        if (gcr) {
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            if (dir > 0) cairo_set_source_rgba(gcr, 1.0, 0.78, 0.45, 0.62);
            else cairo_set_source_rgba(gcr, 1.0, 0.26, 0.20, 0.52);
            cairo_arc(gcr, cx, cy, 3.8, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }
    }
    cairo_restore(cr);
}

inline void render_cairo_shot(cairo_t* cr, cairo_t* gcr, SceneKind kind, double time_sec,
                              float prog, const AudioFrame& af, double shot_alpha = 1.0) {
    const double t = time_sec;
    cairo_save(cr);

    switch (kind) {
    case SceneKind::METRO_WINDOW: {
        // Full-bleed panoramic metro cabin window: thin vertical steel mullion + overhead handrail + glass beads
        cairo_set_source_rgba(cr, 0.025, 0.032, 0.048, 0.88);
        cairo_rectangle(cr, 0, 0, W, 38);
        cairo_rectangle(cr, 0, H - 36, W, 36);
        cairo_rectangle(cr, 1390 - prog * 40.0, 0, 16, H);
        cairo_fill(cr);
        // Specular grab pole highlight
        cairo_set_source_rgba(cr, 0.40, 0.62, 0.86, 0.30);
        cairo_rectangle(cr, 1392 - prog * 40.0, 0, 3, H);
        cairo_fill(cr);
        // Subtle horizontal luggage rack / handrail reflection
        cairo_set_source_rgba(cr, 0.55, 0.72, 0.90, 0.10);
        cairo_rectangle(cr, 0, 125, W, 4);
        cairo_fill(cr);
        for (int i = 0; i < 64; ++i) {
            double bx = hash21(i, 2.3f) * W;
            double by = std::fmod(hash21(i, 8.7f) * H + t * (10.0 + 18.0 * hash21(i, 4.0f)), static_cast<double>(H));
            double br = 1.6 + 3.4 * hash21(i, 15.1f);
            cairo_set_source_rgba(cr, 0.70, 0.84, 0.98, 0.22);
            cairo_arc(cr, bx, by, br, 0, 2.0 * M_PI);
            cairo_fill(cr);
        }
        break;
    }
    case SceneKind::RAIN_SKYSCRAPERS_UP: {
        // Low-angle looking UP wet neon skyscrapers into stormy night sky (tilt-up parallax)
        double tilt = prog * 65.0;
        cairo_set_source_rgba(cr, 0.015, 0.022, 0.038, 0.96);
        // Converging left & right towers leaving dramatic sky canyon down center
        cairo_move_to(cr, 0, H);
        cairo_line_to(cr, 0, 90 + tilt);
        cairo_line_to(cr, 480, 220 + tilt * 0.6);
        cairo_line_to(cr, 590, H);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_move_to(cr, 240, H);
        cairo_line_to(cr, 280, 360 + tilt * 0.5);
        cairo_line_to(cr, 710, 440 + tilt * 0.5);
        cairo_line_to(cr, 780, H);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_move_to(cr, W, H);
        cairo_line_to(cr, W, 60 + tilt);
        cairo_line_to(cr, 1410, 210 + tilt * 0.6);
        cairo_line_to(cr, 1290, H);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Glowing skyscraper window grids & rooftop beacons
        cairo_set_source_rgba(cr, 0.96, 0.78, 0.44, 0.42);
        for (int r = 0; r < 14; ++r) {
            for (int c = 0; c < 6; ++c) {
                if (hash21(r * 7, c * 13) > 0.45f) {
                    cairo_rectangle(cr, 65 + c * 62, 260 + tilt * 0.6 + r * 52, 22, 16);
                    cairo_rectangle(cr, 1460 + c * 68, 260 + tilt * 0.6 + r * 52, 22, 16);
                }
            }
        }
        cairo_fill(cr);

        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.30, 0.42, 0.85);
        cairo_arc(gcr, 480, 216 + tilt * 0.6, 8.0, 0, 2.0 * M_PI);
        cairo_arc(gcr, 1410, 206 + tilt * 0.6, 8.0, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);

        draw_rain(cr, t, 0.08, 0.90, 230);
        break;
    }
    case SceneKind::PUDDLE_NEON_CLOSEUP: {
        // Shot 03: Low puddle close-up! Stone pavement joints, splashing rain crowns, wind-blown rain at 19.4s
        double hor = H * 0.22;
        double wind_gust = smoothstep(18.8f, 20.8f, static_cast<float>(t));
        // Distant neon storefront awning silhouette at top
        cairo_set_source_rgba(cr, 0.02, 0.025, 0.042, 0.96);
        cairo_rectangle(cr, 0, 0, W, hor);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.98, 0.28, 0.48, 0.88);
        cairo_rectangle(cr, 280, hor - 42, 320, 14);
        cairo_fill(cr);
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.28, 0.48, 0.90);
        cairo_rectangle(gcr, 280, hor - 44, 320, 18);
        cairo_fill(gcr);
        cairo_restore(gcr);

        // Perspective stone curb lines across wet pavement
        cairo_set_source_rgba(cr, 0.04, 0.05, 0.08, 0.55);
        cairo_set_line_width(cr, 2.0);
        for (int k = -4; k <= 4; ++k) {
            cairo_move_to(cr, W * 0.5 + k * 210, hor);
            cairo_line_to(cr, W * 0.5 + k * 520, H);
        }
        cairo_stroke(cr);

        // Raindrop splash crowns on wet ground
        for (int s = 0; s < 14; ++s) {
            double ph = fractf(t * 1.45 + s * 0.13);
            double sx = 160.0 + hash21(std::floor(t * 1.45 + s * 0.13), s * 5.0f) * (W - 320.0);
            double sy = hor + 120.0 + hash21(s * 9.0f, std::floor(t * 1.45 + s * 0.13)) * (H - hor - 200.0);
            double rad = 6.0 + ph * 34.0;
            cairo_set_source_rgba(cr, 0.72, 0.86, 1.0, (1.0 - ph) * 0.45);
            cairo_set_line_width(cr, 1.8);
            cairo_save(cr);
            cairo_translate(cr, sx, sy);
            cairo_scale(cr, 1.0, 0.35);
            cairo_arc(cr, 0, 0, rad, 0, 2.0 * M_PI);
            cairo_stroke(cr);
            cairo_restore(cr);
        }

        // Paper corner fluttering in wind across the puddle when "总有好风" hits!
        if (wind_gust > 0.05) {
            double px = 480.0 + wind_gust * 560.0;
            double py = 680.0 - std::sin(wind_gust * M_PI) * 110.0;
            draw_paper_motif(cr, gcr, px, py, 26.0, -0.3 + wind_gust * 0.8, 0, 0.0, 0.85 * wind_gust);
        }

        draw_rain(cr, t, 0.10 + 0.72 * wind_gust, 0.96, 260);
        break;
    }
    case SceneKind::HANDS_TICKET_MACRO: {
        // Shot 04 ("信白手 信它不负我"): Close-up cupped hands & coat sleeves holding creased paper ticket!
        double sway_y = std::sin(t * 1.6) * 14.0;
        double zoom = 1.0 + prog * 0.08;
        cairo_save(cr);
        cairo_translate(cr, W * 0.5, H * 0.56 + sway_y);
        cairo_scale(cr, zoom, zoom);

        cairo_set_source_rgba(cr, 0.025, 0.032, 0.052, 0.98);
        cairo_move_to(cr, -780, 520);
        cairo_line_to(cr, -260, 120);
        cairo_line_to(cr, -170, 220);
        cairo_line_to(cr, -520, 540);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_move_to(cr, 780, 520);
        cairo_line_to(cr, 250, 110);
        cairo_line_to(cr, 160, 215);
        cairo_line_to(cr, 520, 540);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_set_source_rgba(cr, 0.045, 0.052, 0.075, 0.98);
        cairo_move_to(cr, -255, 130);
        cairo_curve_to(cr, -195, 60, -110, 48, -68, 72);
        cairo_curve_to(cr, -54, 108, -96, 178, -175, 215);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_move_to(cr, 245, 120);
        cairo_curve_to(cr, 185, 52, 105, 42, 65, 68);
        cairo_curve_to(cr, 50, 105, 92, 175, 168, 210);
        cairo_close_path(cr);
        cairo_fill(cr);

        draw_paper_motif(cr, nullptr, 0, 26, 98.0, -0.06 + std::sin(t * 1.1) * 0.03, 0, 0.0, 0.96, 0.99, 0.94, 0.82);

        cairo_set_source_rgba(cr, 1.0, 0.78, 0.44, 0.55);
        cairo_set_line_width(cr, 2.8);
        cairo_move_to(cr, -220, 95);
        cairo_curve_to(cr, -165, 56, -100, 52, -72, 70);
        cairo_move_to(cr, 215, 88);
        cairo_curve_to(cr, 160, 50, 98, 48, 68, 66);
        cairo_stroke(cr);
        cairo_restore(cr);

        // Apply identical transform on glow layer for warm ticket luminescence
        if (gcr) {
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_translate(gcr, W * 0.5, H * 0.56 + sway_y);
            cairo_scale(gcr, zoom, zoom);
            draw_paper_motif(gcr, nullptr, 0, 26, 98.0, -0.06 + std::sin(t * 1.1) * 0.03, 0, 0.0, 0.55, 1.0, 0.92, 0.72);
            cairo_restore(gcr);
        }

        draw_rain(cr, t, 0.25, 0.70, 150);
        break;
    }
    case SceneKind::CROSSWALK_WAIT_CARS: {
        // Shot 05: Standing still crying at crosswalk curb while blurred car lights race across
        double hor = H * 0.58;
        cairo_set_source_rgba(cr, 0.018, 0.025, 0.042, 0.96);
        cairo_rectangle(cr, 0, hor - 380, 420, 384);
        cairo_rectangle(cr, 1460, hor - 420, 460, 424);
        cairo_fill(cr);

        // Zebra crosswalk stripes
        for (int s = 0; s < 6; ++s) {
            double u0 = 0.20 + s * 0.115;
            double x0t = 960 + (u0 - 0.5) * 900, x1t = 960 + (u0 + 0.055 - 0.5) * 900;
            double x0b = 960 + (u0 - 0.5) * 1920, x1b = 960 + (u0 + 0.055 - 0.5) * 1920;
            cairo_move_to(cr, x0t, hor + 24);
            cairo_line_to(cr, x1t, hor + 24);
            cairo_line_to(cr, x1b, H);
            cairo_line_to(cr, x0b, H);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 0.55, 0.65, 0.78, 0.18);
            cairo_fill(cr);
        }

        // Red pedestrian signal on right
        double sx = 1480, sy = hor - 260;
        cairo_set_source_rgba(cr, 0.02, 0.03, 0.045, 0.98);
        cairo_rectangle(cr, sx - 6, sy, 12, hor - sy + 42);
        cairo_rectangle(cr, sx - 26, sy - 54, 52, 96);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 1.0, 0.22, 0.22, 0.95);
        cairo_arc(cr, sx, sy - 28, 12, 0, 2.0 * M_PI);
        cairo_fill(cr);
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.22, 0.22, 0.92);
        cairo_arc(gcr, sx, sy - 28, 18, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);

        // Motion-blurred car headlight/taillight streaks passing in foreground street
        for (int c = 0; c < 3; ++c) {
            double dir = (c % 2 == 0) ? 1.0 : -1.0;
            double cx = fractf(t * 0.42 + c * 0.37) * (W + 700.0) - 350.0;
            if (dir < 0) cx = W - cx;
            double cy = hor + 60.0 + c * 36.0;
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            if (dir > 0) cairo_set_source_rgba(gcr, 1.0, 0.90, 0.65, 0.52);
            else cairo_set_source_rgba(gcr, 1.0, 0.22, 0.20, 0.48);
            cairo_rectangle(gcr, cx - 180, cy - 6, 360, 12);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }

        // Bowing stationary traveler silhouette with backpack
        double px = 640.0 + prog * 18.0, py = hor + 160.0;
        draw_traveler(cr, gcr, px, py, 1.85, 1.0, t, 0.0, 0.92, 0.0, 0.35, 1.0, 0.28, 0.28, 0.58, 1.0);
        draw_rain(cr, t, 0.32, 0.92, 220);
        break;
    }
    case SceneKind::CROSSWALK_GREEN_STRIDE: {
        // Shot 06: Low-angle tracking shot beside striding traveler as green light bathes wet zebra stripes!
        double hor = H * 0.44;
        double track_x = -prog * 140.0;
        for (int s = 0; s < 8; ++s) {
            double x0 = s * 300.0 + track_x;
            cairo_move_to(cr, x0, hor + 40);
            cairo_line_to(cr, x0 + 130, hor + 40);
            cairo_line_to(cr, x0 + 60, H);
            cairo_line_to(cr, x0 - 110, H);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 0.50, 0.72, 0.78, 0.22);
            cairo_fill(cr);
        }
        // Bright emerald pedestrian signal mast on right
        double sx = 1540.0, sy = hor - 210.0;
        cairo_set_source_rgba(cr, 0.02, 0.03, 0.045, 0.98);
        cairo_rectangle(cr, sx - 6, sy, 12, hor - sy + 90);
        cairo_rectangle(cr, sx - 28, sy - 58, 56, 102);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.18, 0.98, 0.62, 0.96);
        cairo_arc(cr, sx, sy + 14, 14, 0, 2.0 * M_PI);
        cairo_fill(cr);
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 0.18, 0.98, 0.62, 0.95);
        cairo_arc(gcr, sx, sy + 14, 22, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);

        double px = 680.0 + prog * 360.0, py = hor + 390.0;
        cairo_save(cr);
        cairo_translate(cr, px, py + 6);
        cairo_scale(cr, 1.0, -0.48);
        draw_traveler(cr, nullptr, 0, 0, 2.35, 1.0, prog * 18.0, 1.0, -0.05, 0.0, 0.55, 0.18, 0.98, 0.62, 0.18, 0.28);
        cairo_restore(cr);

        draw_traveler(cr, gcr, px, py, 2.35, 1.0, prog * 18.0, 1.0, -0.05, 0.0, 0.55, 0.18, 0.98, 0.62, 0.72, 1.0);
        draw_paper_motif(cr, gcr, px + 44.0, py - 215.0, 16.0, -0.2, 0, 0.0, 0.80);
        draw_rain(cr, t, 0.45, 0.75, 190);
        break;
    }
    case SceneKind::ROOM_DESK_LAMP: {
        // Shot 07: Full-bleed cozy apartment interior (NO dark border around walls!)
        double cam_dx = -prog * 45.0;
        cairo_save(cr);
        cairo_translate(cr, cam_dx, 0);

        // Panoramic rain window mullions gently framing night city outside (left/center)
        cairo_set_source_rgba(cr, 0.035, 0.028, 0.035, 0.92);
        cairo_rectangle(cr, -80, H - 235, W + 220, 240);  // full foreground desk surface
        cairo_fill(cr);
        // Window vertical frames (thin architectural lines, NOT black blocks)
        cairo_set_source_rgba(cr, 0.04, 0.032, 0.038, 0.90);
        cairo_rectangle(cr, 480, 0, 14, H - 235);
        cairo_rectangle(cr, 1080, 0, 14, H - 235);
        cairo_rectangle(cr, 0, 480, W + 120, 12);
        cairo_fill(cr);

        // Soft fluttering translucent curtain on left edge
        cairo_set_source_rgba(cr, 0.85, 0.76, 0.64, 0.14);
        cairo_move_to(cr, -40, 0);
        cairo_line_to(cr, 210 + std::sin(t * 1.4) * 18.0, 0);
        cairo_curve_to(cr, 185 + std::sin(t * 1.2) * 26.0, 420, 235 + std::cos(t * 1.1) * 24.0, 680, 215, H - 235);
        cairo_line_to(cr, -40, H - 235);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Bookshelf / Stacked books & warm ceramic mug on foreground wooden desk
        cairo_set_source_rgba(cr, 0.05, 0.035, 0.042, 0.98);
        cairo_rectangle(cr, 680, H - 295, 165, 60);
        cairo_rectangle(cr, 700, H - 335, 130, 40);
        cairo_rectangle(cr, 950, H - 298, 48, 63);  // ceramic mug
        cairo_fill(cr);
        // Mug handle
        cairo_set_line_width(cr, 6.0);
        cairo_arc(cr, 998, H - 266, 15, -M_PI * 0.5, M_PI * 0.5);
        cairo_stroke(cr);

        // Unfolded creased paper ticket / architectural sketch glowing softly under lamp
        draw_paper_motif(cr, nullptr, 1170, H - 246, 56.0, 0.04, 0, 0.0, 0.94);

        // Gooseneck brass desk lamp (right side)
        cairo_set_source_rgba(cr, 0.04, 0.03, 0.038, 0.98);
        cairo_rectangle(cr, 1350, H - 252, 130, 17);
        cairo_fill(cr);
        cairo_set_line_width(cr, 11.0);
        cairo_move_to(cr, 1415, H - 252);
        cairo_curve_to(cr, 1445, H - 560, 1295, H - 635, 1205, H - 525);
        cairo_stroke(cr);
        cairo_move_to(cr, 1148, H - 498);
        cairo_line_to(cr, 1235, H - 555);
        cairo_line_to(cr, 1255, H - 472);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Volumetric golden lamp cone & bulb glow
        cairo_set_source_rgba(cr, 1.0, 0.88, 0.62, 0.22 + 0.06 * af.mid);
        cairo_move_to(cr, 1185, H - 490);
        cairo_line_to(cr, 920, H - 235);
        cairo_line_to(cr, 1390, H - 235);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.78, 0.36, 0.88);
        cairo_arc(gcr, 1198, H - 498, 28.0, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);

        // Rising gentle steam curls from mug
        cairo_set_source_rgba(cr, 1.0, 0.94, 0.84, 0.42);
        cairo_set_line_width(cr, 3.0);
        for (int s = 0; s < 3; ++s) {
            double sx = 964.0 + s * 10.0, sy = H - 306.0;
            cairo_move_to(cr, sx, sy);
            cairo_curve_to(cr, sx + std::sin(t * 2.1 + s) * 16, sy - 42,
                           sx - std::cos(t * 1.7 + s) * 16, sy - 86,
                           sx + std::sin(t * 1.4 + s) * 18, sy - 135);
            cairo_stroke(cr);
        }
        cairo_restore(cr);
        break;
    }
    case SceneKind::ROOM_PILLOW_WINDOW: {
        // Shot 08 ("信入睡的枕头"): Moonlit pillow foreground left + resting figure beside wide panoramic city window!
        double pan = prog * 36.0;
        cairo_save(cr);
        cairo_translate(cr, pan, 0);

        double sill_y = H - 270.0;
        // Distant nocturnal city skyline cleanly behind window glass
        for (int i = 0; i < 14; ++i) {
            double bx = 620.0 + i * 98.0;
            double bw = 64.0 + hash21(i, 3.1f) * 24.0;
            double bh = 160.0 + hash21(i, 7.1f) * 260.0;
            cairo_set_source_rgba(cr, 0.018, 0.028, 0.052, 0.92);
            cairo_rectangle(cr, bx, sill_y - bh, bw, bh + 12.0);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, 1.0, 0.80, 0.44, 0.50);
            for (int r = 18; r < bh - 20; r += 26) {
                for (int c = 0; c < 3; ++c) {
                    if (hash21(i * 17.0f + c, r) > 0.65f) {
                        cairo_rectangle(cr, bx + 10 + c * 18, sill_y - bh + r, 8, 10);
                    }
                }
            }
            cairo_fill(cr);
        }

        // Thin window frame mullion
        cairo_set_source_rgba(cr, 0.025, 0.030, 0.046, 0.96);
        cairo_rectangle(cr, 1340, 0, 14, sill_y);
        cairo_fill(cr);

        // Opaque indoor bedroom wall, sill & bed silhouette (drawn cleanly OVER city below sill_y!)
        cairo_set_source_rgba(cr, 0.022, 0.026, 0.042, 1.0);
        cairo_rectangle(cr, -80, sill_y, W + 200, H - sill_y + 20);
        cairo_rectangle(cr, -80, H - 295, 920, 300);
        cairo_rectangle(cr, -60, H - 430, 44, 430);
        cairo_fill(cr);

        // Soft glowing pillow catching silver moonlight
        cairo_set_source_rgba(cr, 0.88, 0.91, 0.97, 0.94);
        cairo_move_to(cr, 40, H - 295);
        cairo_curve_to(cr, 80, H - 380, 300, H - 380, 355, H - 295);
        cairo_close_path(cr);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.64, 0.72, 0.84, 0.42);
        cairo_set_line_width(cr, 2.5);
        cairo_move_to(cr, 85, H - 320);
        cairo_curve_to(cr, 155, H - 340, 225, H - 340, 290, H - 320);
        cairo_stroke(cr);

        // Protagonist silhouette standing/leaning quietly beside wide window on right
        draw_traveler(cr, gcr, 1180, sill_y + 185.0, 2.15, -1.0, 0.0, 0.0, -0.14, 0.0, 0.04, 1.0, 0.78, 0.46, 0.68, 1.0);
        cairo_restore(cr);
        break;
    }
    case SceneKind::VIADUCT_EXPRESS_TRAIN: {
        // Shot 09 ("时代轰隆隆地来不转弯"): Misty hills + diagonal viaduct + fast commuter train!
        double hy = H * 0.72;
        cairo_set_source_rgba(cr, 0.065, 0.125, 0.20, 0.65);
        cairo_move_to(cr, 0, H);
        for (int x = 0; x <= W; x += 40) {
            cairo_line_to(cr, x, hy - 140 - 90 * std::sin(x * 0.0032 + 1.0));
        }
        cairo_line_to(cr, W, H);
        cairo_close_path(cr);
        cairo_fill(cr);

        double track_y = H * 0.54;
        cairo_set_source_rgba(cr, 0.028, 0.044, 0.068, 0.96);
        cairo_rectangle(cr, 0, track_y, W, 28);
        for (int px = 120; px < W; px += 340) {
            cairo_rectangle(cr, px, track_y + 24, 30, H - track_y);
            cairo_rectangle(cr, px + 8, track_y - 90, 9, 94);
            cairo_rectangle(cr, px - 16, track_y - 86, 40, 5);
        }
        cairo_fill(cr);

        cairo_set_source_rgba(cr, 0.22, 0.34, 0.46, 0.42);
        cairo_set_line_width(cr, 1.6);
        cairo_move_to(cr, 0, track_y - 82);
        cairo_line_to(cr, W, track_y - 82);
        cairo_stroke(cr);

        double train_x = -950.0 + prog * 2250.0;
        double ty = track_y - 68.0;
        cairo_set_source_rgba(cr, 0.04, 0.065, 0.105, 0.98);
        for (int car = 0; car < 6; ++car) {
            double cx = train_x + car * 255.0;
            cairo_rectangle(cr, cx, ty, 245.0, 66.0);
            cairo_fill(cr);
            for (int w = 0; w < 6; ++w) {
                cairo_set_source_rgba(cr, 1.0, 0.86, 0.58, 0.90);
                cairo_rectangle(cr, cx + 18 + w * 36, ty + 16, 26, 22);
                cairo_fill(cr);
            }
            cairo_set_source_rgba(cr, 0.04, 0.065, 0.105, 0.98);
        }
        double nx = train_x + 6 * 255.0;
        cairo_move_to(cr, nx, ty);
        cairo_curve_to(cr, nx + 56, ty + 8, nx + 74, ty + 48, nx + 68, ty + 66);
        cairo_line_to(cr, nx, ty + 66);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.90, 0.65, 0.92);
        cairo_arc(gcr, nx + 62, ty + 44, 10.0, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        for (int car = 0; car < 6; ++car) {
            double cx = train_x + car * 255.0;
            cairo_set_source_rgba(gcr, 1.0, 0.80, 0.44, 0.45);
            cairo_rectangle(gcr, cx + 14, ty + 16, 218, 22);
            cairo_fill(gcr);
        }
        cairo_restore(gcr);
        break;
    }
    case SceneKind::PLATFORM_WIND_WAIT: {
        // Shot 10 ("信我每日清晨前往的车站"): Hero platform silhouette leaning forward in morning wind!
        double plat_y = H * 0.78;
        cairo_set_source_rgba(cr, 0.022, 0.032, 0.048, 0.98);
        cairo_rectangle(cr, 0, plat_y, W, H - plat_y);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.96, 0.76, 0.22, 0.90);
        cairo_rectangle(cr, 0, plat_y + 22, W, 7);
        cairo_fill(cr);
        // Canopy truss edge at very top
        cairo_set_source_rgba(cr, 0.018, 0.026, 0.042, 0.96);
        cairo_move_to(cr, 0, 0);
        cairo_line_to(cr, 1080, 0);
        cairo_line_to(cr, 760, 72);
        cairo_line_to(cr, 0, 128);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Wind gusts & subtle camera push-in
        double sc = 2.05 + prog * 0.16;
        draw_traveler(cr, gcr, 560.0 + prog * 40.0, plat_y + 26.0, sc, 1.0, t * 1.4, 0.0, -0.22, 0.0, 1.25,
                      0.98, 0.76, 0.46, 0.80, 1.0);
        break;
    }
    case SceneKind::MOUNTAIN_WIND_SWEEP: {
        // Shot 11 ("好风来了助我凭脚踝翻过山"): Aerial flyover across 4 layered ridges + sweeping wind ribbons
        double cam_shift = prog * 180.0;
        double cols[4][3] = {{0.10, 0.20, 0.36}, {0.06, 0.14, 0.26}, {0.035, 0.08, 0.17}, {0.015, 0.03, 0.065}};
        for (int r = 0; r < 4; ++r) {
            double base_y = H * (0.56 + r * 0.12) + prog * (r * 18.0);
            cairo_set_source_rgba(cr, cols[r][0], cols[r][1], cols[r][2], 0.96);
            cairo_move_to(cr, 0, H);
            for (int x = 0; x <= W; x += 32) {
                double xx = x + cam_shift * (0.3 + r * 0.25);
                double mh = (115.0 - r * 12.0) * std::sin(xx * 0.0036 + r * 1.7) + 52.0 * std::cos(xx * 0.0085 - r);
                cairo_line_to(cr, x, base_y - mh);
            }
            cairo_line_to(cr, W, H);
            cairo_close_path(cr);
            cairo_fill(cr);
        }
        // Flowing golden wind ribbons & paper birds crossing ridge
        for (int w = 0; w < 5; ++w) {
            double wy = H * (0.28 + w * 0.10) + std::sin(t * 2.0 + w) * 28.0;
            cairo_set_source_rgba(cr, 1.0, 0.86, 0.56, 0.22 + 0.14 * af.bass);
            cairo_set_line_width(cr, 2.2);
            cairo_move_to(cr, -100, wy + 70);
            cairo_curve_to(cr, W * 0.35, wy - 40, W * 0.65, wy + 50, W + 100, wy - 65);
            cairo_stroke(cr);
        }
        for (int b = 0; b < 6; ++b) {
            double bu = fractf(prog * 1.1 + b * 0.17);
            double bx = bu * (W + 200) - 100;
            double by = H * 0.48 - bu * 220.0 + std::sin(bu * 7.0 + b) * 35.0;
            draw_paper_motif(cr, gcr, bx, by, 16.0, -0.32, 1, t * 8.0 + b, std::sin(bu * M_PI));
        }
        break;
    }
    case SceneKind::SKYLINE_BLUEPRINT_RISE: {
        // Shot 12 ("无名闯繁城 盖梦中楼"): Dense skyline rising from blueprints + rotating cranes!
        double base_y = H * 0.88;
        double rise = smoothstep(0.0f, 0.72f, prog);
        cairo_set_source_rgba(cr, 0.015, 0.025, 0.048, 0.98);
        cairo_rectangle(cr, 0, base_y, W, H - base_y);
        cairo_fill(cr);

        for (int i = 0; i < 28; ++i) {
            double bx = 20.0 + i * 68.0 + hash21(i, 1.1f) * 18.0;
            double bw = 38.0 + hash21(i, 2.2f) * 32.0;
            double full_h = 180.0 + hash21(i, 3.3f) * 420.0;
            double delay = hash21(i, 4.4f) * 0.40;
            double grow = smoothstep(static_cast<float>(delay), static_cast<float>(delay + 0.55), static_cast<float>(rise));
            double bh = full_h * grow;
            double top_y = base_y - bh;

            cairo_set_source_rgba(cr, 0.28, 0.66, 1.0, 0.50 * (1.0 - 0.50 * grow));
            cairo_set_line_width(cr, 1.3);
            cairo_rectangle(cr, bx, top_y, bw, bh);
            cairo_stroke(cr);

            cairo_set_source_rgba(cr, 0.020, 0.032, 0.062, 0.94 * grow);
            cairo_rectangle(cr, bx, top_y, bw, bh);
            cairo_fill(cr);

            if (grow > 0.30) {
                int cols = std::max(2, static_cast<int>(bw / 11));
                int rows = std::max(3, static_cast<int>(bh / 16));
                for (int r = 1; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        float rnd = hash21(i * 31.0f + c, r * 17.0f);
                        if (rnd > 0.50f) {
                            double band_mod = 0.70 + 0.45 * af.bands[(i + r) % 8];
                            if (rnd > 0.72f) cairo_set_source_rgba(cr, 1.0, 0.80, 0.42, 0.82 * band_mod * grow);
                            else cairo_set_source_rgba(cr, 0.46, 0.80, 1.0, 0.62 * band_mod * grow);
                            cairo_rectangle(cr, bx + 5.0 + c * ((bw - 10.0) / cols), top_y + 8.0 + r * 15.0, 5.0, 7.0);
                            cairo_fill(cr);
                        }
                    }
                }
            }
        }

        double crane_xs[3] = {460.0, 960.0, 1460.0};
        double crane_hs[3] = {540.0, 620.0, 560.0};
        for (int k = 0; k < 3; ++k) {
            double cx = crane_xs[k], cy_top = base_y - crane_hs[k] * smoothstep(0.0f, 0.65f, static_cast<float>(rise));
            double jib = 155.0 * std::cos(t * 0.32 + k * 1.4);
            cairo_set_source_rgba(cr, 0.10, 0.18, 0.30, 0.92);
            cairo_set_line_width(cr, 4.5);
            cairo_move_to(cr, cx, base_y);
            cairo_line_to(cr, cx, cy_top);
            cairo_move_to(cr, cx - jib * 0.35, cy_top + 10);
            cairo_line_to(cr, cx + jib, cy_top + 10);
            cairo_stroke(cr);

            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_set_source_rgba(gcr, 1.0, 0.36, 0.18, 0.5 + 0.5 * std::sin(t * 4.0 + k));
            cairo_arc(gcr, cx, cy_top, 5.5, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }
        break;
    }
    case SceneKind::ROOFTOP_CATCH_STAR: {
        // Shot 13 ("徒手摘星辰 从无到有"): Upward rooftop silhouette catching descending golden star!
        cairo_set_source_rgba(cr, 0.014, 0.022, 0.040, 0.98);
        cairo_rectangle(cr, 0, H * 0.84, W, H * 0.16);
        cairo_fill(cr);

        for (int b = 0; b < 8; ++b) {
            double bu = fractf(t * 0.18 + b * 0.13);
            double bx = 180.0 + bu * 1550.0;
            double by = H * 0.74 - bu * 560.0 + std::sin(bu * 6.0 + b) * 32.0;
            draw_paper_motif(cr, gcr, bx, by, 15.0, -0.35, 1, t * 7.8 + b, std::sin(bu * M_PI) * 0.90);
        }

        double reach = smoothstep(0.05f, 0.68f, prog);
        double tx = 1320.0 - prog * 30.0, ty = H * 0.84;
        draw_traveler(cr, gcr, tx, ty, 2.15, -1.0, 0.0, 0.0, -0.60, reach, 0.85, 0.99, 0.84, 0.52, 0.88, 1.0);

        double sx = tx - 82.0;
        double sy = ty - 344.0 + (1.0 - reach) * -220.0;
        double pulse = 1.0 + 0.30 * af.air + 0.15 * std::sin(t * 5.0);
        cairo_set_source_rgba(cr, 1.0, 0.96, 0.80, 0.98);
        cairo_arc(cr, sx, sy, 8.0 * pulse, 0, 2.0 * M_PI);
        cairo_fill(cr);
        cairo_set_line_width(cr, 2.5);
        cairo_move_to(cr, sx - 44 * pulse, sy);
        cairo_line_to(cr, sx + 44 * pulse, sy);
        cairo_move_to(cr, sx, sy - 34 * pulse);
        cairo_line_to(cr, sx, sy + 34 * pulse);
        cairo_stroke(cr);

        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.86, 0.48, 0.95);
        cairo_arc(gcr, sx, sy, 18.0 * pulse, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);
        break;
    }
    case SceneKind::INTERLUDE_APPROACH:
    case SceneKind::INTERLUDE_HAND_LIGHT: {
        bool close_pass = (kind == SceneKind::INTERLUDE_HAND_LIGHT);
        double hy = H * (close_pass ? 0.52 : 0.58);
        double gap = close_pass ? (260.0 - prog * 90.0) : (480.0 - prog * 210.0);
        double lx = 960.0 - gap, rx = 960.0 + gap;
        double sc_c = close_pass ? 1.55 : 1.15;
        double sc_a = close_pass ? 2.00 : 1.50;

        draw_traveler(cr, gcr, lx, hy, sc_c, 1.0, close_pass ? 0.0 : prog * 8.0, close_pass ? 0.0 : 0.55,
                      -0.08, 0.55, 0.12, 1.0, 0.78, 0.45, 0.70, 0.96);
        draw_traveler(cr, gcr, rx, hy, sc_a, -1.0, 0.0, 0.0, 0.12, close_pass ? 0.48 : 0.20, 0.12,
                      1.0, 0.78, 0.45, 0.70, 0.96);

        cairo_save(cr);
        cairo_translate(cr, 0, hy * 2.0 + 4.0);
        cairo_scale(cr, 1.0, -0.60);
        draw_traveler(cr, nullptr, lx, hy, sc_c, 1.0, 0, 0, -0.08, 0.55, 0.12, 1.0, 0.78, 0.45, 0.2, 0.26);
        draw_traveler(cr, nullptr, rx, hy, sc_a, -1.0, 0, 0, 0.12, close_pass ? 0.48 : 0.20, 0.12, 1.0, 0.78, 0.45, 0.2, 0.26);
        cairo_restore(cr);

        double ox = close_pass ? (lx + 110.0 + prog * (gap * 1.4 - 110.0)) : (lx + 70.0);
        double oy = hy - (close_pass ? 155.0 : 110.0) - std::sin(prog * M_PI) * 32.0;
        cairo_set_source_rgba(cr, 1.0, 0.92, 0.70, 0.96);
        cairo_arc(cr, ox, oy, 8.5 + 2.5 * af.mid, 0, 2.0 * M_PI);
        cairo_fill(cr);
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.78, 0.36, 0.94);
        cairo_arc(gcr, ox, oy, 20.0, 0, 2.0 * M_PI);
        cairo_set_source_rgba(gcr, 1.0, 0.70, 0.32, 0.42);
        cairo_arc(gcr, ox, hy + (hy - oy) * 0.62, 16.0, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);
        break;
    }
    case SceneKind::HARBOR_FIREFLIES_WALK: {
        // Shot 16 ("我信我的渴望 它是从前的我给我的光"): Harbor promenade walk + glowing firefly motes
        double hy = H * 0.56;
        draw_cable_bridge(cr, gcr, hy - 8.0, t, 0.45);
        double rail_y = H * 0.78;
        cairo_set_source_rgba(cr, 0.018, 0.025, 0.040, 0.98);
        cairo_rectangle(cr, 0, rail_y + 40, W, H - rail_y);
        cairo_fill(cr);
        cairo_set_line_width(cr, 5.0);
        cairo_move_to(cr, 0, rail_y);
        cairo_line_to(cr, W, rail_y);
        cairo_stroke(cr);

        draw_traveler(cr, gcr, 540.0 + prog * 340.0, rail_y + 115.0, 1.75, 1.0, prog * 14.0, 0.85, -0.10, 0.0, 0.30,
                      1.0, 0.80, 0.44, 0.76, 1.0);
        for (int e = 0; e < 32; ++e) {
            double eu = fractf(hash21(e, 6.2f) + t * (0.06 + 0.05 * hash21(e, 1.9f)));
            double ex = hash21(e, 8.8f) * W + std::sin(t * 1.8 + e) * 32.0;
            double ey = H * (0.88 - eu * 0.72);
            double al = std::sin(eu * M_PI) * 0.92;
            cairo_set_source_rgba(cr, 1.0, 0.86, 0.48, al);
            cairo_arc(cr, ex, ey, 3.0 + 2.0 * hash21(e, 3.1f), 0, 2.0 * M_PI);
            cairo_fill(cr);
        }
        break;
    }
    case SceneKind::PAPER_BIRD_BAY_GLIDE: {
        // Shot 17 ("信它说 得你想要的"): Close flight tracking crisp white origami bird over bay glow!
        double hy = H * 0.45;
        draw_cable_bridge(cr, gcr, hy - 10.0, t, 0.60);
        double bx = 420.0 + prog * 1020.0;
        double by = H * 0.56 + std::sin(t * 2.8) * 38.0;
        draw_paper_motif(cr, gcr, bx, by, 76.0, -0.08 + std::cos(t * 2.8) * 0.08, 1, t * 8.5, 0.98, 1.0, 0.95, 0.84);
        // Mirror skim reflection in harbor water below
        cairo_save(cr);
        cairo_translate(cr, bx, H * 0.78);
        cairo_scale(cr, 1.0, -0.45);
        draw_paper_motif(cr, nullptr, 0, 0, 68.0, 0.08, 1, t * 8.5, 0.25, 1.0, 0.84, 0.58);
        cairo_restore(cr);
        break;
    }
    case SceneKind::BOARDWALK_LANTERNS: {
        // Shot 18 ("我信城中诸位 虽乡音和过往不尽相同"): Coastal boardwalk, festoon lanterns, varied city travelers!
        double hy = H * 0.56;
        draw_cable_bridge(cr, gcr, hy - 10.0, t, 0.55);
        double rail_y = H * 0.76;
        cairo_set_source_rgba(cr, 0.018, 0.022, 0.036, 0.98);
        cairo_rectangle(cr, 0, rail_y + 44, W, H - rail_y);
        cairo_fill(cr);
        cairo_set_line_width(cr, 5.5);
        cairo_move_to(cr, 0, rail_y);
        cairo_line_to(cr, W, rail_y);
        cairo_stroke(cr);
        for (int bx = 60; bx < W; bx += 110) cairo_rectangle(cr, bx, rail_y, 6, 46);
        cairo_fill(cr);

        cairo_set_source_rgba(cr, 0.03, 0.03, 0.05, 0.90);
        cairo_set_line_width(cr, 2.5);
        cairo_move_to(cr, 0, H * 0.10);
        cairo_curve_to(cr, 640, H * 0.26, 1280, H * 0.26, W, H * 0.10);
        cairo_stroke(cr);
        for (int b = 1; b <= 11; ++b) {
            double u = b / 12.0;
            double bx = u * W, sway = std::sin(t * 1.4 + b * 0.7) * 6.0;
            double by = H * (0.10 + 4.0 * u * (1.0 - u) * 0.12);
            cairo_set_source_rgba(cr, 1.0, 0.88, 0.62, 0.94);
            cairo_arc(cr, bx + sway, by + 10.0, 7.0, 0, 2.0 * M_PI);
            cairo_fill(cr);
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_set_source_rgba(gcr, 1.0, 0.70, 0.32, 0.90);
            cairo_arc(gcr, bx + sway, by + 10.0, 14.0, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }

        double pan = -prog * 50.0;
        struct G { double x, sc, fac; };
        G gs[] = {{380, 1.48, 1.0}, {630, 1.62, 1.0}, {930, 1.54, -1.0}, {1220, 1.64, -1.0}, {1520, 1.46, -1.0}};
        for (auto& g : gs) {
            draw_traveler(cr, gcr, g.x + pan, rail_y + 115.0, g.sc, g.fac, t * 0.8, 0.0, -0.10, 0.12, 0.22,
                          1.0, 0.78, 0.44, 0.65, 1.0);
        }
        break;
    }
    case SceneKind::GLASSES_TOAST_CLOSE: {
        // Shot 19 ("也举起杯 祝你乘风破浪"): Curved sleeves, hands, and delicate clinking crystal glasses!
        double clink = smoothstep(0.08f, 0.56f, prog);
        double zoom = 1.0 + prog * 0.07;
        cairo_save(cr);
        cairo_translate(cr, W * 0.5, H * 0.52);
        cairo_scale(cr, zoom, zoom);

        struct ToastArm { double sx, sy, ex, ey, gx, gy, tilt; };
        ToastArm arms[3] = {
            {-760, 480, -340, 210, -140 + clink * 66, -18 - clink * 28,  0.22 - clink * 0.06},
            { 760, 470,  345, 200,  144 - clink * 66, -16 - clink * 28, -0.22 + clink * 0.06},
            { 190, 560,  110, 270,    8,               18 - clink * 38,  0.03}
        };

        for (int i = 0; i < 3; ++i) {
            const auto& a = arms[i];
            // Tapered natural forearm silhouette
            cairo_set_source_rgba(cr, 0.025, 0.028, 0.044, 0.97);
            cairo_move_to(cr, a.sx - 44, a.sy);
            cairo_curve_to(cr, a.ex - 26, a.ey, a.gx - 12, a.gy + 95, a.gx - 8, a.gy + 72);
            cairo_line_to(cr, a.gx + 10, a.gy + 72);
            cairo_curve_to(cr, a.ex + 22, a.ey + 18, a.sx + 44, a.sy, a.sx + 44, a.sy);
            cairo_close_path(cr);
            cairo_fill(cr);

            cairo_save(cr);
            cairo_translate(cr, a.gx, a.gy);
            cairo_rotate(cr, a.tilt);
            // Stem & base gripped delicately by fingers
            cairo_set_source_rgba(cr, 0.035, 0.038, 0.056, 0.98);
            cairo_rectangle(cr, -3.0, 26, 6.0, 62);
            cairo_move_to(cr, -28, 88);
            cairo_curve_to(cr, -14, 83, 14, 83, 28, 88);
            cairo_line_to(cr, 28, 92);
            cairo_line_to(cr, -28, 92);
            cairo_close_path(cr);
            cairo_fill(cr);

            // Crystal glass bowl with warm amber wine glow
            cairo_move_to(cr, -36, -62);
            cairo_curve_to(cr, -34, 14, -12, 30, 0, 30);
            cairo_curve_to(cr, 12, 30, 34, 14, 36, -62);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 1.0, 0.80, 0.40, 0.34);
            cairo_fill(cr);

            // Golden amber wine fill inside lower half of bowl
            cairo_move_to(cr, -32, -16);
            cairo_curve_to(cr, -28, 16, -10, 28, 0, 28);
            cairo_curve_to(cr, 10, 28, 28, 16, 32, -16);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 0.96, 0.66, 0.24, 0.78);
            cairo_fill(cr);

            // Crisp specular glass rim & vertical highlight
            cairo_set_source_rgba(cr, 1.0, 0.96, 0.84, 0.88);
            cairo_set_line_width(cr, 2.2);
            cairo_move_to(cr, -36, -62);
            cairo_curve_to(cr, -34, 14, -12, 30, 0, 30);
            cairo_curve_to(cr, 12, 30, 34, 14, 36, -62);
            cairo_line_to(cr, -36, -62);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
        if (clink > 0.45) {
            double sp = (clink - 0.45) / 0.55;
            cairo_set_source_rgba(cr, 1.0, 0.95, 0.78, sp * 0.95);
            cairo_arc(cr, 0, -78, 10.0 * sp, 0, 2.0 * M_PI);
            cairo_fill(cr);
            if (gcr) {
                cairo_save(gcr);
                cairo_scale(gcr, 0.5, 0.5);
                cairo_translate(gcr, W * 0.5, H * 0.52);
                cairo_scale(gcr, zoom, zoom);
                cairo_set_source_rgba(gcr, 1.0, 0.84, 0.46, sp * 0.92);
                cairo_arc(gcr, 0, -78, 22.0 * sp, 0, 2.0 * M_PI);
                cairo_fill(gcr);
                cairo_restore(gcr);
            }
        }
        cairo_restore(cr);

        for (int e = 0; e < 26; ++e) {
            double eu = fractf(hash21(e, 2.2f) + t * 0.12);
            double ex = 300.0 + hash21(e, 7.7f) * 1320.0;
            double ey = H * (0.90 - eu * 0.80);
            cairo_set_source_rgba(cr, 1.0, 0.88, 0.52, std::sin(eu * M_PI) * 0.88);
            cairo_arc(cr, ex, ey, 3.0 + 2.0 * hash21(e, 4.1f), 0, 2.0 * M_PI);
            cairo_fill(cr);
        }
        break;
    }
    case SceneKind::BAY_BRIDGE_PANORAMA: {
        double hy = H * 0.56 - prog * 28.0;
        draw_cable_bridge(cr, gcr, hy, t, 0.75);
        for (int e = 0; e < 38; ++e) {
            double eu = fractf(hash21(e, 6.2f) + t * (0.05 + 0.05 * hash21(e, 1.9f)));
            double ex = hash21(e, 8.8f) * W + std::sin(t * 1.5 + e) * 24.0;
            double ey = H * (0.94 - eu * 0.86);
            cairo_set_source_rgba(cr, 1.0, 0.85, 0.45, std::sin(eu * M_PI) * 0.90);
            cairo_arc(cr, ex, ey, 2.5 + 2.2 * hash21(e, 3.1f), 0, 2.0 * M_PI);
            cairo_fill(cr);
        }
        break;
    }
    case SceneKind::OBSERVATION_BLUEPRINT: {
        double hy = H * 0.62;
        for (int i = 0; i < 20; ++i) {
            double bx = 30.0 + i * 96.0, bw = 42.0 + hash21(i, 4.1f) * 28.0, bh = 95.0 + hash21(i, 9.1f) * 190.0;
            cairo_set_source_rgba(cr, 0.02, 0.035, 0.072, 0.94);
            cairo_rectangle(cr, bx, hy - bh + 20, bw, bh);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, 1.0, 0.82, 0.46, 0.65);
            for (int wy = 14; wy < bh - 12; wy += 16) cairo_rectangle(cr, bx + 8, hy - bh + 20 + wy, 6, 7);
            cairo_fill(cr);
        }
        cairo_set_source_rgba(cr, 0.42, 0.82, 1.0, 0.42);
        cairo_set_line_width(cr, 1.8);
        cairo_move_to(cr, 240, hy - 140);
        cairo_curve_to(cr, 680, 180, 1240, 180, 1680, hy - 160);
        cairo_stroke(cr);

        cairo_set_source_rgba(cr, 0.015, 0.022, 0.038, 0.98);
        cairo_rectangle(cr, 0, H * 0.82, W, H * 0.18);
        cairo_fill(cr);
        draw_traveler(cr, gcr, 620.0 + prog * 30.0, H * 0.82, 1.80, 1.0, 0.0, 0.0, -0.28, 0.0, 0.55,
                      0.56, 0.84, 1.0, 0.78, 1.0);
        break;
    }
    case SceneKind::BRIDGE_DECK_SPEED: {
        // Shot 22 ("时代轰隆隆地来不转弯"): Realistic perspective sea-bridge with A-frame pylon, guardrails & dashed lanes!
        double vx = W * 0.5, vy = H * 0.55;

        // Horizontal sea-bridge continuation toward horizon so deck never looks like a floating wedge!
        cairo_set_source_rgba(cr, 0.024, 0.036, 0.062, 0.92);
        cairo_rectangle(cr, 0, vy, W, 14.0);
        cairo_fill(cr);

        // Perspective roadway deck
        cairo_set_source_rgba(cr, 0.018, 0.026, 0.044, 0.98);
        cairo_move_to(cr, 0, H);
        cairo_line_to(cr, vx - 38, vy);
        cairo_line_to(cr, vx + 38, vy);
        cairo_line_to(cr, W, H);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Tall A-frame bridge pylon anchoring the horizon vanish point
        cairo_set_source_rgba(cr, 0.032, 0.048, 0.082, 0.98);
        cairo_set_line_width(cr, 18.0);
        cairo_move_to(cr, vx - 135, vy + 85);
        cairo_line_to(cr, vx - 8, vy - 285);
        cairo_line_to(cr, vx + 8, vy - 285);
        cairo_line_to(cr, vx + 135, vy + 85);
        cairo_stroke(cr);
        cairo_set_line_width(cr, 12.0);
        cairo_move_to(cr, vx - 68, vy - 110);
        cairo_line_to(cr, vx + 68, vy - 110);
        cairo_stroke(cr);

        // Perspective steel deck guardrails
        cairo_set_source_rgba(cr, 0.42, 0.58, 0.78, 0.45);
        cairo_set_line_width(cr, 3.0);
        cairo_move_to(cr, 0, H - 65);
        cairo_line_to(cr, vx - 38, vy);
        cairo_move_to(cr, W, H - 65);
        cairo_line_to(cr, vx + 38, vy);
        cairo_stroke(cr);

        // Racing center dashed lane markings
        cairo_set_source_rgba(cr, 1.0, 0.86, 0.56, 0.60);
        for (int d = 0; d < 10; ++d) {
            double z0 = fractf(d / 10.0 + t * 0.75);
            double z1 = std::min(1.0, z0 + 0.045);
            double y0 = vy + std::pow(z0, 1.8) * (H - vy);
            double y1 = vy + std::pow(z1, 1.8) * (H - vy);
            double w0 = 1.5 + z0 * 6.0;
            cairo_set_line_width(cr, w0);
            cairo_move_to(cr, vx, y0);
            cairo_line_to(cr, vx, y1);
            cairo_stroke(cr);
        }

        // Sweeping harp stay cables overhead
        for (int c = 0; c < 14; ++c) {
            double z = fractf(c / 14.0 + t * 0.35);
            double x_out = 100.0 + (1.0 - z) * 860.0;
            double y_bot = vy + (1.0 - z) * 440.0;
            double alpha = smoothstep(0.05f, 0.25f, static_cast<float>(z)) * (1.0 - smoothstep(0.85f, 1.0f, static_cast<float>(z)));
            cairo_set_source_rgba(cr, 0.65, 0.82, 0.99, 0.44 * alpha);
            cairo_set_line_width(cr, 1.5 + 2.0 * (1.0 - z));
            cairo_move_to(cr, vx, vy - 280);
            cairo_line_to(cr, vx - x_out, y_bot);
            cairo_move_to(cr, vx, vy - 280);
            cairo_line_to(cr, vx + x_out, y_bot);
            cairo_stroke(cr);
        }

        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        for (int s = 0; s < 6; ++s) {
            double z = fractf(s / 6.0 + t * 0.9);
            double y = vy + std::pow(z, 1.6) * (H - vy);
            double x = vx + (s % 2 == 0 ? -1.0 : 1.0) * std::pow(z, 1.4) * 260.0;
            cairo_set_source_rgba(gcr, 1.0, 0.82, 0.44, 0.82);
            cairo_arc(gcr, x, y, 3.0 + 9.0 * z, 0, 2.0 * M_PI);
            cairo_fill(gcr);
        }
        cairo_restore(gcr);
        break;
    }
    case SceneKind::SEA_RAIL_DAWN_STATION: {
        double track_y = H * 0.58;
        cairo_set_source_rgba(cr, 0.025, 0.042, 0.072, 0.96);
        cairo_rectangle(cr, 0, track_y, W, 24);
        for (int px = 100; px < W; px += 320) cairo_rectangle(cr, px, track_y + 20, 26, H - track_y);
        cairo_fill(cr);

        double train_x = -800.0 + prog * 2100.0;
        double ty = track_y - 54.0;
        cairo_set_source_rgba(cr, 0.04, 0.065, 0.11, 0.98);
        for (int car = 0; car < 5; ++car) {
            double cx = train_x + car * 235.0;
            cairo_rectangle(cr, cx, ty, 224.0, 52.0);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, 1.0, 0.86, 0.58, 0.88);
            for (int w = 0; w < 6; ++w) cairo_rectangle(cr, cx + 16 + w * 33, ty + 14, 22, 18);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, 0.04, 0.065, 0.11, 0.98);
        }

        double plat_y = H * 0.80;
        cairo_set_source_rgba(cr, 0.02, 0.03, 0.05, 0.98);
        cairo_rectangle(cr, 0, plat_y, W, H - plat_y);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.96, 0.78, 0.24, 0.90);
        cairo_rectangle(cr, 0, plat_y + 18, W, 6);
        cairo_fill(cr);
        draw_traveler(cr, gcr, 520.0, plat_y + 22.0, 1.85, 1.0, t * 1.4, 0.0, -0.18, 0.0, 0.95,
                      0.96, 0.78, 0.48, 0.78, 1.0);
        break;
    }
    case SceneKind::CAPE_LIGHTHOUSE_GALE: {
        // Shot 24 ("好风来了助我凭脚踝翻过山"): Grounded lighthouse + analytic cape ridge + rooted grass & silhouette!
        // Left lighthouse headland (drawn cleanly beneath lighthouse)
        cairo_set_source_rgba(cr, 0.020, 0.034, 0.062, 0.98);
        cairo_move_to(cr, 402, H * 0.59);
        cairo_line_to(cr, 415, H * 0.36);
        cairo_line_to(cr, 433, H * 0.36);
        cairo_line_to(cr, 446, H * 0.59);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_move_to(cr, 0, H);
        cairo_line_to(cr, 0, H * 0.56);
        cairo_curve_to(cr, 280, H * 0.52, 470, H * 0.58, 620, H * 0.64);
        cairo_line_to(cr, 680, H);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.88, 0.56, 0.94);
        cairo_arc(gcr, 424, H * 0.36, 12.0, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);

        // Analytic foreground right cape hill so grass & hero silhouette stand firmly grounded!
        auto cape_y = [&](double x) -> double {
            double u = std::clamp((x - 1100.0) / (W - 1100.0), 0.0, 1.0);
            return H - 360.0 * std::sin(u * M_PI * 0.62) - 40.0 * u;
        };
        cairo_set_source_rgba(cr, 0.015, 0.024, 0.044, 0.98);
        cairo_move_to(cr, 1100, H);
        for (int x = 1100; x <= W; x += 20) cairo_line_to(cr, x, cape_y(x));
        cairo_line_to(cr, W, H);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_set_line_width(cr, 2.6);
        for (int g = 0; g < 26; ++g) {
            double gx = 1160.0 + g * 28.0;
            double gy = cape_y(gx);
            double bend = 26.0 + 14.0 * std::sin(t * 6.0 + g);
            cairo_move_to(cr, gx, gy + 12);
            cairo_curve_to(cr, gx + bend * 0.4, gy - 14, gx + bend * 0.8, gy - 32, gx + bend, gy - 46);
            cairo_stroke(cr);
        }

        double hero_x = 1490.0;
        draw_traveler(cr, gcr, hero_x, cape_y(hero_x) + 12.0, 1.85, -1.0, 0.0, 0.0, -0.25, 0.15, 1.35,
                      0.98, 0.82, 0.50, 0.85, 1.0);
        break;
    }
    case SceneKind::SAILBOAT_GALE_ACTION: {
        // Shot 25 ("好风吹它一个挥着旗扬我帆 / 跨海行船不胜不还"): Hero white-sailed sloop slicing sunlit waves!
        double bx = 660.0 + prog * 460.0;
        double by = H * 0.65 + std::sin(t * 2.4) * 14.0;
        double heel = -0.06 + 0.04 * std::sin(t * 2.1);
        double sc = 1.85;

        cairo_save(cr);
        cairo_translate(cr, bx, by);
        cairo_rotate(cr, heel);
        cairo_scale(cr, sc, sc);

        cairo_move_to(cr, -95, 0);
        cairo_line_to(cr, 108, -15);
        cairo_curve_to(cr, 96, 18, 60, 30, -76, 25);
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, 0.04, 0.08, 0.15, 0.96);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.94, 0.97, 1.0, 0.72);
        cairo_set_line_width(cr, 2.4);
        cairo_move_to(cr, -95, 0);
        cairo_line_to(cr, 108, -15);
        cairo_stroke(cr);

        cairo_set_source_rgba(cr, 0.86, 0.92, 0.98, 0.88);
        cairo_set_line_width(cr, 3.2);
        cairo_move_to(cr, 0, 0);
        cairo_line_to(cr, 4, -188);
        cairo_stroke(cr);

        double billow = 20.0 + 10.0 * af.bass;
        cairo_move_to(cr, 3, -178);
        cairo_curve_to(cr, -48 - billow, -112, -74 - billow, -42, -80, -14);
        cairo_line_to(cr, 0, -10);
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, 0.98, 0.98, 1.0, 0.95);
        cairo_fill(cr);

        cairo_move_to(cr, 6, -164);
        cairo_curve_to(cr, 50 + billow * 0.6, -98, 80 + billow * 0.5, -42, 95, -16);
        cairo_line_to(cr, 6, -12);
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, 0.85, 0.92, 0.99, 0.90);
        cairo_fill(cr);

        // Whipping crimson pennant
        cairo_move_to(cr, 4, -188);
        for (int seg = 1; seg <= 6; ++seg) cairo_line_to(cr, 4 - seg * 15, -188 + std::sin(t * 12.0 - seg * 1.1) * (3 + seg * 1.8));
        for (int seg = 6; seg >= 1; --seg) cairo_line_to(cr, 4 - seg * 15, -178 + std::sin(t * 12.0 - seg * 1.1) * (3 + seg * 1.8));
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, 0.98, 0.40, 0.22, 0.96);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_set_source_rgba(cr, 0.88, 0.95, 1.0, 0.58);
        cairo_set_line_width(cr, 3.2);
        for (int w = 0; w < 3; ++w) {
            double wy = by + 26.0 + w * 16.0;
            cairo_move_to(cr, bx + 140 - w * 45, wy - 5);
            cairo_curve_to(cr, bx - 50, wy + 10, bx - 300, wy + 4, bx - 560 - w * 70, wy + 22);
            cairo_stroke(cr);
        }
        break;
    }
    case SceneKind::SUNSET_EXIT_PANORAMA: {
        // Shot 26 ("信那一年 我出站后 第一眼燃烧着的天空"): Full-bleed panoramic blazing sky past thin sliding edge pillars!
        double hy = H * 0.74;
        cairo_set_source_rgba(cr, 0.055, 0.025, 0.045, 0.92);
        for (int i = 0; i < 26; ++i) {
            double bx = i * 76.0 + hash21(i, 2.7f) * 14.0;
            double bw = 36.0 + hash21(i, 8.1f) * 40.0;
            double bh = 70.0 + hash21(i, 5.5f) * 200.0;
            cairo_rectangle(cr, bx, hy - bh, bw, bh + 18.0);
            if (i % 4 == 1) cairo_rectangle(cr, bx + bw * 0.22, hy - bh - 28.0, bw * 0.56, 30.0);
            if (i % 6 == 2) cairo_rectangle(cr, bx + bw * 0.47, hy - bh - 68.0, 4.0, 70.0);
        }
        cairo_fill(cr);

        // Wet dark ground plaza
        cairo_set_source_rgba(cr, 0.015, 0.014, 0.022, 0.86);
        cairo_rectangle(cr, 0, hy, W, H - hy);
        cairo_fill(cr);

        // Thin sliding foreground exit pillars on FAR edges (zero giant black border!)
        double slide = prog * 70.0;
        cairo_set_source_rgba(cr, 0.015, 0.012, 0.020, 0.95);
        cairo_rectangle(cr, 0, 0, std::max(10.0, 82.0 - slide), H);
        cairo_rectangle(cr, W - std::max(10.0, 82.0 - slide), 0, std::max(10.0, 82.0 - slide), H);
        cairo_fill(cr);

        draw_traveler(cr, gcr, 620.0 + prog * 95.0, hy + 28.0, 1.95, 1.0, t * 2.2, 0.42, -0.45, 0.0, 0.72,
                      1.0, 0.58, 0.24, 0.94, 1.0);
        break;
    }
    case SceneKind::SUNSET_CROSSWALK_PACK: {
        // Shot 27 ("信旧背包 信绿灯走"): Medium-close dusk crosswalk silhouette with worn backpack & crisp emerald green signal!
        double hor = H * 0.62;
        cairo_set_source_rgba(cr, 0.02, 0.018, 0.030, 0.96);
        for (int s = 0; s < 7; ++s) {
            double x0 = 260 + s * 220;
            cairo_move_to(cr, x0, hor + 40);
            cairo_line_to(cr, x0 + 95, hor + 40);
            cairo_line_to(cr, x0 + 135, H);
            cairo_line_to(cr, x0 + 10, H);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 0.92, 0.56, 0.34, 0.20);
            cairo_fill(cr);
        }
        double sx = 1520, sy = hor - 270;
        cairo_set_source_rgba(cr, 0.02, 0.018, 0.028, 0.98);
        cairo_rectangle(cr, sx - 7, sy, 14, hor - sy + 90);
        cairo_rectangle(cr, sx - 28, sy - 58, 56, 102);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.16, 0.98, 0.60, 0.98);
        cairo_arc(cr, sx, sy + 14, 14, 0, 2.0 * M_PI);
        cairo_fill(cr);
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 0.16, 0.98, 0.60, 0.95);
        cairo_arc(gcr, sx, sy + 14, 22, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);

        draw_traveler(cr, gcr, 680.0 + prog * 180.0, hor + 260.0, 2.30, 1.0, prog * 10.0, 0.85, -0.15, 0.0, 0.65,
                      0.18, 0.98, 0.62, 0.78, 1.0);
        break;
    }
    case SceneKind::MACRO_EYE_REFLECTION: {
        // Shot 28 ("信倒映前方 的眼眸"): Sculpted almond upper lash-line + naturally rooted lashes
        double cx = W * 0.5, cy = H * 0.5;
        cairo_set_source_rgba(cr, 0.015, 0.010, 0.016, 0.92);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        double p0x = cx - 540.0, p0y = cy + 36.0;
        double p1x = cx,         p1y = cy - 355.0;
        double p2x = cx + 540.0, p2y = cy + 36.0;
        cairo_set_line_width(cr, 4.8);
        cairo_move_to(cr, p0x, p0y);
        cairo_curve_to(cr, (p0x + 2.0 * p1x) / 3.0, (p0y + 2.0 * p1y) / 3.0,
                       (2.0 * p1x + p2x) / 3.0, (2.0 * p1y + p2y) / 3.0, p2x, p2y);
        cairo_stroke(cr);

        cairo_set_source_rgba(cr, 0.015, 0.010, 0.016, 0.36);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, cx - 470, cy - 18);
        cairo_curve_to(cr, cx - 220, cy - 245, cx + 220, cy - 245, cx + 470, cy - 18);
        cairo_stroke(cr);

        cairo_set_source_rgba(cr, 0.014, 0.009, 0.015, 0.88);
        for (int l = 0; l < 32; ++l) {
            double bz = 0.08 + 0.84 * (l / 31.0) + (hash21(l, 4.1f) - 0.5) * 0.018;
            double omt = 1.0 - bz;
            double lx = omt * omt * p0x + 2.0 * omt * bz * p1x + bz * bz * p2x;
            double ly = omt * omt * p0y + 2.0 * omt * bz * p1y + bz * bz * p2y + 1.5;
            double u = (bz - 0.5) * 2.0;
            double sweep = u * 0.64 + (hash21(l, 7.3f) - 0.5) * 0.14;
            double len = (34.0 + 24.0 * hash21(l, 2.9f)) * (1.0 - 0.35 * std::abs(u));
            double ex = lx + std::sin(sweep) * len, ey = ly - std::cos(sweep) * len;
            double mx = lx + std::sin(sweep * 0.52) * len * 0.55, my = ly - std::cos(sweep * 0.52) * len * 0.55;
            cairo_move_to(cr, lx - 2.0, ly);
            cairo_curve_to(cr, mx - 1.1, my, ex, ey, ex, ey);
            cairo_curve_to(cr, mx + 1.1, my, lx + 2.0, ly, lx + 2.0, ly);
            cairo_close_path(cr);
            cairo_fill(cr);
        }
        break;
    }
    case SceneKind::MOUNTAIN_BEACONS: {
        // Shot 29 ("你我在各自的山迎着战"): 5 layered misty ridges + glowing summit beacons
        double hy = H * 0.56;
        double cols[4][3] = {{0.08, 0.16, 0.30}, {0.05, 0.11, 0.22}, {0.028, 0.06, 0.13}, {0.012, 0.025, 0.055}};
        double rys[4] = {hy - 30, hy + 30, hy + 95, hy + 170};
        for (int r = 0; r < 4; ++r) {
            cairo_set_source_rgba(cr, cols[r][0], cols[r][1], cols[r][2], 0.98);
            cairo_move_to(cr, 0, H);
            for (int x = 0; x <= W; x += 32) {
                double mh = (100.0 - r * 12.0) * std::sin(x * 0.0036 + r * 1.9) + 46.0 * std::cos(x * 0.0085 - r);
                cairo_line_to(cr, x, rys[r] - mh);
            }
            cairo_line_to(cr, W, H);
            cairo_close_path(cr);
            cairo_fill(cr);
        }
        double pxs[4] = {310.0, 740.0, 1210.0, 1610.0};
        for (int p = 0; p < 4; ++p) {
            double px = pxs[p];
            double py = rys[1] - (88.0 * std::sin(px * 0.0036 + 1.9) + 46.0 * std::cos(px * 0.0085 - 1.0));
            draw_traveler(cr, gcr, px, py + 4, 0.72, (p < 2 ? 1.0 : -1.0), 0.0, 0.0, -0.25, 0.80, 0.95, 0.62, 0.88, 1.0, 0.88, 1.0);
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_set_source_rgba(gcr, 1.0, 0.84, 0.46, 0.92);
            cairo_arc(gcr, px, py - 102.0 * 0.72, 9.0, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }
        break;
    }
    case SceneKind::PASS_REVEALS_SEA: {
        // Shot 30 ("那片海有多蓝 定要去看看"): Dark foreground mountain shoulders sliding open to reveal boundless blue sea!
        double open = prog * 180.0;
        cairo_set_source_rgba(cr, 0.014, 0.026, 0.056, 0.96);
        cairo_move_to(cr, 0, H);
        cairo_line_to(cr, 0, H * 0.34);
        cairo_curve_to(cr, 320 - open * 0.4, H * 0.46, 600 - open, H * 0.76, 860 - open, H);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_move_to(cr, W, H);
        cairo_line_to(cr, W, H * 0.32);
        cairo_curve_to(cr, W - 320 + open * 0.4, H * 0.46, W - 600 + open, H * 0.76, W - 860 + open, H);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Distant glistening white sail out on the open sea horizon
        draw_paper_motif(cr, gcr, 960.0 + prog * 40.0, H * 0.58, 28.0 + prog * 10.0, 0.0, 2, 0.0, 0.92);
        break;
    }
    case SceneKind::COSMIC_SKYLINE_SOAR: {
        // Shot 31 ("看我是怎样的世代 / 所追的梦指向那星河随我来"): Soaring over harbor bridge & skyline into Milky Way!
        double hy = H * 0.62;
        draw_cable_bridge(cr, gcr, hy + 12.0, t, 0.90);
        for (int i = 0; i < 24; ++i) {
            double bx = (i < 12) ? (18.0 + i * 44.0) : (1360.0 + (i - 12) * 46.0);
            double bw = 32.0 + hash21(i, 4.4f) * 24.0;
            double bh = 85.0 + hash21(i, 9.1f) * 220.0;
            double top_y = hy - bh + 20.0;
            cairo_set_source_rgba(cr, 0.018, 0.032, 0.072, 0.94);
            cairo_rectangle(cr, bx, top_y, bw, bh);
            if (i % 3 == 0) cairo_rectangle(cr, bx + bw * 0.25, top_y - 26.0, bw * 0.5, 28.0);
            if (i % 5 == 1) cairo_rectangle(cr, bx + bw * 0.47, top_y - 45.0, 3.5, 47.0);
            cairo_fill(cr);
            int cols = std::max(2, static_cast<int>(bw / 11));
            for (int wy = 14; wy < bh - 12; wy += 14) {
                for (int c = 0; c < cols; ++c) {
                    if (hash21(i * 19.0f + c, wy) > 0.46f) {
                        cairo_set_source_rgba(cr, 0.98, 0.82, 0.46, 0.68 * (0.65 + 0.45 * af.bands[(i + c) % 8]));
                        cairo_rectangle(cr, bx + 6.0 + c * ((bw - 10.0) / cols), top_y + wy, 5.0, 6.0);
                        cairo_fill(cr);
                    }
                }
            }
        }
        for (int s = 0; s < 42; ++s) {
            double sp = fractf(hash21(s, 14.3f) + t * 0.18);
            double sx = 140.0 + hash21(s, 3.3f) * 1640.0 + sp * 200.0;
            double sy = H * (0.66 - sp * 0.62);
            cairo_set_source_rgba(cr, 0.84, 0.94, 1.0, std::sin(sp * M_PI) * 0.94);
            cairo_arc(cr, sx, sy, 2.6 + 2.2 * af.air, 0, 2.0 * M_PI);
            cairo_fill(cr);
        }
        break;
    }
    case SceneKind::MEMORY_TIME_MONTAGE: {
        // Shot 32 ("白手起 我手起繁城 过去未来"): Golden light ribbons connecting foreground silhouettes of past & future!
        double hy = H * 0.58;
        draw_cable_bridge(cr, gcr, hy, t, 0.72);

        double deck_y = H * 0.84;
        cairo_set_source_rgba(cr, 0.015, 0.022, 0.038, 0.98);
        cairo_rectangle(cr, 0, deck_y, W, H - deck_y);
        cairo_fill(cr);
        cairo_set_line_width(cr, 4.0);
        cairo_move_to(cr, 0, deck_y - 32);
        cairo_line_to(cr, W, deck_y - 32);
        cairo_stroke(cr);

        draw_traveler(cr, nullptr, 460.0, deck_y + 18.0, 1.45, 1.0, 0.0, 0.0, 0.65, 0.0, 0.45, 0.96, 0.78, 0.46, 0.40, 0.42);
        draw_traveler(cr, gcr, 1420.0, deck_y + 18.0, 1.75, 1.0, t * 2.0, 0.35, -0.35, 0.65, 0.90, 0.98, 0.88, 0.56, 0.88, 0.96);

        for (int r = 0; r < 4; ++r) {
            cairo_set_source_rgba(cr, 1.0, 0.84, 0.48, 0.34 + 0.12 * af.mid);
            cairo_set_line_width(cr, 2.5);
            cairo_move_to(cr, 160, deck_y - 90 - r * 32);
            cairo_curve_to(cr, 680, H * (0.48 + r * 0.05), 1240, H * (0.34 - r * 0.05), 1780, H * (0.14 + r * 0.04));
            cairo_stroke(cr);
        }
        break;
    }
    case SceneKind::SILENT_COBALT_SAIL: {
        // Shot 33 ("要去哪 要跨海行船 不胜不还"): Silent mirror cobalt sea & glowing origami sailboat!
        double bx = 920.0 + prog * 180.0;
        double by = H * 0.545;
        draw_paper_motif(cr, gcr, bx, by, 68.0, std::sin(t * 1.1) * 0.025, 2, 0.0, 1.0, 0.98, 0.98, 1.0);
        cairo_save(cr);
        cairo_translate(cr, bx, by + 48.0);
        cairo_scale(cr, 1.0, -0.52);
        draw_paper_motif(cr, nullptr, 0, 0, 68.0, -std::sin(t * 1.1) * 0.025, 2, 0.0, 0.32, 0.72, 0.88, 1.0);
        cairo_restore(cr);
        break;
    }
    case SceneKind::DAWN_SEA_HORIZON: {
        // Shot 34: Peaceful morning sail gliding toward dawn horizon + tiny sea birds soaring!
        double bx = 1100.0 + prog * 120.0;
        double by = H * 0.54;
        double sc = 56.0 - prog * 24.0;
        draw_paper_motif(cr, gcr, bx, by, sc, 0.0, 2, 0.0, 0.90, 1.0, 0.95, 0.86);
        for (int b = 0; b < 5; ++b) {
            double bu = fractf(prog * 0.6 + b * 0.14);
            double x = 360.0 + b * 140.0 + prog * 220.0;
            double y = H * (0.32 + 0.04 * b) - prog * 55.0;
            draw_paper_motif(cr, nullptr, x, y, 11.0, -0.15, 1, t * 5.0 + b, 0.55 * (1.0 - bu * 0.4), 0.92, 0.94, 0.98);
        }
        break;
    }
    case SceneKind::EPILOGUE_STARS_SEA: {
        // Shot 35: Distant serene sail fading gently into dawn stillness
        double fade = 1.0 - smoothstep(0.0f, 0.65f, prog);
        double bx = 1220.0 + prog * 90.0;
        draw_paper_motif(cr, gcr, bx, H * 0.54, std::max(14.0, 32.0 - prog * 14.0), 0.0, 2, 0.0, fade * 0.85, 1.0, 0.95, 0.86);
        break;
    }
    }

    cairo_restore(cr);
}

}  // namespace mv
