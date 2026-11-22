#pragma once

#include "audio_lyrics.hpp"
#include "hdr_canvas.hpp"
#include "sky_and_atmosphere.hpp"
#include <algorithm>
#include <cairo/cairo.h>
#include <cmath>
#include <vector>

namespace mv {

inline double y_act(double yn) {
    return BAR_H + yn * (H - 2 * BAR_H);
}

// Expressive backlit cinematic traveler silhouette with coat physics, gait, and worn backpack
inline void draw_traveler(cairo_t* cr, cairo_t* gcr,
                          double fx, double fy, double sc,
                          double facing,      // +1 right, -1 left
                          double walk_phase,  // continuous radians
                          double walk_amp,    // 0 = standing, 1 = full stride
                          double head_bow,    // 0 = level, +1 = bowed down (crying), -0.6 = looking up at sky
                          double arm_reach,   // 0 = relaxed, 1 = reaching up/raising toast
                          double wind,        // 0..1.5 coat & strap wind flutter
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

    auto stroke_and_fill_limb = [&](double x0, double y0, double x1, double y1, double x2, double y2, double w0, double w1) {
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

    // Dark velvet body silhouette color
    cairo_set_source_rgba(cr, 0.035, 0.045, 0.070, alpha);

    // Back leg
    double bl_hip_x = -2.0 + lean * 14.0;
    double bl_hip_y = -68.0;
    double bl_knee_x = bl_hip_x - stride * 18.0 + 4.0;
    double bl_knee_y = -36.0 - knee_b * 0.4;
    double bl_foot_x = bl_hip_x - stride * 32.0 - 2.0;
    double bl_foot_y = -3.0 - knee_b * 0.65;
    stroke_and_fill_limb(bl_hip_x, bl_hip_y, bl_knee_x, bl_knee_y, bl_foot_x, bl_foot_y, 13.0, 10.0);

    // Front leg
    double fl_hip_x = 3.0 + lean * 14.0;
    double fl_hip_y = -68.0;
    double fl_knee_x = fl_hip_x + stride * 18.0 + 5.0;
    double fl_knee_y = -36.0 - knee_f * 0.4;
    double fl_foot_x = fl_hip_x + stride * 32.0 + 4.0;
    double fl_foot_y = -3.0 - knee_f * 0.65;
    stroke_and_fill_limb(fl_hip_x, fl_hip_y, fl_knee_x, fl_knee_y, fl_foot_x, fl_foot_y, 13.5, 10.0);

    // Sneakers
    auto draw_shoe = [&](double sx, double sy) {
        cairo_move_to(cr, sx - 6, sy + 2);
        cairo_line_to(cr, sx + 12, sy + 2);
        cairo_curve_to(cr, sx + 15, sy - 4, sx + 6, sy - 7, sx - 5, sy - 5);
        cairo_close_path(cr);
        cairo_fill(cr);
    };
    draw_shoe(bl_foot_x, bl_foot_y);
    draw_shoe(fl_foot_x, fl_foot_y);

    // Worn canvas backpack (behind torso) with dangling loose strap
    double bp_x = -14.0 + lean * 36.0;
    double bp_y = -126.0;
    cairo_save(cr);
    cairo_translate(cr, bp_x, bp_y);
    cairo_rotate(cr, lean * 0.8);
    cairo_move_to(cr, -16, -2);
    cairo_curve_to(cr, -24, 14, -23, 38, -14, 46);
    cairo_line_to(cr, 2, 46);
    cairo_line_to(cr, 3, -2);
    cairo_close_path(cr);
    cairo_fill(cr);
    // Dangling canvas strap blowing in the wind
    cairo_set_line_width(cr, 2.2);
    double strap_sway = wind * 14.0 + std::sin(walk_phase * 1.4) * 3.0;
    cairo_move_to(cr, -16, 43);
    cairo_curve_to(cr, -20 - strap_sway * 0.4, 54, -22 - strap_sway, 62, -19 - strap_sway * 1.15, 70);
    cairo_stroke(cr);
    cairo_restore(cr);

    // Coat & torso with wind-blown hem
    double sh_x = 4.0 + lean * 46.0;
    double sh_y = -132.0;
    double hem_flutter = wind * 24.0 + std::sin(walk_phase * 1.6 + 0.5) * 4.5 * (0.4 + wind);
    cairo_move_to(cr, sh_x - 12, sh_y);
    cairo_curve_to(cr, sh_x + 2, sh_y - 4, sh_x + 14, sh_y + 2, sh_x + 13, sh_y + 16);
    cairo_curve_to(cr, sh_x + 12, sh_y + 42, 14.0 + lean * 12.0, -76, 15.0 + stride * 7.0, -52);
    cairo_curve_to(cr, 2.0, -48, -12.0 - hem_flutter * 0.5, -49, -18.0 - hem_flutter, -53);
    cairo_curve_to(cr, -16.0 - hem_flutter * 0.3, -78, sh_x - 14, sh_y + 22, sh_x - 12, sh_y);
    cairo_close_path(cr);
    cairo_fill(cr);

    // Head & collar
    double hx = sh_x + 2.0 + head_bow * 5.0;
    double hy = sh_y - 16.0 + head_bow * 5.0;
    // Neck / scarf collar
    cairo_move_to(cr, sh_x - 7, sh_y + 2);
    cairo_line_to(cr, hx - 3, hy + 8);
    cairo_line_to(cr, hx + 6, hy + 8);
    cairo_line_to(cr, sh_x + 9, sh_y + 3);
    cairo_close_path(cr);
    cairo_fill(cr);

    // Tapered natural coat scarf tail fluttering downward in wind
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

    // Head oval + subtle wind hair tuft
    cairo_save(cr);
    cairo_translate(cr, hx, hy);
    cairo_rotate(cr, head_bow * 0.45);
    cairo_scale(cr, 1.0, 1.15);
    cairo_arc(cr, 0, 0, 10.5, 0, 2.0 * M_PI);
    cairo_fill(cr);
    // Hair tuft
    cairo_move_to(cr, -9, -3);
    cairo_curve_to(cr, -14 - wind * 5.0, -10, -4, -14, 7, -9);
    cairo_curve_to(cr, 11, -5, 10, 1, 7, 3);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_restore(cr);

    // Front arm (relaxed swing, or raised toward sky/glass!)
    double arm_sh_x = sh_x + 4.0;
    double arm_sh_y = sh_y + 8.0;
    double elb_x, elb_y, hand_x, hand_y;
    if (arm_reach > 0.05) {
        double r_ang = -0.35 + arm_reach * 1.45;  // sweeps up from down-forward to up-skyward
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
    cairo_set_source_rgba(cr, 0.040, 0.052, 0.082, alpha);
    stroke_and_fill_limb(arm_sh_x, arm_sh_y, elb_x, elb_y, hand_x, hand_y, 8.2, 6.4);

    // Crisp luminous rim light along windward/sky-facing profile
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
            cairo_set_source_rgba(gcr, r_rim, g_rim, b_rim, rim_alpha * 0.55 * alpha);
            cairo_set_line_width(gcr, 3.0);
            cairo_move_to(gcr, sh_x + 11, sh_y + 2);
            cairo_line_to(gcr, 15.0 + lean * 12.0, -76);
            cairo_stroke(gcr);
            cairo_restore(gcr);
        }
    }

    cairo_restore(cr);
}

// Origami motif: 0 = creased ticket, 1 = paper bird, 2 = paper sailboat
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
            // Creased arrival ticket with diagonal fold
            cairo_move_to(c, -1.0, -0.55);
            cairo_line_to(c, 1.0, -0.55);
            cairo_line_to(c, 1.0, 0.55);
            cairo_line_to(c, -1.0, 0.55);
            cairo_close_path(c);
            cairo_set_source_rgba(c, r, g, b, 0.85 * alpha * mult);
            cairo_fill(c);
            cairo_set_source_rgba(c, r * 0.65, g * 0.68, b * 0.72, 0.65 * alpha * mult);
            cairo_set_line_width(c, lw);
            cairo_move_to(c, -0.8, -0.4);
            cairo_line_to(c, 0.6, 0.5);
            cairo_stroke(c);
        } else if (mode == 1) {
            // Graceful origami bird in tailwind
            double wy = -0.55 + std::sin(flap) * 0.35;
            cairo_move_to(c, -0.9, 0.15);
            cairo_line_to(c, 0.0, -0.15);
            cairo_line_to(c, 1.0, 0.0);
            cairo_line_to(c, 0.15, 0.25);
            cairo_close_path(c);
            cairo_set_source_rgba(c, r, g, b, 0.90 * alpha * mult);
            cairo_fill(c);
            cairo_move_to(c, -0.1, -0.05);
            cairo_line_to(c, 0.35, wy - 0.55);
            cairo_line_to(c, 0.45, 0.05);
            cairo_close_path(c);
            cairo_set_source_rgba(c, r * 0.88, g * 0.90, b * 0.95, 0.82 * alpha * mult);
            cairo_fill(c);
        } else {
            // Folded paper sailboat
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
    draw_on(cr, 1.0, 0.06);
    if (gcr) {
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        draw_on(gcr, 0.55, 0.08);
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
        float speed = 920.0f * z;
        float x0 = fractf(hash21(fi, 3.7f) + t * (slant_dx * 0.18f * z)) * (W + 240.0f) - 120.0f;
        float y0 = fractf(hash21(fi, 17.9f) + t * speed / static_cast<float>(H)) * (H - 2 * BAR_H) + BAR_H;
        float len = (24.0f + 38.0f * z) * (0.7 + 0.5 * intensity);
        float dx = static_cast<float>(slant_dx) * len * 0.45f;
        cairo_set_source_rgba(cr, 0.72, 0.84, 0.96, (0.12 + 0.24 * z) * intensity);
        cairo_set_line_width(cr, 0.7 + 0.9 * z);
        cairo_move_to(cr, x0, y0);
        cairo_line_to(cr, x0 + dx, y0 + len);
        cairo_stroke(cr);
    }
    cairo_restore(cr);
}

