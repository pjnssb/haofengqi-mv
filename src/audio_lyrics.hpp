#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mv {

struct WordToken {
    double t0 = 0.0;
    double t1 = 0.0;
    std::string text;
};

struct LyricLine {
    double start = 0.0;
    double end = 0.0;
    std::string text;
    std::vector<WordToken> tokens;
};

struct SongMeta {
    std::string title = "好风起";
    std::string artist = "周深";
    std::string album = "好风起";
    double duration = 265.54;
    int fps = 30;
    int n_frames = 0;
};

struct LyricsData {
    SongMeta meta;
    std::vector<LyricLine> credits;
    std::vector<LyricLine> lyrics;
};

struct AudioFrame {
    float all = 0.0f;
    float bass = 0.0f;
    float mid = 0.0f;
    float air = 0.0f;
    float onset = 0.0f;
    float bands[8]{};
};

struct AudioFeatures {
    uint32_t fps = 30;
    uint32_t n_frames = 0;
    float duration = 265.54f;
    std::vector<AudioFrame> frames;

    AudioFrame sample(double t) const {
        if (frames.empty()) return {};
        double f = t * static_cast<double>(fps);
        if (f <= 0.0) return frames.front();
        if (f >= static_cast<double>(frames.size() - 1)) return frames.back();
        size_t i0 = static_cast<size_t>(f);
        size_t i1 = std::min(i0 + 1, frames.size() - 1);
        float a = static_cast<float>(f - static_cast<double>(i0));
        const auto& x = frames[i0];
        const auto& y = frames[i1];
        AudioFrame r;
        r.all = x.all + (y.all - x.all) * a;
        r.bass = x.bass + (y.bass - x.bass) * a;
        r.mid = x.mid + (y.mid - x.mid) * a;
        r.air = x.air + (y.air - x.air) * a;
        r.onset = x.onset + (y.onset - x.onset) * a;
        for (int k = 0; k < 8; ++k) {
            r.bands[k] = x.bands[k] + (y.bands[k] - x.bands[k]) * a;
        }
        return r;
    }
};

struct ActInfo {
    int id;
    double start;
    double end;
    const char* code;
    const char* title_zh;
    const char* title_en;
};

inline const std::vector<ActInfo>& get_acts() {
    static const std::vector<ActInfo> acts = {
        {0,   0.00,  14.80, "PROLOGUE",  "序幕 · 出站前的雨声",               "RAIN BEFORE THE EXIT"},
        {1,  14.80,  43.20, "ACT I",     "第一幕 · 雨巷立哭与路口绿灯",       "THE RAIN CROSSWALK"},
        {2,  43.20,  59.20, "ACT II",    "第二幕 · 斗室台灯与不心忧的枕头",   "TUNGSTEN & QUIET SLEEP"},
        {3,  59.20,  74.00, "ACT III",   "第三幕 · 清晨站台与时代列车",       "THE 06:15 VIADUCT"},
        {4,  74.00,  94.50, "ACT IV",    "第四幕 · 无名闯繁城，徒手摘星辰",   "TOWERS & STARLIGHT"},
        {5,  94.50, 111.00, "INTERLUDE", "间奏 · 旧日微光的水面",             "REFLECTION OF FORMER SELF"},
        {6, 111.00, 155.40, "ACT V",     "第五幕 · 城中诸位，举杯祝破浪",     "HARBOR LANTERNS & TOAST"},
        {7, 155.40, 184.40, "ACT VI",    "第六幕 · 挥旗扬帆，跨海行船",       "SAILS INTO THE GALE"},
        {8, 184.40, 203.00, "ACT VII",   "第七幕 · 燃烧着的天空与眼眸",       "BURNING SKY & THE IRIS"},
        {9, 203.00, 239.00, "ACT VIII",  "第八幕 · 各自的山、星河与蔚蓝",     "PEAKS, MILKY WAY & THE SEA"},
        {10,239.00, 270.00, "EPILOGUE",  "尾声 · 跨越山海的风帆",             "BEYOND THE HORIZON"}
    };
    return acts;
}

inline int find_act(double t) {
    const auto& acts = get_acts();
    for (int i = static_cast<int>(acts.size()) - 1; i >= 0; --i) {
        if (t >= acts[i].start) return i;
    }
    return 0;
}

// Minimal fast JSON parser for build/lyrics.json
namespace detail {

struct JsonVal {
    enum Type { JNULL, JBOOL, JNUM, JSTR, JARR, JOBJ } type = JNULL;
    double num = 0.0;
    std::string str;
    std::vector<JsonVal> arr;
    std::vector<std::pair<std::string, JsonVal>> obj;

    const JsonVal* get(const std::string& k) const {
        if (type != JOBJ) return nullptr;
        for (const auto& kv : obj) {
            if (kv.first == k) return &kv.second;
        }
        return nullptr;
    }
};

class JsonParser {
    std::string s_;
    size_t p_ = 0;

    void skip_ws() {
        while (p_ < s_.size() && (s_[p_] == ' ' || s_[p_] == '\t' || s_[p_] == '\n' || s_[p_] == '\r')) ++p_;
    }

    static void append_utf8(std::string& out, uint32_t cp) {
        if (cp <= 0x7F) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    std::string parse_string() {
        if (s_[p_] != '"') throw std::runtime_error("Expected '\"'");
        ++p_;
        std::string out;
        while (p_ < s_.size() && s_[p_] != '"') {
            if (s_[p_] == '\\') {
                ++p_;
                if (p_ >= s_.size()) break;
                char c = s_[p_++];
                switch (c) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        if (p_ + 4 <= s_.size()) {
                            uint32_t cp = std::stoul(s_.substr(p_, 4), nullptr, 16);
                            p_ += 4;
                            append_utf8(out, cp);
                        }
                        break;
                    }
                    default: out.push_back(c); break;
                }
            } else {
                out.push_back(s_[p_++]);
            }
        }
        if (p_ < s_.size() && s_[p_] == '"') ++p_;
        return out;
    }

public:
    explicit JsonParser(std::string s) : s_(std::move(s)) {}

