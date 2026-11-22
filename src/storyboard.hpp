#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace mv {

enum class TransType {
    DISSOLVE = 0,     // Smooth linear cross-dissolve
    LIGHT_LEAK = 1,   // Warm optical halation sweep peak at midpoint
    MIST_WIPE = 2,    // Soft horizontal atmospheric sweep
    BLOOM_FLASH = 3   // Musical impact glow transition
};

enum class SceneKind {
    METRO_WINDOW = 0,       // Shot 01: Blurred metro cabin window & bokeh
    RAIN_SKYSCRAPERS_UP,    // Shot 02: Looking up wet neon skyscrapers into storm sky
    PUDDLE_NEON_CLOSEUP,    // Shot 03: Close-up rain puddles, neon ripples, wind slant
    HANDS_TICKET_MACRO,     // Shot 04: Close-up cupped hands holding creased ticket in rain
    CROSSWALK_WAIT_CARS,    // Shot 05: Crosswalk curb silhouette + passing car headlight trails
    CROSSWALK_GREEN_STRIDE, // Shot 06: Low-angle green signal & boots striding through puddles
    ROOM_DESK_LAMP,         // Shot 07: Full-bleed cozy room interior (lamp, mug steam, blueprint)
    ROOM_PILLOW_WINDOW,     // Shot 08: Moonlit pillow & panoramic rainy city window
    VIADUCT_EXPRESS_TRAIN,  // Shot 09: Diagonal elevated viaduct + sleek commuter train
    PLATFORM_WIND_WAIT,     // Shot 10: Open-air platform silhouette leaning into slipstream
    MOUNTAIN_WIND_SWEEP,    // Shot 11: Aerial sweep over misty mountain ridges
    SKYLINE_BLUEPRINT_RISE, // Shot 12: Blueprint wireframes growing into lit metropolis + cranes
    ROOFTOP_CATCH_STAR,     // Shot 13: Rooftop silhouette catching star amid soaring paper birds
    INTERLUDE_APPROACH,     // Shot 14: Blue-hour mirror puddle, past & present selves approaching
    INTERLUDE_HAND_LIGHT,   // Shot 15: Close mirror shot passing golden ember across water
    HARBOR_FIREFLIES_WALK,  // Shot 16: Traveler walking harbor promenade among glowing motes
    PAPER_BIRD_BAY_GLIDE,   // Shot 17: Close flight of origami bird skimming glowing bay water
    BOARDWALK_LANTERNS,     // Shot 18: Coastal boardwalk with swaying festoon bulbs & sea bridge
    GLASSES_TOAST_CLOSE,    // Shot 19: Silhouetted travelers raising amber glasses in toast
    BAY_BRIDGE_PANORAMA,    // Shot 20: Sweeping aerial view of illuminated bay bridge & buoys
    OBSERVATION_BLUEPRINT,  // Shot 21: Observation deck overlooking glowing city & star arcs
    BRIDGE_DECK_SPEED,      // Shot 22: Perspective sea-bridge harp cables & speeding light trails
    SEA_RAIL_DAWN_STATION,  // Shot 23: Coastal dawn rail station beside waking ocean swells
    CAPE_LIGHTHOUSE_GALE,   // Shot 24: Windswept cliff cape, lighthouse beam & silhouette
    SAILBOAT_GALE_ACTION,   // Shot 25: Dynamic white-sailed vessel cutting across sunlit swells
    SUNSET_EXIT_PANORAMA,   // Shot 26: Full-bleed blazing crimson-amber sky past exit pillars
    SUNSET_CROSSWALK_PACK,  // Shot 27: Dusk silhouette with old backpack at emerald green signal
    MACRO_EYE_REFLECTION,   // Shot 28: Painterly full-frame macro eye reflecting burning sky
    MOUNTAIN_BEACONS,       // Shot 29: 5-layer peaks with distant travelers holding beacons
    PASS_REVEALS_SEA,       // Shot 30: Flying through dark mountain pass into vast azure sea
    COSMIC_SKYLINE_SOAR,    // Shot 31: Soaring aerial over harbor skyline into Milky Way
    MEMORY_TIME_MONTAGE,    // Shot 32: Golden ribbons weaving past/present/future silhouettes
    SILENT_COBALT_SAIL,     // Shot 33: Sudden a-cappella stillness, white sail on mirror sea
    DAWN_SEA_HORIZON,       // Shot 34: Slow crane upward over peaceful morning sea & birds
    EPILOGUE_STARS_SEA      // Shot 35: Deep starry dawn sea & logline epilogue
};

struct ShotSpec {
    int id;
    double start;
    double end;
    double trans_dur;
    TransType trans_type;
    SceneKind kind;
    const char* act_code;
    const char* title_zh;
};