inline void draw_cable_bridge(cairo_t* cr, cairo_t* gcr, double y_deck, double t, double glow_boost) {
    cairo_save(cr);
    double pylon_xs[2] = {540.0, 1340.0};
    double pylon_h = 215.0;

    // Bridge deck beam
    cairo_set_source_rgba(cr, 0.05, 0.07, 0.11, 0.95);
    cairo_rectangle(cr, 0, y_deck, W, 11.0);
    cairo_fill(cr);

    // Stay cables & A-frame pylons
    for (double px : pylon_xs) {
        cairo_set_source_rgba(cr, 0.05, 0.07, 0.12, 0.95);
        cairo_set_line_width(cr, 9.0);
        cairo_move_to(cr, px - 22.0, y_deck + 32.0);
        cairo_line_to(cr, px - 3.0, y_deck - pylon_h);
        cairo_line_to(cr, px + 3.0, y_deck - pylon_h);
        cairo_line_to(cr, px + 22.0, y_deck + 32.0);
        cairo_stroke(cr);

        // Harp-like stay cables
        for (int c = 1; c <= 11; ++c) {
            double frac = c / 12.0;
            double span = frac * 390.0;
            double cy_top = y_deck - pylon_h * (0.35 + 0.62 * frac);
            cairo_set_source_rgba(cr, 0.62, 0.76, 0.95, 0.16 + 0.14 * glow_boost);
            cairo_set_line_width(cr, 1.1);
            cairo_move_to(cr, px, cy_top);
            cairo_line_to(cr, px - span, y_deck);
            cairo_move_to(cr, px, cy_top);
            cairo_line_to(cr, px + span, y_deck);
            cairo_stroke(cr);
        }

        // Pylon beacon & cable glow
        if (gcr) {
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_set_source_rgba(gcr, 1.0, 0.62, 0.28, 0.55 + 0.35 * glow_boost);
            cairo_arc(gcr, px, y_deck - pylon_h - 4.0, 5.5, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }
    }

    // Moving bridge traffic light pearls along deck
    for (int i = 0; i < 26; ++i) {
        double dir = (i % 2 == 0) ? 1.0 : -1.0;
        double spd = 0.038 + 0.024 * hash21(i, 4.1f);
        double cx = fractf(hash21(i, 9.2f) + dir * t * spd) * W;
        double cy = y_deck + (dir > 0 ? 2.5 : 5.5);
        if (dir > 0) cairo_set_source_rgba(cr, 1.0, 0.88, 0.68, 0.85);
        else cairo_set_source_rgba(cr, 1.0, 0.32, 0.25, 0.80);
        cairo_arc(cr, cx, cy, 2.3, 0, 2.0 * M_PI);
        cairo_fill(cr);
        if (gcr) {
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            if (dir > 0) cairo_set_source_rgba(gcr, 1.0, 0.78, 0.45, 0.65);
            else cairo_set_source_rgba(gcr, 1.0, 0.26, 0.20, 0.55);
            cairo_arc(gcr, cx, cy, 3.8, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }
    }
    cairo_restore(cr);
}

inline void render_cairo_scene(HdrCanvas& canvas, double time_sec, const AudioFrame& af, int act_id) {
    cairo_t* cr = canvas.scene_cr;
    cairo_t* gcr = canvas.glow_cr;
    const double t = time_sec;

    // Clip scene rendering cleanly inside the 2.39:1 active picture rectangle
    cairo_save(cr);
    cairo_rectangle(cr, 0, BAR_H, W, H - 2 * BAR_H);
    cairo_clip(cr);

    cairo_save(gcr);
    cairo_rectangle(gcr, 0, BAR_H / 2.0, AW, AH - BAR_H);
    cairo_clip(gcr);

    if (act_id == 0) {
        // PROLOGUE: Inside metro car window silhouette + condensation droplets + drifting ticket
        cairo_set_source_rgba(cr, 0.018, 0.022, 0.032, 0.94);
        // Window frame perimeter
        cairo_rectangle(cr, 0, BAR_H, 190, H - 2 * BAR_H);
        cairo_rectangle(cr, W - 190, BAR_H, 190, H - 2 * BAR_H);
        cairo_rectangle(cr, 0, BAR_H, W, 56);
        cairo_rectangle(cr, 0, H - BAR_H - 72, W, 72);
        cairo_fill(cr);

        // Rounded window bezel edge
        cairo_set_line_width(cr, 28.0);
        cairo_set_source_rgba(cr, 0.024, 0.030, 0.044, 0.96);
        cairo_rectangle(cr, 195, BAR_H + 56, W - 390, H - 2 * BAR_H - 128);
        cairo_stroke(cr);

        // Vertical metro grab pole
        cairo_set_source_rgba(cr, 0.075, 0.095, 0.125, 0.95);
        cairo_rectangle(cr, 1430, BAR_H, 14, H - 2 * BAR_H);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.35, 0.55, 0.78, 0.28);
        cairo_rectangle(cr, 1431, BAR_H, 3, H - 2 * BAR_H);
        cairo_fill(cr);

        // Glass rain beads
        for (int i = 0; i < 48; ++i) {
            double bx = 220.0 + hash21(i, 2.3f) * (W - 440.0);
            double by = y_act(0.08 + 0.80 * hash21(i, 8.7f)) + std::fmod(t * (6.0 + 10.0 * hash21(i, 4.0f)), 40.0);
            double br = 1.6 + 3.2 * hash21(i, 15.1f);
            cairo_set_source_rgba(cr, 0.65, 0.80, 0.95, 0.22);
            cairo_arc(cr, bx, by, br, 0, 2.0 * M_PI);
            cairo_fill(cr);
        }
    } else if (act_id == 1) {
        // ACT I: Rain crosswalk, buildings, traffic green-light turn at 36.4s, traveler crying -> walking
        double horizon_y = y_act(0.63);
        double green_t = smoothstep(36.1f, 36.8f, static_cast<float>(t));
        double wind_shift = smoothstep(18.5f, 21.5f, static_cast<float>(t));

        // Dark rainy urban silhouettes
        cairo_set_source_rgba(cr, 0.020, 0.028, 0.045, 0.96);
        struct Bldg { double x, w, h; };
        Bldg bldgs[] = {{0, 240, 420}, {210, 170, 310}, {360, 220, 380}, {1320, 240, 390}, {1540, 190, 460}, {1710, 220, 340}};
        for (auto& b : bldgs) {
            cairo_rectangle(cr, b.x, horizon_y - b.h, b.w, b.h + 4.0);
        }
        cairo_fill(cr);

        // Distant shop awning neon bar on left
        cairo_set_source_rgba(cr, 0.95, 0.25, 0.45, 0.82);
        cairo_rectangle(cr, 185, horizon_y - 142, 130, 10);
        cairo_fill(cr);
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.24, 0.44, 0.85);
        cairo_rectangle(gcr, 185, horizon_y - 144, 130, 14);
        cairo_fill(gcr);
        cairo_restore(gcr);

        // Streetlight pole mid-left
        cairo_set_source_rgba(cr, 0.03, 0.04, 0.06, 0.96);
        cairo_rectangle(cr, 725, horizon_y - 310, 8, 312);
        cairo_rectangle(cr, 725, horizon_y - 310, 44, 6);
        cairo_fill(cr);
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 0.48, 0.78, 1.0, 0.85);
        cairo_arc(gcr, 766, horizon_y - 304, 9, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);

        // Perspective zebra crosswalk stripes on wet asphalt
        for (int s = 0; s < 6; ++s) {
            double u0 = 0.22 + s * 0.11;
            double u1 = u0 + 0.055;
            double x_top0 = 960.0 + (u0 - 0.5) * 900.0;
            double x_top1 = 960.0 + (u1 - 0.5) * 900.0;
            double x_bot0 = 960.0 + (u0 - 0.5) * 1850.0;
            double x_bot1 = 960.0 + (u1 - 0.5) * 1850.0;
            cairo_move_to(cr, x_top0, horizon_y + 24);
            cairo_line_to(cr, x_top1, horizon_y + 24);
            cairo_line_to(cr, x_bot1, H - BAR_H - 12);
            cairo_line_to(cr, x_bot0, H - BAR_H - 12);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 0.56, 0.66, 0.78, 0.18 + 0.06 * green_t);
            cairo_fill(cr);
        }

        // Pedestrian Traffic Signal Pole on right (Red -> Emerald Green at 36.4s!)
        double sig_x = 1460.0;
        double sig_y = horizon_y - 250.0;
        cairo_set_source_rgba(cr, 0.025, 0.032, 0.048, 0.98);
        cairo_rectangle(cr, sig_x - 5, sig_y, 10, horizon_y - sig_y + 42);
        cairo_rectangle(cr, sig_x - 24, sig_y - 52, 48, 92);
        cairo_fill(cr);

        double sr = (1.0 - green_t) * 1.0 + green_t * 0.18;
        double sg = (1.0 - green_t) * 0.18 + green_t * 0.98;
        double sb = (1.0 - green_t) * 0.20 + green_t * 0.58;
        double lamp_y = (green_t < 0.5) ? (sig_y - 28) : (sig_y + 14);
        cairo_set_source_rgba(cr, sr, sg, sb, 0.95);
        cairo_arc(cr, sig_x, lamp_y, 11.5, 0, 2.0 * M_PI);
        cairo_fill(cr);
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, sr, sg, sb, 0.95);
        cairo_arc(gcr, sig_x, lamp_y, 16.0, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);

        // Protagonist: standing still crying (15..36s) -> lifts head & crosses intersection (36.7..43.2s)
        double walk_start = 36.8;
        double walk_u = smoothstep(static_cast<float>(walk_start), 43.2f, static_cast<float>(t));
        double px = 760.0 + walk_u * 290.0;
        double py = horizon_y + 110.0 - walk_u * 35.0;
        double pscale = 1.65 - walk_u * 0.18;
        double bow = 0.85 * (1.0 - smoothstep(32.5f, 36.6f, static_cast<float>(t)));
        double walk_amp = smoothstep(36.7f, 37.8f, static_cast<float>(t));
        double wphase = std::max(0.0, t - walk_start) * 5.2;
        double wind = 0.25 + 0.55 * wind_shift;

        // Inverted wet asphalt reflection silhouette
        cairo_save(cr);
        cairo_translate(cr, px, py + 4);
        cairo_scale(cr, 1.0, -0.58);
        draw_traveler(cr, nullptr, 0, 0, pscale, 1.0, wphase, walk_amp, bow, 0.0, wind,
                      sr, sg, sb, 0.15, 0.28);
        cairo_restore(cr);

        // Main silhouette
        draw_traveler(cr, gcr, px, py, pscale, 1.0, wphase, walk_amp, bow, 0.0, wind,
                      sr, sg, sb, 0.65, 1.0);

        // Creased arrival ticket peeking beside backpack / held in hand
        draw_paper_motif(cr, gcr, px + 26.0, py - 95.0 * pscale * 0.6, 11.0, -0.25, 0, 0.0, 0.72);

        // Slanted heavy rain accelerating sideways when wind hits ("过了雨 总有好风")
        draw_rain(cr, t, 0.15 + 0.62 * wind_shift, 0.92, 220);
    } else if (act_id == 2) {
        // ACT II: Midnight apartment building facade with glowing corner room, gooseneck lamp, mug steam, pillow!
        double zoom = 1.0 + 0.08 * smoothstep(43.2f, 59.0f, static_cast<float>(t));
        cairo_save(cr);
        cairo_translate(cr, W * 0.54, H * 0.50);
        cairo_scale(cr, zoom, zoom);
        cairo_translate(cr, -W * 0.54, -H * 0.50);

        // Dark apartment facade
        cairo_set_source_rgba(cr, 0.018, 0.024, 0.038, 0.98);
        cairo_rectangle(cr, 180, BAR_H, W - 360, H - 2 * BAR_H);
        cairo_fill(cr);

        // Grid of sleeping apartment windows
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 5; ++col) {
                double wx = 260.0 + col * 290.0;
                double wy = y_act(0.08 + row * 0.25);
                if (row >= 1 && row <= 2 && col >= 1 && col <= 3) continue;  // Main warm window aperture
                bool faint_lit = ((row * 7 + col * 3) % 5 == 0);
                if (faint_lit) {
                    cairo_set_source_rgba(cr, 0.24, 0.32, 0.44, 0.22);
                } else {
                    cairo_set_source_rgba(cr, 0.030, 0.042, 0.068, 0.85);
                }
                cairo_rectangle(cr, wx, wy, 190, 128);
                cairo_fill(cr);
            }
        }

        // Main Hero Window Room: Warm Tungsten Sanctuary!
        double rx = 640.0, ry = y_act(0.24), rw = 640.0, rh = 420.0;
        cairo_pattern_t* pat = cairo_pattern_create_radial(rx + rw * 0.68, ry + rh * 0.45, 20.0,
                                                           rx + rw * 0.55, ry + rh * 0.55, 420.0);
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 1.00, 0.80, 0.46, 0.96);
        cairo_pattern_add_color_stop_rgba(pat, 0.45, 0.72, 0.42, 0.18, 0.92);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.14, 0.11, 0.13, 0.98);
        cairo_set_source(cr, pat);
        cairo_rectangle(cr, rx, ry, rw, rh);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);

        // Glow layer ONLY around desk lamp bulb & soft reading cone (keeps pillow, mug & desk crisp!)
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.74, 0.34, 0.78 + 0.18 * af.mid);
        cairo_arc(gcr, rx + 486.0, ry + rh - 222.0, 26.0, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_set_source_rgba(gcr, 1.0, 0.68, 0.28, 0.28);
        cairo_move_to(gcr, rx + 478, ry + rh - 220);
        cairo_line_to(gcr, rx + 395, ry + rh - 125);
        cairo_line_to(gcr, rx + 558, ry + rh - 125);
        cairo_close_path(gcr);
        cairo_fill(gcr);
        cairo_restore(gcr);

        // Room interior silhouettes: Bed & Pillow on left ("信入睡的枕头"), Desk & Gooseneck Lamp on right
        cairo_set_source_rgba(cr, 0.055, 0.036, 0.045, 0.96);
        // Bed frame & soft mattress
        cairo_rectangle(cr, rx + 24, ry + rh - 108, 220, 108);
        cairo_rectangle(cr, rx + 24, ry + rh - 150, 16, 150);
        cairo_fill(cr);
        // Soft luminous pillow resting cleanly on bed
        cairo_set_source_rgba(cr, 0.94, 0.88, 0.80, 0.92);
        cairo_move_to(cr, rx + 46, ry + rh - 108);
        cairo_curve_to(cr, rx + 58, ry + rh - 134, rx + 110, ry + rh - 134, rx + 122, ry + rh - 108);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Wooden desk & chair silhouette on right
        cairo_set_source_rgba(cr, 0.045, 0.030, 0.038, 0.98);
        cairo_rectangle(cr, rx + 340, ry + rh - 125, 250, 14);
        cairo_rectangle(cr, rx + 356, ry + rh - 111, 12, 111);
        cairo_rectangle(cr, rx + 566, ry + rh - 111, 12, 111);
        // Stacked books
        cairo_rectangle(cr, rx + 368, ry + rh - 152, 54, 27);
        // Ceramic mug
        cairo_rectangle(cr, rx + 444, ry + rh - 148, 20, 23);
        cairo_fill(cr);

        // Creased blueprint paper flat on the desk
        draw_paper_motif(cr, nullptr, rx + 505, ry + rh - 131, 22.0, 0.03, 0, 0.0, 0.90);

        // Arched gooseneck desk lamp + volumetric warm cone
        cairo_set_source_rgba(cr, 0.035, 0.025, 0.032, 0.98);
        cairo_set_line_width(cr, 5.5);
        cairo_move_to(cr, rx + 550, ry + rh - 125);
        cairo_curve_to(cr, rx + 565, ry + rh - 240, rx + 510, ry + rh - 275, rx + 482, ry + rh - 235);
        cairo_stroke(cr);
        // Lamp shade
        cairo_move_to(cr, rx + 465, ry + rh - 222);
        cairo_line_to(cr, rx + 498, ry + rh - 242);
        cairo_line_to(cr, rx + 504, ry + rh - 214);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Warm lamp cone
        cairo_set_source_rgba(cr, 1.0, 0.88, 0.60, 0.28 + 0.08 * af.mid);
        cairo_move_to(cr, rx + 478, ry + rh - 220);
        cairo_line_to(cr, rx + 395, ry + rh - 125);
        cairo_line_to(cr, rx + 558, ry + rh - 125);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Rising curls of mug steam
        cairo_set_source_rgba(cr, 1.0, 0.94, 0.82, 0.42);
        cairo_set_line_width(cr, 2.2);
        for (int s = 0; s < 2; ++s) {
            double sx = rx + 450 + s * 8.0;
            double sy = ry + rh - 152;
            cairo_move_to(cr, sx, sy);
            cairo_curve_to(cr, sx + std::sin(t * 2.1 + s) * 10, sy - 22,
                           sx - std::cos(t * 1.8 + s) * 10, sy - 44,
                           sx + std::sin(t * 1.5 + s) * 12, sy - 68);
            cairo_stroke(cr);
        }

        // Window wooden mullions framing the glass
        cairo_set_source_rgba(cr, 0.022, 0.024, 0.036, 0.98);
        cairo_set_line_width(cr, 14.0);
        cairo_rectangle(cr, rx, ry, rw, rh);
        cairo_stroke(cr);
        cairo_set_line_width(cr, 8.0);
        cairo_move_to(cr, rx + rw * 0.5, ry);
        cairo_line_to(cr, rx + rw * 0.5, ry + rh);
        cairo_move_to(cr, rx, ry + rh * 0.45);
        cairo_line_to(cr, rx + rw, ry + rh * 0.45);
        cairo_stroke(cr);

        cairo_restore(cr);
        draw_rain(cr, t, 0.18, 0.45, 95);
    } else if (act_id == 3) {
        // ACT III: 06:15 AM Morning Viaduct + Roaring Express Train + Waiting Traveler leaning forward
        double plat_y = y_act(0.76);

        // Misty morning distant mountain silhouettes
        cairo_set_source_rgba(cr, 0.065, 0.12, 0.19, 0.65);
        cairo_move_to(cr, 0, plat_y);
        for (int x = 0; x <= W; x += 40) {
            double mh = 150.0 + 75.0 * std::sin(x * 0.0032 + 1.2) + 35.0 * std::cos(x * 0.0085);
            cairo_line_to(cr, x, plat_y - mh);
        }
        cairo_line_to(cr, W, plat_y);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Elevated viaduct track & catenary gantry masts
        double track_y = y_act(0.56);
        cairo_set_source_rgba(cr, 0.032, 0.048, 0.072, 0.95);
        cairo_rectangle(cr, 0, track_y, W, 22);
        for (int px = 140; px < W; px += 340) {
            cairo_rectangle(cr, px, track_y + 20, 26, plat_y - track_y);
            cairo_rectangle(cr, px + 6, track_y - 78, 8, 82);
            cairo_rectangle(cr, px - 14, track_y - 74, 34, 5);
        }
        cairo_fill(cr);

        // Overhead power catenary lines
        cairo_set_source_rgba(cr, 0.22, 0.32, 0.44, 0.40);
        cairo_set_line_width(cr, 1.5);
        cairo_move_to(cr, 0, track_y - 71);
        cairo_line_to(cr, W, track_y - 71);
        cairo_stroke(cr);

        // Express Commuter Train roaring left-to-right ("时代轰隆隆地来不转弯")
        double train_u = std::fmod(std::max(0.0, t - 58.2) * 0.16, 1.25);
        double train_x = -1250.0 + train_u * (W + 1350.0);
        double ty = track_y - 54.0;
        cairo_set_source_rgba(cr, 0.045, 0.068, 0.105, 0.98);
        for (int car = 0; car < 5; ++car) {
            double cx = train_x + car * 238.0;
            cairo_rectangle(cr, cx, ty, 228.0, 52.0);
            cairo_fill(cr);
            // Warm sunlit carriage windows
            for (int w = 0; w < 6; ++w) {
                cairo_set_source_rgba(cr, 1.0, 0.84, 0.56, 0.85);
                cairo_rectangle(cr, cx + 16 + w * 34, ty + 14, 24, 18);
                cairo_fill(cr);
            }
            cairo_set_source_rgba(cr, 0.045, 0.068, 0.105, 0.98);
        }
        // Train nose cone + dual bright headlamp beams
        double nose_x = train_x + 5 * 238.0;
        cairo_move_to(cr, nose_x, ty);
        cairo_curve_to(cr, nose_x + 48, ty + 6, nose_x + 62, ty + 38, nose_x + 58, ty + 52);
        cairo_line_to(cr, nose_x, ty + 52);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.90, 0.65, 0.85);
        cairo_arc(gcr, nose_x + 52, ty + 34, 8.0, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        for (int car = 0; car < 5; ++car) {
            double cx = train_x + car * 238.0;
            cairo_set_source_rgba(gcr, 1.0, 0.78, 0.42, 0.45);
            cairo_rectangle(gcr, cx + 12, ty + 14, 204, 18);
            cairo_fill(gcr);
        }
        cairo_restore(gcr);

        // Foreground station platform + yellow safety tactile line
        cairo_set_source_rgba(cr, 0.024, 0.032, 0.048, 0.98);
        cairo_rectangle(cr, 0, plat_y, W, H - BAR_H - plat_y);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.95, 0.76, 0.22, 0.88);
        cairo_rectangle(cr, 0, plat_y + 18, W, 6);
        cairo_fill(cr);

        // Station overhead steel roof canopy
        cairo_set_source_rgba(cr, 0.018, 0.026, 0.042, 0.96);
        cairo_move_to(cr, 0, BAR_H);
        cairo_line_to(cr, 920, BAR_H);
        cairo_line_to(cr, 680, BAR_H + 68);
        cairo_line_to(cr, 0, BAR_H + 115);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Protagonist waiting steadily by the yellow line, coat whipping in the train's slipstream!
        double slipstream = 0.45 + 0.95 * std::exp(-std::pow((train_x + 500.0 - 460.0) / 620.0, 2.0));
        draw_traveler(cr, gcr, 440.0, plat_y + 22.0, 1.62, 1.0, t * 1.4, 0.0,
                      -0.25, 0.0, slipstream, 0.96, 0.74, 0.46, 0.72, 1.0);
    } else if (act_id == 4) {
        // ACT IV (Chorus 1): Mountain ridge -> Blueprints rising into glowing towers + cranes + star catch!
        double local_t = std::max(0.0, t - 74.0);
        double rise = smoothstep(0.0f, 7.0f, static_cast<float>(local_t));
        double star_reach = smoothstep(13.5f, 16.5f, static_cast<float>(local_t));
        double base_y = y_act(0.84);

        // Layered mountain silhouette on left/far ground ("凭脚踝翻过山")
        cairo_set_source_rgba(cr, 0.022, 0.036, 0.068, 0.96);
        cairo_move_to(cr, 0, H - BAR_H);
        for (int x = 0; x <= W; x += 30) {
            double mh = 115.0 * std::exp(-std::pow((x - 280.0) / 420.0, 2.0)) +
                        65.0 * std::sin(x * 0.005) + 45.0;
            cairo_line_to(cr, x, base_y - mh);
        }
        cairo_line_to(cr, W, H - BAR_H);
        cairo_close_path(cr);
        cairo_fill(cr);

        // Rising architectural skyline ("无名闯繁城 盖梦中楼")
        for (int i = 0; i < 24; ++i) {
            double bx = 260.0 + i * 64.0 + hash21(i, 1.1f) * 20.0;
            double bw = 38.0 + hash21(i, 2.2f) * 32.0;
            double full_h = 150.0 + hash21(i, 3.3f) * 280.0;
            double delay = hash21(i, 4.4f) * 0.45;
            double grow = smoothstep(static_cast<float>(delay), static_cast<float>(delay + 0.55), static_cast<float>(rise));
            double bh = full_h * grow;
            double top_y = base_y - bh;

            // Blueprint wireframe phase transitioning into solid dark glass tower
            cairo_set_source_rgba(cr, 0.22, 0.58, 0.95, 0.45 * (1.0 - 0.55 * grow));
            cairo_set_line_width(cr, 1.2);
            cairo_rectangle(cr, bx, top_y, bw, bh);
            cairo_stroke(cr);

            cairo_set_source_rgba(cr, 0.022, 0.034, 0.062, 0.92 * grow);
            cairo_rectangle(cr, bx, top_y, bw, bh);
            cairo_fill(cr);

            // Lit windows pulsing subtly with audio bands
            if (grow > 0.35) {
                int cols = std::max(2, static_cast<int>(bw / 11));
                int rows = std::max(3, static_cast<int>(bh / 16));
                for (int r = 1; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        float rnd = hash21(i * 31.0f + c, r * 17.0f);
                        if (rnd > 0.52f) {
                            double band_mod = 0.72 + 0.45 * af.bands[(i + r) % 8];
                            double wx = bx + 5.0 + c * ((bw - 10.0) / cols);
                            double wy = top_y + 8.0 + r * 15.0;
                            bool gold = (rnd > 0.72f);
                            if (gold) cairo_set_source_rgba(cr, 1.0, 0.80, 0.42, 0.78 * band_mod * grow);
                            else cairo_set_source_rgba(cr, 0.45, 0.78, 1.0, 0.58 * band_mod * grow);
                            cairo_rectangle(cr, wx, wy, 5.0, 7.0);
                            cairo_fill(cr);
                        }
                    }
                }
            }
        }

        // Lattice tower cranes sweeping twilight sky
        double crane_xs[3] = {520.0, 910.0, 1270.0};
        double crane_hs[3] = {370.0, 445.0, 390.0};
        for (int k = 0; k < 3; ++k) {
            double cx = crane_xs[k];
            double cy_top = base_y - crane_hs[k] * smoothstep(0.0f, 0.75f, static_cast<float>(rise));
            double jib_len = 130.0 * std::cos(t * 0.25 + k * 1.4);
            cairo_set_source_rgba(cr, 0.08, 0.14, 0.24, 0.90);
            cairo_set_line_width(cr, 4.0);
            cairo_move_to(cr, cx, base_y);
            cairo_line_to(cr, cx, cy_top);
            cairo_move_to(cr, cx - jib_len * 0.35, cy_top + 8);
            cairo_line_to(cr, cx + jib_len, cy_top + 8);
            cairo_stroke(cr);

            // Blinking aviation beacon on top
            double blink = 0.5 + 0.5 * std::sin(t * 3.8 + k * 2.1);
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_set_source_rgba(gcr, 1.0, 0.36, 0.18, 0.5 + 0.5 * blink);
            cairo_arc(gcr, cx, cy_top, 4.8, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }

        // Flowing tailwind ribbons carrying paper birds skyward
        for (int b = 0; b < 7; ++b) {
            double prog = fractf(t * 0.14 + b * 0.15);
            double bx = 280.0 + prog * 1380.0;
            double by = y_act(0.75 - prog * 0.58) + std::sin(prog * 6.0 + b) * 28.0;
            draw_paper_motif(cr, gcr, bx, by, 13.0 - prog * 3.5, -0.35 + std::sin(t * 3 + b) * 0.12,
                             1, t * 7.5 + b, std::sin(prog * M_PI) * 0.88);
        }

        // Foreground rooftop ledge & traveler reaching up to catch a star ("徒手摘星辰 从无到有")
        cairo_set_source_rgba(cr, 0.015, 0.022, 0.038, 0.98);
        cairo_rectangle(cr, 1380, y_act(0.84), 560, 200);
        cairo_fill(cr);

        double tx = 1580.0, ty = y_act(0.84);
        draw_traveler(cr, gcr, tx, ty, 1.48, -1.0, 0.0, 0.0, -0.58, star_reach, 0.85,
                      0.98, 0.82, 0.50, 0.82, 1.0);

        // Radiant star descending onto outstretched fingertips!
        if (star_reach > 0.05) {
            double sx = tx - 56.0;
            double sy = ty - 236.0 + (1.0 - star_reach) * -140.0;
            double pulse = 1.0 + 0.28 * af.air + 0.15 * std::sin(t * 5.0);
            cairo_save(cr);
            cairo_set_source_rgba(cr, 1.0, 0.95, 0.78, star_reach);
            cairo_arc(cr, sx, sy, 5.5 * pulse, 0, 2.0 * M_PI);
            cairo_fill(cr);
            // Cross star flare
            cairo_set_line_width(cr, 2.0);
            cairo_move_to(cr, sx - 28 * pulse, sy);
            cairo_line_to(cr, sx + 28 * pulse, sy);
            cairo_move_to(cr, sx, sy - 22 * pulse);
            cairo_line_to(cr, sx, sy + 22 * pulse);
            cairo_stroke(cr);
            cairo_restore(cr);

            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_set_source_rgba(gcr, 1.0, 0.85, 0.48, 0.95 * star_reach);
            cairo_arc(gcr, sx, sy, 14.0 * pulse, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }
    } else if (act_id == 5) {
        // INTERLUDE (94.5..111.0): Quiet rooftop puddle — past child-self hands glowing light to present traveler!
        double hy = y_act(0.64);
        double reach = smoothstep(96.0f, 107.0f, static_cast<float>(t));
        double orb_x = 800.0 + reach * 310.0;
        double orb_y = hy - 110.0 - std::sin(reach * M_PI) * 38.0;

        // Present traveler on right (facing left) & younger self on left (facing right)
        draw_traveler(cr, gcr, 720.0, hy, 1.15, 1.0, 0.0, 0.0, -0.1, 0.45, 0.12, 1.0, 0.78, 0.45, 0.65, 0.95);
        draw_traveler(cr, gcr, 1180.0, hy, 1.50, -1.0, 0.0, 0.0, 0.15, 0.35, 0.14, 1.0, 0.78, 0.45, 0.65, 0.95);

        // Inverted soft mirror reflections in puddle
        cairo_save(cr);
        cairo_translate(cr, 0, hy * 2.0 + 4.0);
        cairo_scale(cr, 1.0, -0.62);
        draw_traveler(cr, nullptr, 720.0, hy, 1.15, 1.0, 0.0, 0.0, -0.1, 0.45, 0.12, 1.0, 0.78, 0.45, 0.2, 0.25);
        draw_traveler(cr, nullptr, 1180.0, hy, 1.50, -1.0, 0.0, 0.0, 0.15, 0.35, 0.14, 1.0, 0.78, 0.45, 0.2, 0.25);
        cairo_restore(cr);

        // Golden floating spark passing from past self to present self
        cairo_set_source_rgba(cr, 1.0, 0.92, 0.70, 0.95);
        cairo_arc(cr, orb_x, orb_y, 7.0 + 2.0 * af.mid, 0, 2.0 * M_PI);
        cairo_fill(cr);
        cairo_save(gcr);
        cairo_scale(gcr, 0.5, 0.5);
        cairo_set_source_rgba(gcr, 1.0, 0.76, 0.36, 0.92);
        cairo_arc(gcr, orb_x, orb_y, 18.0, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        // Mirror spark reflection
        cairo_set_source_rgba(gcr, 1.0, 0.70, 0.32, 0.40);
        cairo_arc(gcr, orb_x, hy + (hy - orb_y) * 0.65, 15.0, 0, 2.0 * M_PI);
        cairo_fill(gcr);
        cairo_restore(gcr);
    } else if (act_id == 6) {
        // ACT V (111.0..155.4): Harbor bridge, coastal terrace festoon bulbs, diverse strangers raising glasses!
        double hy = y_act(0.59);
        draw_cable_bridge(cr, gcr, hy - 14.0, t, 0.35 + 0.55 * af.mid);

        // Foreground terrace balustrade & deck
        double rail_y = y_act(0.75);
        cairo_set_source_rgba(cr, 0.020, 0.024, 0.036, 0.98);
        cairo_rectangle(cr, 0, rail_y + 42, W, H - BAR_H - (rail_y + 42));
        cairo_fill(cr);
        cairo_set_line_width(cr, 5.0);
        cairo_move_to(cr, 0, rail_y);
        cairo_line_to(cr, W, rail_y);
        cairo_stroke(cr);
        for (int bx = 60; bx < W; bx += 110) {
            cairo_rectangle(cr, bx, rail_y, 6, 44);
        }
        cairo_fill(cr);

        // Overhead warm festoon string bulbs swaying gently in sea breeze
        cairo_set_source_rgba(cr, 0.03, 0.03, 0.05, 0.90);
        cairo_set_line_width(cr, 2.4);
        cairo_move_to(cr, 0, y_act(0.14));
        cairo_curve_to(cr, 640, y_act(0.28), 1280, y_act(0.28), W, y_act(0.14));
        cairo_stroke(cr);

        for (int b = 1; b <= 11; ++b) {
            double u = b / 12.0;
            double bx = u * W;
            double sway = std::sin(t * 1.4 + b * 0.7) * 6.0;
            double by = y_act(0.14 + 4.0 * u * (1.0 - u) * 0.105);
            cairo_set_source_rgba(cr, 1.0, 0.88, 0.62, 0.92);
            cairo_arc(cr, bx + sway, by + 10.0, 6.0, 0, 2.0 * M_PI);
            cairo_fill(cr);
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_set_source_rgba(gcr, 1.0, 0.70, 0.32, 0.88);
            cairo_arc(gcr, bx + sway, by + 10.0, 12.0, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }

        // Five silhouettes along coastal terrace, raising glasses together at 132.8s ("也举起杯 祝你乘风破浪")
        double toast = smoothstep(131.5f, 134.5f, static_cast<float>(t)) * (1.0 - smoothstep(145.0f, 152.0f, static_cast<float>(t)) * 0.55);
        struct Guest { double x, sc, fac, reach_off; };
        Guest guests[] = {{430, 1.32, 1.0, 0.0}, {650, 1.44, 1.0, 0.1}, {920, 1.38, -1.0, -0.05}, {1190, 1.46, -1.0, 0.08}, {1460, 1.30, -1.0, 0.0}};
        for (auto& gt : guests) {
            double ar = toast * (0.62 + gt.reach_off);
            draw_traveler(cr, gcr, gt.x, rail_y + 105.0, gt.sc, gt.fac, t * 0.8, 0.0,
                          -0.12, ar, 0.25, 1.0, 0.76, 0.42, 0.60, 1.0);
            if (toast > 0.1) {
                double gx = gt.x + gt.fac * 40.0 * gt.sc;
                double gy = rail_y + 105.0 - (142.0 + ar * 36.0) * gt.sc;
                cairo_set_source_rgba(cr, 1.0, 0.88, 0.56, toast * 0.90);
                cairo_rectangle(cr, gx - 4, gy - 10, 8, 12);
                cairo_fill(cr);
            }
        }

        // Golden ember motes drifting upward like idealist sparks ("理想要比金子还难得")
        double ember_gain = smoothstep(139.0f, 144.0f, static_cast<float>(t));
        for (int e = 0; e < 26; ++e) {
            double eu = fractf(hash21(e, 6.2f) + t * (0.04 + 0.04 * hash21(e, 1.9f)));
            double ex = hash21(e, 8.8f) * W + std::sin(t * 1.5 + e) * 24.0;
            double ey = y_act(0.92 - eu * 0.82);
            double al = std::sin(eu * M_PI) * (0.45 + 0.50 * ember_gain);
            cairo_set_source_rgba(cr, 1.0, 0.85, 0.45, al);
            cairo_arc(cr, ex, ey, 2.2 + 1.8 * hash21(e, 3.1f), 0, 2.0 * M_PI);
            cairo_fill(cr);
        }
    } else if (act_id == 7) {
        // ACT VI (155.4..184.4): Pre-chorus bridge wind -> Open Sea Sailboat & Whipping Pennant!
        double sail_t = smoothstep(169.5f, 171.2f, static_cast<float>(t));
        double hy = y_act(0.56);

        if (sail_t < 0.98) {
            cairo_save(cr);
            draw_cable_bridge(cr, gcr, hy + 20.0, t, 0.75 * (1.0 - sail_t));
            cairo_restore(cr);
        }

        if (sail_t > 0.02) {
            // Dynamic white-sailed vessel gliding across ocean swells ("挥着旗扬我帆 / 跨海行船 不胜不还")
            double bx = 660.0 + std::sin(t * 0.55) * 90.0 + (t - 170.0) * 28.0;
            double by = y_act(0.67) + std::sin(t * 2.2) * 11.0;
            double heel = -0.06 + 0.04 * std::sin(t * 2.0);
            double sc = 1.65 * sail_t;

            cairo_save(cr);
            cairo_translate(cr, bx, by);
            cairo_rotate(cr, heel);
            cairo_scale(cr, sc, sc);

            // Sleek white-edged hull
            cairo_move_to(cr, -95, 0);
            cairo_line_to(cr, 105, -14);
            cairo_curve_to(cr, 95, 16, 60, 28, -75, 24);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 0.05, 0.09, 0.16, 0.96 * sail_t);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, 0.92, 0.96, 1.0, 0.65 * sail_t);
            cairo_set_line_width(cr, 2.2);
            cairo_move_to(cr, -95, 0);
            cairo_line_to(cr, 105, -14);
            cairo_stroke(cr);

            // Tall mast
            cairo_set_source_rgba(cr, 0.85, 0.90, 0.98, 0.85 * sail_t);
            cairo_set_line_width(cr, 3.2);
            cairo_move_to(cr, 0, 0);
            cairo_line_to(cr, 4, -185);
            cairo_stroke(cr);

            // Billowing mainsail & jib
            double billow = 18.0 + 8.0 * af.bass;
            cairo_move_to(cr, 3, -176);
            cairo_curve_to(cr, -45 - billow, -110, -72 - billow, -42, -78, -14);
            cairo_line_to(cr, 0, -10);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 0.96, 0.97, 1.0, 0.94 * sail_t);
            cairo_fill(cr);

            cairo_move_to(cr, 6, -162);
            cairo_curve_to(cr, 48 + billow * 0.6, -96, 78 + billow * 0.5, -42, 92, -16);
            cairo_line_to(cr, 6, -12);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 0.84, 0.91, 0.98, 0.88 * sail_t);
            cairo_fill(cr);

            // Streaming crimson-gold pennant at masthead ("挥着旗扬我帆")
            cairo_move_to(cr, 4, -185);
            for (int seg = 1; seg <= 6; ++seg) {
                double sx = 4.0 - seg * 14.0;
                double sy = -185.0 + std::sin(t * 11.0 - seg * 1.1) * (2.5 + seg * 1.6);
                cairo_line_to(cr, sx, sy);
            }
            for (int seg = 6; seg >= 1; --seg) {
                double sx = 4.0 - seg * 14.0;
                double sy = -176.0 + std::sin(t * 11.0 - seg * 1.1) * (2.5 + seg * 1.6);
                cairo_line_to(cr, sx, sy);
            }
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 0.98, 0.42, 0.24, 0.95 * sail_t);
            cairo_fill(cr);
            cairo_restore(cr);

            // Frothy bow wave & wake lines
            cairo_set_source_rgba(cr, 0.86, 0.94, 1.0, 0.55 * sail_t);
            cairo_set_line_width(cr, 3.0);
            for (int w = 0; w < 3; ++w) {
                double wy = by + 22.0 + w * 14.0;
                cairo_move_to(cr, bx + 135 - w * 40, wy - 4);
                cairo_curve_to(cr, bx - 40, wy + 8, bx - 260, wy + 4, bx - 460 - w * 70, wy + 18);
                cairo_stroke(cr);
            }
        }
    } else if (act_id == 8) {
        // ACT VII: Station Exit Archway looking out at Burning Sky (184.4..195.8) -> Macro Eye Eyelashes (195.8..203.0)!
        double eye_blend = smoothstep(195.6f, 196.6f, static_cast<float>(t));

        if (eye_blend < 0.99) {
            double a = 1.0 - eye_blend;
            double hy = y_act(0.76);
            // Varied stepped sunset skyline + rooftop spires & cranes
            cairo_set_source_rgba(cr, 0.06, 0.03, 0.05, 0.90 * a);
            for (int i = 0; i < 21; ++i) {
                double bx = 270.0 + i * 66.0 + hash21(i, 2.7f) * 14.0;
                double bw = 34.0 + hash21(i, 8.1f) * 38.0;
                double bh = 55.0 + hash21(i, 5.5f) * 165.0;
                cairo_rectangle(cr, bx, hy - bh, bw, bh + 18.0);
                if (i % 4 == 1) {
                    cairo_rectangle(cr, bx + bw * 0.22, hy - bh - 26.0, bw * 0.56, 26.0);
                }
                if (i % 6 == 2) {
                    cairo_rectangle(cr, bx + bw * 0.46, hy - bh - 64.0, 4.0, 64.0);
                }
            }
            cairo_fill(cr);

            // Dark Station Exit Archway Framing left/right/top
            cairo_set_source_rgba(cr, 0.015, 0.014, 0.022, 0.96 * a);
            cairo_rectangle(cr, 0, BAR_H, 280, H - 2 * BAR_H);
            cairo_rectangle(cr, W - 280, BAR_H, 280, H - 2 * BAR_H);
            cairo_rectangle(cr, 0, BAR_H, W, 92);
            cairo_rectangle(cr, 0, hy, W, H - BAR_H - hy);
            cairo_fill(cr);

            // Subtle archway inner bevel rim catching sunset glow
            cairo_set_source_rgba(cr, 0.95, 0.55, 0.22, 0.28 * a);
            cairo_set_line_width(cr, 2.5);
            cairo_move_to(cr, 280, hy);
            cairo_line_to(cr, 280, BAR_H + 92);
            cairo_line_to(cr, W - 280, BAR_H + 92);
            cairo_line_to(cr, W - 280, hy);
            cairo_stroke(cr);

            // Protagonist with old backpack stepping out into the fiery evening ("我出站后 第一眼燃烧着的天空")
            draw_traveler(cr, gcr, 620.0, hy + 18.0, 1.65, 1.0, t * 2.2, 0.35, -0.45, 0.0, 0.65,
                          1.0, 0.56, 0.24, 0.92 * a, a);

            // Foreground Pedestrian Green Light turning crisp emerald at 193.5s ("信绿灯走")
            double green_on = smoothstep(193.2f, 194.2f, static_cast<float>(t));
            cairo_set_source_rgba(cr, 0.02, 0.02, 0.03, 0.98 * a);
            cairo_rectangle(cr, 1520, hy - 250, 12, 260);
            cairo_rectangle(cr, 1500, hy - 295, 52, 84);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, 0.16, 0.98, 0.58, (0.25 + 0.75 * green_on) * a);
            cairo_arc(cr, 1526, hy - 236, 12.0, 0, 2.0 * M_PI);
            cairo_fill(cr);
            cairo_save(gcr);
            cairo_scale(gcr, 0.5, 0.5);
            cairo_set_source_rgba(gcr, 0.16, 0.98, 0.58, (0.25 + 0.75 * green_on) * a);
            cairo_arc(gcr, 1526, hy - 236, 16.0, 0, 2.0 * M_PI);
            cairo_fill(gcr);
            cairo_restore(gcr);
        }

        if (eye_blend > 0.01) {
            // Sculpted upper almond lash-line + naturally clustered tapering eyelashes rooted cleanly on the curve
            double cx = W * 0.5, cy = H * 0.5;
            cairo_set_source_rgba(cr, 0.015, 0.010, 0.016, 0.92 * eye_blend);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

            double p0x = cx - 540.0, p0y = cy + 36.0;
            double p1x = cx,         p1y = cy - 355.0;
            double p2x = cx + 540.0, p2y = cy + 36.0;

            cairo_set_line_width(cr, 4.8);
            cairo_move_to(cr, p0x, p0y);
            cairo_curve_to(cr, (p0x + 2.0 * p1x) / 3.0, (p0y + 2.0 * p1y) / 3.0,
                           (2.0 * p1x + p2x) / 3.0, (2.0 * p1y + p2y) / 3.0,
                           p2x, p2y);
            cairo_stroke(cr);

            cairo_set_source_rgba(cr, 0.015, 0.010, 0.016, 0.36 * eye_blend);
            cairo_set_line_width(cr, 2.0);
            cairo_move_to(cr, cx - 470, cy - 18);
            cairo_curve_to(cr, cx - 220, cy - 245, cx + 220, cy - 245, cx + 470, cy - 18);
            cairo_stroke(cr);

            cairo_set_source_rgba(cr, 0.014, 0.009, 0.015, 0.88 * eye_blend);
            for (int l = 0; l < 32; ++l) {
                double bz = 0.08 + 0.84 * (l / 31.0) + (hash21(l, 4.1f) - 0.5) * 0.018;
                double omt = 1.0 - bz;
                double lx = omt * omt * p0x + 2.0 * omt * bz * p1x + bz * bz * p2x;
                double ly = omt * omt * p0y + 2.0 * omt * bz * p1y + bz * bz * p2y + 1.5;
                double u = (bz - 0.5) * 2.0;
                double sweep = u * 0.64 + (hash21(l, 7.3f) - 0.5) * 0.14;
                double len = (34.0 + 24.0 * hash21(l, 2.9f)) * (1.0 - 0.35 * std::abs(u));
                double ex = lx + std::sin(sweep) * len;
                double ey = ly - std::cos(sweep) * len;
                double mx = lx + std::sin(sweep * 0.52) * len * 0.55;
                double my = ly - std::cos(sweep * 0.52) * len * 0.55;
                cairo_move_to(cr, lx - 2.0, ly);
                cairo_curve_to(cr, mx - 1.1, my, ex, ey, ex, ey);
                cairo_curve_to(cr, mx + 1.1, my, lx + 2.0, ly, lx + 2.0, ly);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
        }
    } else if (act_id == 9) {
        // ACT VIII: Mountains & beacons (203..218) -> City-to-MilkyWay Flight (218..231.8) -> Silent Sea Sail (231.8..239)
        double chorus_t = smoothstep(217.6f, 218.6f, static_cast<float>(t));
        double silence_t = smoothstep(231.5f, 232.3f, static_cast<float>(t));
        double hy = y_act(0.62);

        if (silence_t < 0.98) {
            double a_peak = (1.0 - silence_t);
            double ridge_alpha = a_peak * (1.0 - chorus_t);
            if (ridge_alpha > 0.02) {
                double ridge_ys[3] = {hy - 30, hy + 20, hy + 72};
                double ridge_cols[3][3] = {{0.06, 0.14, 0.26}, {0.035, 0.085, 0.175}, {0.015, 0.032, 0.068}};
                for (int r = 0; r < 3; ++r) {
                    cairo_set_source_rgba(cr, ridge_cols[r][0], ridge_cols[r][1], ridge_cols[r][2], ridge_alpha);
                    cairo_move_to(cr, 0, H - BAR_H);
                    for (int x = 0; x <= W; x += 32) {
                        double mh = (90.0 - r * 14.0) * std::sin(x * 0.0038 + r * 1.9) + (45.0) * std::cos(x * 0.009 - r);
                        cairo_line_to(cr, x, ridge_ys[r] - mh);
                    }
                    cairo_line_to(cr, W, H - BAR_H);
                    cairo_close_path(cr);
                    cairo_fill(cr);
                }

                double peak_xs[4] = {320.0, 740.0, 1210.0, 1610.0};
                for (int p = 0; p < 4; ++p) {
                    double px = peak_xs[p];
                    double py = ridge_ys[1] - (76.0 * std::sin(px * 0.0038 + 1.9) + 45.0 * std::cos(px * 0.009 - 1.0));
                    draw_traveler(cr, gcr, px, py + 4.0, 0.62, (p < 2 ? 1.0 : -1.0), 0.0, 0.0, -0.25, 0.75, 0.9,
                                  0.62, 0.88, 1.0, 0.85 * ridge_alpha, ridge_alpha);
                    cairo_save(gcr);
                    cairo_scale(gcr, 0.5, 0.5);
                    cairo_set_source_rgba(gcr, 1.0, 0.82, 0.44, 0.85 * ridge_alpha);
                    cairo_arc(gcr, px, py - 92.0 * 0.62, 7.5, 0, 2.0 * M_PI);
                    cairo_fill(gcr);
                    cairo_restore(gcr);
                }
            }

            if (chorus_t > 0.02) {
                double ca = chorus_t * a_peak;
                cairo_save(cr);
                draw_cable_bridge(cr, gcr, hy + 18.0, t, 0.85 * ca);

                for (int i = 0; i < 24; ++i) {
                    double bx = (i < 12) ? (18.0 + i * 44.0) : (1360.0 + (i - 12) * 46.0);
                    double bw = 32.0 + hash21(i, 4.4f) * 24.0;
                    double bh = 75.0 + hash21(i, 9.1f) * 195.0;
                    double top_y = hy - bh + 20.0;
                    cairo_set_source_rgba(cr, 0.018, 0.032, 0.072, 0.94 * ca);
                    cairo_rectangle(cr, bx, top_y, bw, bh);
                    if (i % 3 == 0) {
                        cairo_rectangle(cr, bx + bw * 0.25, top_y - 26.0, bw * 0.5, 28.0);
                    }
                    if (i % 5 == 1) {
                        cairo_rectangle(cr, bx + bw * 0.47, top_y - 45.0, 3.5, 47.0);
                    }
                    cairo_fill(cr);

                    int cols = std::max(2, static_cast<int>(bw / 11));
                    for (int wy = 14; wy < bh - 12; wy += 14) {
                        for (int c = 0; c < cols; ++c) {
                            if (hash21(i * 19.0f + c, wy) > 0.48f) {
                                double band_mod = 0.65 + 0.45 * af.bands[(i + c) % 8];
                                cairo_set_source_rgba(cr, 0.98, 0.82, 0.46, 0.62 * ca * band_mod);
                                cairo_rectangle(cr, bx + 6.0 + c * ((bw - 10.0) / cols), top_y + wy, 5.0, 6.0);
                                cairo_fill(cr);
                            }
                        }
                    }
                }

                for (int s = 0; s < 36; ++s) {
                    double sp = fractf(hash21(s, 14.3f) + t * 0.16);
                    double sx = 180.0 + hash21(s, 3.3f) * 1560.0 + sp * 180.0;
                    double sy = y_act(0.65 - sp * 0.62);
                    double al = std::sin(sp * M_PI) * ca;
                    cairo_set_source_rgba(cr, 0.82, 0.92, 1.0, al * 0.92);
                    cairo_arc(cr, sx, sy, 2.4 + 2.0 * af.air, 0, 2.0 * M_PI);
                    cairo_fill(cr);
                }
                cairo_restore(cr);
            }
        }

        if (silence_t > 0.01) {
            // Sudden crystal stillness: single white origami sail gliding over mirror cobalt sea!
            double bx = 960.0 + (t - 232.0) * 18.0;
            double by = y_act(0.60);
            draw_paper_motif(cr, gcr, bx, by, 54.0, std::sin(t * 1.1) * 0.03, 2, 0.0, silence_t, 0.98, 0.98, 1.0);
            // Crisp mirror reflection right beneath hull waterline
            cairo_save(cr);
            cairo_translate(cr, bx, by + 38.0);
            cairo_scale(cr, 1.0, -0.52);
            draw_paper_motif(cr, nullptr, 0, 0, 54.0, -std::sin(t * 1.1) * 0.03, 2, 0.0, silence_t * 0.30, 0.72, 0.88, 1.0);
            cairo_restore(cr);
        }
    } else {
        // ACT X: Epilogue (239.0..265.54) — Vanishing sail fading gently toward dawn horizon
        double fade_sail = 1.0 - smoothstep(240.0f, 255.0f, static_cast<float>(t));
        double prog = std::max(0.0, t - 239.0);
        double bx = 1080.0 + prog * 14.0;
        double by = y_act(0.585);
        double sc = std::max(14.0, 48.0 - prog * 1.8);
        draw_paper_motif(cr, gcr, bx, by, sc, std::sin(t * 0.9) * 0.02, 2, 0.0, fade_sail * 0.9, 1.0, 0.95, 0.86);
    }

    cairo_restore(gcr);
    cairo_restore(cr);
}

}  // namespace mv
