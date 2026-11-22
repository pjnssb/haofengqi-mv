# 《好风起 · 周深》纯代码电影级 MV 编译工程

> **一句话高概念剧情（One-Line Premise）**  
> **「一个异乡旅人携一只浸雨的旧背包走出地铁站，在无数个深夜灯影与清晨站台的奔赴里，把一张皱巴巴的进城车票，折成了跨越山海与星河的风帆。」**

本项目是一个**零外部视频/贴图素材、100% 由纯代码程序化渲染**的 1080p（`2.39:1` CinemaScope 变形宽银幕）叙事 MV 工程。程序直接解析 [`assets/haofengqi.flac`](assets/haofengqi.flac) 中内嵌的**逐字逐句 `<mm:ss.xx>` 歌词元数据**与多频段音频包络/瞬态特征，并通过 GitHub Actions 自动编译 C++20 渲染引擎输出电影质感 MP4 成片。

完整八幕电影分镜与色彩剧本见：[STORYLINE.md](STORYLINE.md)

---

## 技术架构

1. **音频与逐字歌词解析（`scripts/extract_features.py`）**
   - 直接读取 FLAC VorbisComment `LYRICS` 标签，提取演职员表与 44 行逐字精确时间轴（`build/lyrics.json`）。
   - 使用 SciPy 巴特沃斯分频滤波 + STFT 提取全频响度、低频鼓点（`25–140 Hz`）、人声中频（`280–3200 Hz`）、高频泛音（`4.8–10.2 kHz`）、瞬态冲击包络（`onset`）及 8 段对数频谱（`build/features.bin`）。
2. **线性光 HDR 大气与光学引擎（`src/hdr_canvas.hpp` & `src/sky_and_atmosphere.hpp`）**
   - 960×540 线性浮点 HDR 环境着色的 **11 幕动态天光水影**：雨夜霓虹湿滑沥青同心波纹反射、凌晨台灯钨丝光晕、06:15 晨雾丁达尔光束、蓝调天台镜面水洼、海湾夜景波光、跨海长风浪涌高光路径、熔金火烧云、**微距人眼虹膜纤维与角膜天空倒影**、银河星云与无伴奏静谧蔚蓝海面。
   - 2-Pass 可分离高斯光学泛光（Bloom）、水平变形镜头眩光（Anamorphic Flare）、**ACES Filmic 色调映射** 与 35mm 胶片微粒抖动（消除 8-bit 色彩断层）。
3. **矢量场景与逐字衬线排版（`src/cairo_scenes.hpp` & `src/typography.hpp`）**
   - 1920×1080 全矢量人物剪影（含步行步态、风衣摆动、旧背包飘带与逆光轮廓光）、红绿灯切换、桌灯热杯蒸汽、高架通勤列车、起升塔吊与蓝图摩天楼、海湾跨海斜拉桥与举杯剪影、乘风帆船、微距眼睫及群山信标。
   - 基于 **Pango + `Noto Serif CJK SC`（思源宋体）** 的字符级精确像素测宽，实现丝滑逐字点亮歌词与开场/尾声题词。
4. **GitHub Actions 云端自动编译（`.github/workflows/build-mv.yml`）**
   - 推送即自动触发编译与渲染，成片自动上传至 **Actions Artifacts** 与 **GitHub Releases (`latest-mv`)**。

---

## 本地构建与预览

```bash
# 1. 安装依赖（Ubuntu / Debian）
sudo apt-get install -y g++ make pkg-config ffmpeg libcairo2-dev libpango1.0-dev fonts-noto-cjk python3-numpy python3-scipy

# 2. 渲染 16 幕关键帧预览图至 build/previews/
make previews

# 3. 完整编译并流式编码 1080p MP4（输出 build/haofengqi_mv_1080p.mp4）
make render
```
