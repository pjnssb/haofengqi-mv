#include "audio_lyrics.hpp"
#include "cairo_scenes.hpp"
#include "hdr_canvas.hpp"
#include "sky_and_atmosphere.hpp"
#include "storyboard.hpp"
#include "typography.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace mv;

static void write_rgb_png(const HdrCanvas& canvas, const std::string& path) {
    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, W, H);
    cairo_surface_flush(surf);
    uint8_t* dst = cairo_image_surface_get_data(surf);
    int stride = cairo_image_surface_get_stride(surf);
    for (int y = 0; y < H; ++y) {
        uint32_t* row = reinterpret_cast<uint32_t*>(dst + y * stride);
        const uint8_t* src = canvas.rgb_out.data() + y * W * 3;
        for (int x = 0; x < W; ++x) {
            row[x] = (static_cast<uint32_t>(src[x * 3 + 0]) << 16) |
                     (static_cast<uint32_t>(src[x * 3 + 1]) << 8) |
                     (static_cast<uint32_t>(src[x * 3 + 2]));
        }
    }
    cairo_surface_mark_dirty(surf);
    cairo_surface_write_to_png(surf, path.c_str());
    cairo_surface_destroy(surf);
}

static void render_single_frame(HdrCanvas& canvas, int frame_idx, double time_sec,
                                const LyricsData& lrc, const AudioFeatures& feat) {
    AudioFrame af = feat.sample(time_sec);
    ShotBlend sb = evaluate_storyboard(time_sec);
    const bool dual = (sb.prev != nullptr && sb.blend < 0.999f);

    canvas.clear_layers(dual);

    render_atmosphere_shot(canvas.atmo, sb.cur->kind, time_sec, sb.cur_prog, af);
    render_cairo_shot(canvas.scene_cr, canvas.glow_cr, sb.cur->kind, time_sec, sb.cur_prog, af);

    if (dual) {
        render_atmosphere_shot(canvas.atmo_prev, sb.prev->kind, time_sec, sb.prev_prog, af);
        render_cairo_shot(canvas.scene_prev_cr, canvas.glow_prev_cr, sb.prev->kind, time_sec, sb.prev_prog, af);
    }

    render_typography(canvas, time_sec, lrc, af, sb);

    float total_dur = static_cast<float>(lrc.meta.duration > 1.0 ? lrc.meta.duration : 265.54);
    float fade = smoothstep(0.0f, 1.35f, static_cast<float>(time_sec)) *
                 (1.0f - smoothstep(total_dur - 2.4f, total_dur - 0.1f, static_cast<float>(time_sec)));

    float exposure = 1.12f + 0.14f * af.all + 0.08f * af.onset;
    if (sb.cur->kind == SceneKind::MACRO_EYE_REFLECTION && time_sec > 198.5) {
        exposure += 0.18f * smoothstep(198.5f, 200.2f, static_cast<float>(time_sec)) *
                    (1.0f - smoothstep(200.5f, 202.8f, static_cast<float>(time_sec)));
    }
    if (sb.cur->kind == SceneKind::SILENT_COBALT_SAIL || sb.cur->kind == SceneKind::EPILOGUE_STARS_SEA) {
        exposure = 1.06f;
    }

    float flare = 0.25f + 0.45f * af.bass + 0.30f * af.onset;
    if (sb.cur->kind == SceneKind::METRO_WINDOW || sb.cur->kind == SceneKind::INTERLUDE_APPROACH ||
        sb.cur->kind == SceneKind::INTERLUDE_HAND_LIGHT || sb.cur->kind == SceneKind::EPILOGUE_STARS_SEA) {
        flare *= 0.45f;
    }

    canvas.compose(frame_idx, exposure, 0.012f, flare, fade, sb.blend, sb.cur->trans_type);
}

int main(int argc, char** argv) {
    std::string lyrics_path = "build/lyrics.json";
    std::string features_path = "build/features.bin";
    std::string preview_dir;
    std::string preview_times;
    bool stream_mode = false;
    int start_frame = 0;
    int max_frames = -1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--lyrics" && i + 1 < argc) lyrics_path = argv[++i];
        else if (arg == "--features" && i + 1 < argc) features_path = argv[++i];
        else if (arg == "--stream") stream_mode = true;
        else if (arg == "--start-frame" && i + 1 < argc) start_frame = std::atoi(argv[++i]);
        else if (arg == "--max-frames" && i + 1 < argc) max_frames = std::atoi(argv[++i]);
        else if (arg == "--preview-dir" && i + 1 < argc) preview_dir = argv[++i];
        else if (arg == "--times" && i + 1 < argc) preview_times = argv[++i];
    }

    LyricsData lrc = load_lyrics_json(lyrics_path);
    AudioFeatures feat = load_features_bin(features_path);
    int fps = static_cast<int>(feat.fps > 0 ? feat.fps : 30);
    int total_frames = static_cast<int>(std::ceil(lrc.meta.duration * fps));
    if (feat.n_frames > 0) total_frames = std::min(total_frames, static_cast<int>(feat.n_frames));

    HdrCanvas canvas;

    if (!preview_dir.empty()) {
        std::filesystem::create_directories(preview_dir);
        std::vector<double> times;
        if (preview_times.empty()) {
            times = {3.0, 11.0, 19.6, 25.5, 39.6, 47.5, 55.5, 63.0, 86.0, 91.8, 106.5, 122.0, 136.5, 151.5, 160.0, 167.0, 174.5, 181.5, 190.0, 200.0, 207.5, 222.0, 228.5, 235.5};
        } else {
            std::stringstream ss(preview_times);
            std::string item;
            while (std::getline(ss, item, ',')) {
                if (!item.empty()) times.push_back(std::stod(item));
            }
        }
        for (double ts : times) {
            int fidx = std::clamp(static_cast<int>(std::lround(ts * fps)), 0, total_frames - 1);
            render_single_frame(canvas, fidx, ts, lrc, feat);
            char name[128];
            std::snprintf(name, sizeof(name), "frame_%06.2fs.png", ts);
            std::string out_path = preview_dir + "/" + name;
            write_rgb_png(canvas, out_path);
            std::fprintf(stderr, "Wrote preview %s\n", out_path.c_str());
        }
        return 0;
    }

    if (stream_mode) {
        int end_frame = (max_frames > 0) ? std::min(total_frames, start_frame + max_frames) : total_frames;
        auto t0_wall = std::chrono::steady_clock::now();
        for (int f = start_frame; f < end_frame; ++f) {
            double t = static_cast<double>(f) / static_cast<double>(fps);
            render_single_frame(canvas, f, t, lrc, feat);
            std::fwrite(canvas.rgb_out.data(), 1, canvas.rgb_out.size(), stdout);
            if ((f - start_frame) % 150 == 0 || f == end_frame - 1) {
                auto now = std::chrono::steady_clock::now();
                double sec = std::chrono::duration<double>(now - t0_wall).count();
                double done = static_cast<double>(f - start_frame + 1);
                double rate = done / std::max(0.001, sec);
                double eta = (end_frame - f - 1) / std::max(0.001, rate);
                std::fprintf(stderr, "[mv_render] frame %d/%d (%.1f%%) | %.2f fps | elapsed %.1fs | ETA %.1fs\n",
                             f + 1, end_frame, 100.0 * done / (end_frame - start_frame), rate, sec, eta);
            }
        }
        std::fflush(stdout);
        return 0;
    }

    std::fprintf(stderr, "Usage: mv_render --stream | --preview-dir <dir> [--times t1,t2,...]\n");
    return 1;
}