    JsonVal parse() {
        skip_ws();
        if (p_ >= s_.size()) throw std::runtime_error("Unexpected EOF");
        char c = s_[p_];
        if (c == '{') {
            ++p_;
            JsonVal v;
            v.type = JsonVal::JOBJ;
            skip_ws();
            if (p_ < s_.size() && s_[p_] == '}') { ++p_; return v; }
            while (p_ < s_.size()) {
                skip_ws();
                std::string k = parse_string();
                skip_ws();
                if (p_ < s_.size() && s_[p_] == ':') ++p_;
                v.obj.emplace_back(std::move(k), parse());
                skip_ws();
                if (p_ < s_.size() && s_[p_] == ',') { ++p_; continue; }
                if (p_ < s_.size() && s_[p_] == '}') { ++p_; break; }
                break;
            }
            return v;
        } else if (c == '[') {
            ++p_;
            JsonVal v;
            v.type = JsonVal::JARR;
            skip_ws();
            if (p_ < s_.size() && s_[p_] == ']') { ++p_; return v; }
            while (p_ < s_.size()) {
                v.arr.push_back(parse());
                skip_ws();
                if (p_ < s_.size() && s_[p_] == ',') { ++p_; continue; }
                if (p_ < s_.size() && s_[p_] == ']') { ++p_; break; }
                break;
            }
            return v;
        } else if (c == '"') {
            JsonVal v;
            v.type = JsonVal::JSTR;
            v.str = parse_string();
            return v;
        } else if (c == 't' || c == 'f') {
            JsonVal v;
            v.type = JsonVal::JBOOL;
            if (s_.compare(p_, 4, "true") == 0) { v.num = 1; p_ += 4; }
            else { v.num = 0; p_ += 5; }
            return v;
        } else if (c == 'n') {
            p_ += 4;
            return {};
        } else {
            size_t start = p_;
            while (p_ < s_.size() && (std::isdigit(static_cast<unsigned char>(s_[p_])) ||
                   s_[p_] == '-' || s_[p_] == '+' || s_[p_] == '.' || s_[p_] == 'e' || s_[p_] == 'E')) {
                ++p_;
            }
            JsonVal v;
            v.type = JsonVal::JNUM;
            v.num = std::stod(s_.substr(start, p_ - start));
            return v;
        }
    }
};

}  // namespace detail

inline LyricsData load_lyrics_json(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open lyrics JSON: " + path);
    std::ostringstream ss;
    ss << ifs.rdbuf();
    detail::JsonParser parser(ss.str());
    detail::JsonVal root = parser.parse();

    LyricsData out;
    if (auto* m = root.get("meta")) {
        if (auto* v = m->get("title")) out.meta.title = v->str;
        if (auto* v = m->get("artist")) out.meta.artist = v->str;
        if (auto* v = m->get("album")) out.meta.album = v->str;
        if (auto* v = m->get("duration")) out.meta.duration = v->num;
        if (auto* v = m->get("fps")) out.meta.fps = static_cast<int>(v->num);
        if (auto* v = m->get("n_frames")) out.meta.n_frames = static_cast<int>(v->num);
    }

    auto read_lines = [](const detail::JsonVal* arr, std::vector<LyricLine>& dst) {
        if (!arr || arr->type != detail::JsonVal::JARR) return;
        for (const auto& item : arr->arr) {
            LyricLine ln;
            if (auto* v = item.get("start")) ln.start = v->num;
            if (auto* v = item.get("end")) ln.end = v->num;
            if (auto* v = item.get("text")) ln.text = v->str;
            if (auto* toks = item.get("tokens")) {
                for (const auto& tk : toks->arr) {
                    WordToken wt;
                    if (auto* v = tk.get("t")) wt.t0 = v->num;
                    if (auto* v = tk.get("t_end")) wt.t1 = v->num;
                    if (auto* v = tk.get("text")) wt.text = v->str;
                    ln.tokens.push_back(std::move(wt));
                }
            }
            dst.push_back(std::move(ln));
        }
    };

    read_lines(root.get("credits"), out.credits);
    read_lines(root.get("lyrics"), out.lyrics);
    return out;
}

inline AudioFeatures load_features_bin(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open features bin: " + path);
    char magic[4]{};
    ifs.read(magic, 4);
    if (std::memcmp(magic, "HFQF", 4) != 0) {
        throw std::runtime_error("Invalid features binary magic");
    }
    AudioFeatures feat;
    ifs.read(reinterpret_cast<char*>(&feat.fps), sizeof(uint32_t));
    ifs.read(reinterpret_cast<char*>(&feat.n_frames), sizeof(uint32_t));
    ifs.read(reinterpret_cast<char*>(&feat.duration), sizeof(float));
    feat.frames.resize(feat.n_frames);
    for (uint32_t i = 0; i < feat.n_frames; ++i) {
        float buf[13]{};
        ifs.read(reinterpret_cast<char*>(buf), sizeof(buf));
        auto& f = feat.frames[i];
        f.all = buf[0];
        f.bass = buf[1];
        f.mid = buf[2];
        f.air = buf[3];
        f.onset = buf[4];
        for (int k = 0; k < 8; ++k) f.bands[k] = buf[5 + k];
    }
    return feat;
}

}  // namespace mv
