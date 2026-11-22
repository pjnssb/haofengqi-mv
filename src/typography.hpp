#pragma once

#include "audio_lyrics.hpp"
#include "hdr_canvas.hpp"
#include "storyboard.hpp"
#include <algorithm>
#include <cairo/cairo.h>
#include <cmath>
#include <cstdio>
#include <pango/pangocairo.h>
#include <string>
#include <vector>

namespace mv {

inline void set_pango_font(PangoLayout* layout, const char* family, int size_pt, int letter_spacing_units, bool bold = false) {
    PangoFontDescription* desc = pango_font_description_new();
    pango_font_description_set_family(desc, family);
    pango_font_description_set_size(desc, size_pt * PANGO_SCALE);
    pango_font_description_set_weight(desc, bold ? PANGO_WEIGHT_SEMIBOLD : PANGO_WEIGHT_NORMAL);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    if (letter_spacing_units != 0) {
        PangoAttrList* attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_letter_spacing_new(letter_spacing_units));
        pango_layout_set_attributes(layout, attrs);
        pango_attr_list_unref(attrs);
    }
}

inline void draw_centered_text(cairo_t* cr, const std::string& text, double cx, double cy,
                               const char* family, int size_pt, int letter_spacing,
                               double r, double g, double b, double a, bool shadow = true) {
    if (a <= 0.005 || text.empty()) return;
    PangoLayout* layout = pango_cairo_create_layout(cr);
    set_pango_font(layout, family, size_pt, letter_spacing, false);
    pango_layout_set_text(layout, text.c_str(), static_cast<int>(text.size()));
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

    int tw = 0, th = 0;
    pango_layout_get_pixel_size(layout, &tw, &th);
    double x0 = cx - tw * 0.5, y0 = cy - th * 0.5;
    if (shadow) {
        cairo_save(cr);
        cairo_move_to(cr, x0 + 1.5, y0 + 2.0);
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.55 * a);
        pango_cairo_show_layout(cr, layout);
        cairo_restore(cr);
    }
    cairo_save(cr);
    cairo_move_to(cr, x0, y0);
    cairo_set_source_rgba(cr, r, g, b, a);
    pango_cairo_show_layout(cr, layout);
    cairo_restore(cr);
    g_object_unref(layout);
}