inline const std::vector<ShotSpec>& get_storyboard() {
    static const std::vector<ShotSpec> shots = {
        { 0,   0.00,   7.50, 0.00, TransType::DISSOLVE,    SceneKind::METRO_WINDOW,           "PROLOGUE",  "序幕 · 雨夜车窗流光"},
        { 1,   7.50,  15.00, 1.15, TransType::MIST_WIPE,   SceneKind::RAIN_SKYSCRAPERS_UP,    "PROLOGUE",  "序幕 · 出站仰望夜雨"},
        { 2,  15.00,  22.20, 0.90, TransType::DISSOLVE,    SceneKind::PUDDLE_NEON_CLOSEUP,    "ACT I",     "第一幕 · 暴雨霓虹水洼"},
        { 3,  22.20,  29.50, 0.85, TransType::DISSOLVE,    SceneKind::HANDS_TICKET_MACRO,     "ACT I",     "第一幕 · 掌心旧车票"},
        { 4,  29.50,  36.70, 0.90, TransType::DISSOLVE,    SceneKind::CROSSWALK_WAIT_CARS,    "ACT I",     "第一幕 · 雨夜路口立哭"},
        { 5,  36.70,  43.80, 0.65, TransType::BLOOM_FLASH, SceneKind::CROSSWALK_GREEN_STRIDE, "ACT I",     "第一幕 · 绿灯踏过路口"},
        { 6,  43.80,  51.00, 1.05, TransType::LIGHT_LEAK,  SceneKind::ROOM_DESK_LAMP,         "ACT II",    "第二幕 · 斗室暖灯书桌"},
        { 7,  51.00,  59.80, 0.90, TransType::DISSOLVE,    SceneKind::ROOM_PILLOW_WINDOW,     "ACT II",    "第二幕 · 窗畔软枕与夜城"},
        { 8,  59.80,  67.40, 0.95, TransType::MIST_WIPE,   SceneKind::VIADUCT_EXPRESS_TRAIN,  "ACT III",   "第三幕 · 时代高架列车"},
        { 9,  67.40,  74.40, 0.70, TransType::DISSOLVE,    SceneKind::PLATFORM_WIND_WAIT,     "ACT III",   "第三幕 · 清晨站台迎风"},
        {10,  74.40,  81.90, 0.60, TransType::BLOOM_FLASH, SceneKind::MOUNTAIN_WIND_SWEEP,    "ACT IV",    "第四幕 · 好风凭踝翻山"},
        {11,  81.90,  89.20, 0.75, TransType::MIST_WIPE,   SceneKind::SKYLINE_BLUEPRINT_RISE, "ACT IV",    "第四幕 · 蓝图拔起繁城"},
        {12,  89.20,  94.80, 0.65, TransType::LIGHT_LEAK,  SceneKind::ROOFTOP_CATCH_STAR,     "ACT IV",    "第四幕 · 天台徒手摘星"},
        {13,  94.80, 103.00, 1.15, TransType::DISSOLVE,    SceneKind::INTERLUDE_APPROACH,     "INTERLUDE", "间奏 · 蓝调水洼对望"},
        {14, 103.00, 111.30, 0.85, TransType::DISSOLVE,    SceneKind::INTERLUDE_HAND_LIGHT,   "INTERLUDE", "间奏 · 掌心渡光涟漪"},
        {15, 111.30, 118.50, 0.85, TransType::LIGHT_LEAK,  SceneKind::HARBOR_FIREFLIES_WALK,  "ACT V",     "第五幕 · 微光入海湾"},
        {16, 118.50, 125.90, 0.80, TransType::MIST_WIPE,   SceneKind::PAPER_BIRD_BAY_GLIDE,   "ACT V",     "第五幕 · 纸鸟掠波逐光"},
        {17, 125.90, 133.00, 0.95, TransType::DISSOLVE,    SceneKind::BOARDWALK_LANTERNS,     "ACT V",     "第五幕 · 海湾夜色露台"},
        {18, 133.00, 140.10, 0.75, TransType::LIGHT_LEAK,  SceneKind::GLASSES_TOAST_CLOSE,    "ACT V",     "第五幕 · 举杯祝乘风破浪"},
        {19, 140.10, 147.40, 0.85, TransType::DISSOLVE,    SceneKind::BAY_BRIDGE_PANORAMA,    "ACT V",     "第五幕 · 跨海大桥星夜"},
        {20, 147.40, 155.90, 0.85, TransType::DISSOLVE,    SceneKind::OBSERVATION_BLUEPRINT,  "ACT V",     "第五幕 · 理想织就蓝图"},
        {21, 155.90, 163.60, 0.75, TransType::MIST_WIPE,   SceneKind::BRIDGE_DECK_SPEED,      "ACT VI",    "第六幕 · 桥索流光疾驰"},
        {22, 163.60, 170.60, 0.75, TransType::DISSOLVE,    SceneKind::SEA_RAIL_DAWN_STATION,  "ACT VI",    "第六幕 · 海滨清晨车站"},
        {23, 170.60, 178.20, 0.60, TransType::BLOOM_FLASH, SceneKind::CAPE_LIGHTHOUSE_GALE,   "ACT VI",    "第六幕 · 悬崖灯塔狂风"},
        {24, 178.20, 184.90, 0.65, TransType::MIST_WIPE,   SceneKind::SAILBOAT_GALE_ACTION,   "ACT VI",    "第六幕 · 挥旗扬帆跨海"},
        {25, 184.90, 192.20, 0.90, TransType::LIGHT_LEAK,  SceneKind::SUNSET_EXIT_PANORAMA,   "ACT VII",   "第七幕 · 出站燃烧天穹"},
        {26, 192.20, 196.10, 0.65, TransType::DISSOLVE,    SceneKind::SUNSET_CROSSWALK_PACK,  "ACT VII",   "第七幕 · 旧背包与绿灯"},
        {27, 196.10, 203.40, 0.75, TransType::LIGHT_LEAK,  SceneKind::MACRO_EYE_REFLECTION,   "ACT VII",   "第七幕 · 倒映前方的眼眸"},
        {28, 203.40, 211.00, 0.85, TransType::DISSOLVE,    SceneKind::MOUNTAIN_BEACONS,       "ACT VIII",  "第八幕 · 各自山巅微光"},
        {29, 211.00, 218.10, 0.80, TransType::MIST_WIPE,   SceneKind::PASS_REVEALS_SEA,       "ACT VIII",  "第八幕 · 穿山豁然见海"},
        {30, 218.10, 225.40, 0.60, TransType::BLOOM_FLASH, SceneKind::COSMIC_SKYLINE_SOAR,    "ACT VIII",  "第八幕 · 星河繁城升空"},
        {31, 225.40, 232.20, 0.75, TransType::LIGHT_LEAK,  SceneKind::MEMORY_TIME_MONTAGE,    "ACT VIII",  "第八幕 · 过去未来同辉"},
        {32, 232.20, 239.20, 0.75, TransType::DISSOLVE,    SceneKind::SILENT_COBALT_SAIL,     "ACT VIII",  "第八幕 · 静谧蔚蓝行船"},
        {33, 239.20, 248.00, 1.25, TransType::DISSOLVE,    SceneKind::DAWN_SEA_HORIZON,       "EPILOGUE",  "尾声 · 晓风海天远帆"},
        {34, 248.00, 270.00, 1.30, TransType::DISSOLVE,    SceneKind::EPILOGUE_STARS_SEA,     "EPILOGUE",  "尾声 · 跨越山海与星河"}
    };
    return shots;
}