inline void render_typography(HdrCanvas& canvas, double time_sec, const LyricsData& lrc,
                              const AudioFrame& af, const ShotBlend& sb) {
    cairo_t* cr = canvas.overlay_cr;
    const double t = time_sec;
    const double total_dur = lrc.meta.duration > 1.0 ? lrc.meta.duration : 265.54;
    const ShotSpec* cur = sb.cur;

    // 1. Subtle floating top-corner chapter marginalia (no black bar!)
    {
        double age = t - cur->start;
        double rem = cur->end - t;
        double ha = 0.62 * smoothstep(0.0f, 0.7f, static_cast<float>(age)) *
                    smoothstep(0.0f, 0.6f, static_cast<float>(rem));
        ha = std::max(0.38, ha);

        std::string left_label = std::string(cur->act_code) + "  ·  " + cur->title_zh;
        PangoLayout* l_lay = pango_cairo_create_layout(cr);
        set_pango_font(l_lay, "Noto Serif CJK SC", 13, 1800, false);
        pango_layout_set_text(l_lay, left_label.c_str(), static_cast<int>(left_label.size()));
        cairo_save(cr);
        cairo_move_to(cr, 57.5, 41.5);
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.55 * ha);
        pango_cairo_show_layout(cr, l_lay);
        cairo_move_to(cr, 56.0, 40.0);
        cairo_set_source_rgba(cr, 0.92, 0.94, 0.98, ha);
        pango_cairo_show_layout(cr, l_lay);
        cairo_restore(cr);
        g_object_unref(l_lay);

        int cur_s = static_cast<int>(std::max(0.0, t));
        int tot_s = static_cast<int>(total_dur);
        char tc_buf[128];
        std::snprintf(tc_buf, sizeof(tc_buf), "好风起 · 周深    %02d:%02d / %02d:%02d",
                      cur_s / 60, cur_s % 60, tot_s / 60, tot_s % 60);
        PangoLayout* r_lay = pango_cairo_create_layout(cr);
        set_pango_font(r_lay, "Noto Serif CJK SC", 12, 1600, false);
        pango_layout_set_text(r_lay, tc_buf, -1);
        int rw = 0, rh = 0;
        pango_layout_get_pixel_size(r_lay, &rw, &rh);
        cairo_save(cr);
        cairo_move_to(cr, W - 56.0 - rw + 1.5, 41.5);
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.48);
        pango_cairo_show_layout(cr, r_lay);
        cairo_move_to(cr, W - 56.0 - rw, 40.0);
        cairo_set_source_rgba(cr, 0.86, 0.89, 0.94, 0.58);
        pango_cairo_show_layout(cr, r_lay);
        cairo_restore(cr);
        g_object_unref(r_lay);
    }

    // 2. Opening Title & Grouped Intro Credits (00:00 .. 00:14.6)
    if (t < 14.8) {
        double title_a = smoothstep(0.6f, 2.6f, static_cast<float>(t)) *
                         (1.0 - smoothstep(12.2f, 14.4f, static_cast<float>(t)));
        if (title_a > 0.01) {
            draw_centered_text(cr, "好  风  起", W * 0.5, H * 0.42, "Noto Serif CJK SC", 62, 13000,
                               0.99, 0.97, 0.93, title_a * 0.96);
            cairo_set_source_rgba(cr, 0.96, 0.82, 0.54, title_a * 0.65);
            cairo_rectangle(cr, W * 0.5 - 68.0, H * 0.495, 136.0, 1.5);
            cairo_fill(cr);
            draw_centered_text(cr, "周   深", W * 0.5, H * 0.54, "Noto Serif CJK SC", 21, 9500,
                               0.90, 0.92, 0.96, title_a * 0.84);
        }

        struct CreditCard { double t0, t1; const char* line1; const char* line2; };
        CreditCard cards[] = {
            {0.4,  4.0,  "词：唐恬   ·   曲 / 编曲 / 制作人：钱雷", "人声录音 / 配唱：徐威 @52Hz Studio"},
            {4.0,  8.0,  "吉他：高飞   ·   Bass：赵浩成   ·   弦乐编写：胡静成", "弦乐：国际首席爱乐乐团   ·   混音 / 母带：周天澈 @Studio 21A"},
            {8.0,  14.2, "出品：中共深圳市南山区委员会 / 深圳市南山区人民政府", "音乐发行：QQ音乐·新鲜制造工作室   ·   特别鸣谢：周深工作室"}
        };
        for (const auto& c : cards) {
            if (t >= c.t0 && t <= c.t1) {
                double a = smoothstep(static_cast<float>(c.t0), static_cast<float>(c.t0 + 0.7), static_cast<float>(t)) *
                           (1.0 - smoothstep(static_cast<float>(c.t1 - 0.7), static_cast<float>(c.t1), static_cast<float>(t)));
                draw_centered_text(cr, c.line1, W * 0.5, 920.0, "Noto Serif CJK SC", 13, 2200,
                                   0.92, 0.94, 0.98, a * 0.86);
                draw_centered_text(cr, c.line2, W * 0.5, 956.0, "Noto Serif CJK SC", 12, 1800,
                                   0.78, 0.83, 0.90, a * 0.70);
            }
        }
    }

    // 3. Interlude Poetic Whisper (01:36.5 .. 01:49.5)
    if (t >= 96.5 && t <= 109.5) {
        double a = smoothstep(96.5f, 98.2f, static_cast<float>(t)) *
                   (1.0 - smoothstep(107.5f, 109.5f, static_cast<float>(t)));
        draw_centered_text(cr, "所有远行的人，都曾被旧日微光照亮。", W * 0.5, 195.0,
                           "Noto Serif CJK SC", 19, 4400, 0.96, 0.91, 0.80, a * 0.78);
    }

    // 4. Epilogue One-Line Story Logline & Credits (04:01.0 .. 04:24.0)
    if (t >= 241.0 && t <= 264.5) {
        double a = smoothstep(241.0f, 243.5f, static_cast<float>(t)) *
                   (1.0 - smoothstep(261.2f, 264.2f, static_cast<float>(t)));
        cairo_save(cr);
        cairo_translate(cr, W * 0.5, 405.0);
        cairo_scale(cr, 2.5, 0.85);
        cairo_pattern_t* ep = cairo_pattern_create_radial(0.0, 0.0, 40.0, 0.0, 0.0, 320.0);
        cairo_pattern_add_color_stop_rgba(ep, 0.0, 0.010, 0.018, 0.038, 0.42 * a);
        cairo_pattern_add_color_stop_rgba(ep, 0.55, 0.010, 0.018, 0.038, 0.14 * a);
        cairo_pattern_add_color_stop_rgba(ep, 1.0, 0.010, 0.018, 0.038, 0.0);
        cairo_set_source(cr, ep);
        cairo_arc(cr, 0.0, 0.0, 320.0, 0, 2.0 * M_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(ep);
        cairo_restore(cr);

        draw_centered_text(cr, "一个异乡旅人携一只浸雨的旧背包走出地铁站，", W * 0.5, 345.0,
                           "Noto Serif CJK SC", 21, 3200, 0.97, 0.95, 0.90, a * 0.95);
        draw_centered_text(cr, "在无数个深夜灯影与清晨站台的奔赴里，把一张皱巴巴的进城车票，折成了跨越山海与星河的风帆。", W * 0.5, 394.0,
                           "Noto Serif CJK SC", 21, 3200, 0.97, 0.95, 0.90, a * 0.95);
        cairo_set_source_rgba(cr, 0.94, 0.82, 0.54, a * 0.58);
        cairo_rectangle(cr, W * 0.5 - 54.0, 452.0, 108.0, 1.2);
        cairo_fill(cr);
        draw_centered_text(cr, "《好风起》   ·   演唱：周深   ·   词：唐恬   ·   曲：钱雷", W * 0.5, 495.0,
                           "Noto Serif CJK SC", 14, 2800, 0.88, 0.91, 0.96, a * 0.84);
    }

    // 5. Full-Bleed Word-by-Word Illuminated Karaoke Lyrics near Bottom Edge (y ≈ 968)
    const LyricLine* active_line = nullptr;
    for (const auto& ln : lrc.lyrics) {
        if (t >= ln.start - 0.25 && t <= ln.end + 0.35) {
            active_line = &ln;
            break;
        }
    }

    if (active_line && !active_line->tokens.empty()) {
        std::string joined;
        std::vector<std::pair<int, int>> byte_spans;
        byte_spans.reserve(active_line->tokens.size());
        for (const auto& tk : active_line->tokens) {
            int s0 = static_cast<int>(joined.size());
            joined += tk.text;
            int s1 = static_cast<int>(joined.size());
            byte_spans.emplace_back(s0, s1);
        }
        while (!joined.empty() && (joined.back() == ' ' || joined.back() == '\t')) joined.pop_back();
        for (auto& sp : byte_spans) {
            sp.first = std::min(sp.first, static_cast<int>(joined.size()));
            sp.second = std::min(sp.second, static_cast<int>(joined.size()));
        }

        double line_a = smoothstep(static_cast<float>(active_line->start - 0.25),
                                   static_cast<float>(active_line->start + 0.05), static_cast<float>(t)) *
                        (1.0 - smoothstep(static_cast<float>(active_line->end - 0.05),
                                          static_cast<float>(active_line->end + 0.32), static_cast<float>(t)));

        if (line_a > 0.01 && !joined.empty()) {
            // Whisper-soft translucent gradient scrim along bottom 175px (NOT a hard black bar!)
            cairo_save(cr);
            cairo_pattern_t* bot_grad = cairo_pattern_create_linear(0, H - 175.0, 0, H);
            cairo_pattern_add_color_stop_rgba(bot_grad, 0.0, 0.015, 0.024, 0.045, 0.0);
            cairo_pattern_add_color_stop_rgba(bot_grad, 1.0, 0.015, 0.024, 0.045, 0.46 * line_a);
            cairo_set_source(cr, bot_grad);
            cairo_rectangle(cr, 0, H - 175.0, W, 175.0);
            cairo_fill(cr);
            cairo_pattern_destroy(bot_grad);
            cairo_restore(cr);

            PangoLayout* lay = pango_cairo_create_layout(cr);
            set_pango_font(lay, "Noto Serif CJK SC", 28, 3600, false);
            pango_layout_set_text(lay, joined.c_str(), static_cast<int>(joined.size()));

            int tw = 0, th = 0;
            pango_layout_get_pixel_size(lay, &tw, &th);
            double lx = (W - tw) * 0.5;
            double ly = H - 86.0 - th * 0.5;

            double sweep_x = lx - 4.0, cur_char_cx = lx;
            bool sweeping = false;
            for (size_t k = 0; k < active_line->tokens.size(); ++k) {
                const auto& tk = active_line->tokens[k];
                int b0 = byte_spans[k].first, b1 = byte_spans[k].second;
                if (b1 <= b0) {
                    if (t >= tk.t0) sweep_x = std::max(sweep_x, lx);
                    continue;
                }
                PangoRectangle r0{}, r1{};
                pango_layout_index_to_pos(lay, b0, &r0);
                int prev_b = b1 - 1;
                while (prev_b > b0 && (static_cast<unsigned char>(joined[prev_b]) & 0xC0) == 0x80) --prev_b;
                pango_layout_index_to_pos(lay, prev_b, &r1);

                double x0 = lx + static_cast<double>(r0.x) / PANGO_SCALE;
                double x1 = lx + static_cast<double>(r1.x + r1.width) / PANGO_SCALE;
                if (x1 < x0) std::swap(x0, x1);

                if (t >= tk.t1) {
                    sweep_x = std::max(sweep_x, x1 + 2.0);
                    cur_char_cx = x1;
                } else if (t >= tk.t0) {
                    double frac = clamp01(static_cast<float>((t - tk.t0) / std::max(0.06, tk.t1 - tk.t0)));
                    sweep_x = x0 + (x1 - x0) * frac;
                    cur_char_cx = sweep_x;
                    sweeping = true;
                    break;
                } else {
                    break;
                }
            }

            // Drop shadow under full text for guaranteed crisp readability on bright skies/water
            cairo_save(cr);
            cairo_move_to(cr, lx + 1.5, ly + 2.2);
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.62 * line_a);
            pango_cairo_show_layout(cr, lay);
            cairo_restore(cr);

            // Unsung translucent silver base text
            cairo_save(cr);
            cairo_move_to(cr, lx, ly);
            cairo_set_source_rgba(cr, 0.84, 0.88, 0.94, 0.42 * line_a);
            pango_cairo_show_layout(cr, lay);
            cairo_restore(cr);

            // Subtle wide elliptical optical luminescence around active syllable
            if (sweep_x > lx) {
                double halo_x = sweeping ? cur_char_cx : sweep_x;
                cairo_save(cr);
                cairo_translate(cr, halo_x, ly + th * 0.5);
                cairo_scale(cr, 2.2, 0.65);
                cairo_pattern_t* gp = cairo_pattern_create_radial(0.0, 0.0, 2.0, 0.0, 0.0, 56.0);
                cairo_pattern_add_color_stop_rgba(gp, 0.0, 1.0, 0.84, 0.48, (0.12 + 0.07 * af.mid) * line_a);
                cairo_pattern_add_color_stop_rgba(gp, 0.45, 1.0, 0.76, 0.36, 0.045 * line_a);
                cairo_pattern_add_color_stop_rgba(gp, 1.0, 1.0, 0.72, 0.32, 0.0);
                cairo_set_source(cr, gp);
                cairo_arc(cr, 0.0, 0.0, 56.0, 0, 2.0 * M_PI);
                cairo_fill(cr);
                cairo_pattern_destroy(gp);
                cairo_restore(cr);
            }

            // Swept word-by-word illuminated warm ivory lyrics
            if (sweep_x > lx - 2.0) {
                cairo_save(cr);
                cairo_rectangle(cr, lx - 16.0, ly - 10.0, std::max(0.0, sweep_x - lx + 16.0), th + 20.0);
                cairo_clip(cr);
                cairo_move_to(cr, lx, ly);
                cairo_set_source_rgba(cr, 1.0, 0.97, 0.90, 0.98 * line_a);
                pango_cairo_show_layout(cr, lay);
                cairo_restore(cr);
            }

            g_object_unref(lay);
        }
    }

    // Whisper-thin progress line along very bottom pixel
    double prog = clamp01(static_cast<float>(t / total_dur));
    cairo_set_source_rgba(cr, 0.96, 0.84, 0.58, 0.34);
    cairo_rectangle(cr, 0.0, H - 2.0, W * prog, 2.0);
    cairo_fill(cr);
}

}  // namespace mv