struct ShotBlend {
    const ShotSpec* prev = nullptr;
    const ShotSpec* cur = nullptr;
    float blend = 1.0f;  // 1.0 = fully cur, < 1.0 = transitioning from prev to cur
    float cur_prog = 0.0f;
    float prev_prog = 1.0f;
};

inline ShotBlend evaluate_storyboard(double t) {
    const auto& shots = get_storyboard();
    int idx = static_cast<int>(shots.size()) - 1;
    for (int i = 0; i < static_cast<int>(shots.size()); ++i) {
        if (t < shots[i].end) {
            idx = i;
            break;
        }
    }
    const ShotSpec& cur = shots[idx];
    double dur = std::max(0.1, cur.end - cur.start);
    float cur_p = static_cast<float>(std::clamp((t - cur.start) / dur, 0.0, 1.0));

    ShotBlend out;
    out.cur = &cur;
    out.cur_prog = cur_p;

    if (idx > 0 && cur.trans_dur > 0.01) {
        double dt = t - cur.start;
        if (dt < cur.trans_dur) {
            const ShotSpec& prev = shots[idx - 1];
            double pdur = std::max(0.1, prev.end - prev.start);
            float u = static_cast<float>(std::clamp(dt / cur.trans_dur, 0.0, 1.0));
            // Quintic ease-in-out for buttery-smooth transitions
            out.blend = u * u * u * (u * (u * 6.0f - 15.0f) + 10.0f);
            out.prev = &prev;
            out.prev_prog = static_cast<float>(std::clamp((t - prev.start) / pdur, 0.0, 1.25));
        }
    }
    return out;
}

}  // namespace mv
