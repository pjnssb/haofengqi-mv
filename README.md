# 《好风起 · 周深》纯代码全屏电影级 MV 编译工程

> **一句话高概念剧情（One-Line Premise）**  
> **「一个异乡旅人携一只浸雨的旧背包走出地铁站，在无数个深夜灯影与清晨站台的奔赴里，把一张皱巴巴的进城车票，折成了跨越山海与星河的风帆。」**

本项目是一个**零外部视频/贴图素材、100% 由纯代码程序化渲染**的 **16:9 无黑边全画幅（1920×1080 Full-Bleed）** 叙事 MV 工程。程序直接解析 [`assets/haofengqi.flac`](assets/haofengqi.flac) 中内嵌的**逐字逐句 `<mm:ss.xx>` 歌词元数据**与多频段音频动态特征，通过 **35 组逐句对位分镜** 与 **双场景光学平滑转场**，由 GitHub Actions 自动编译 C++20 渲染引擎输出成片。

完整 35 镜逐句对位分镜脚本见：[STORYLINE.md](STORYLINE.md)

---

## 核心特性

1. **16:9 无黑边全屏视界 + 35 镜强叙事节奏**
   - 取消上下黑边与遮幅墙框，全程 1920×1080 满屏景别变化：掌心车票微距、仰拍暴雨高楼、霓虹水洼特写、斗室暖灯与枕头、高架列车、天台摘星、镜面传光、举杯碰盏、桥索疾驰、海角灯塔、火烧云出站、微距虹膜倒影、穿山见海与无伴奏静谧海面。
   - 内置 **双帧缓冲平滑转场（Cross-Dissolve / Light-Leak / Mist-Wipe / Bloom-Flash）**，画面之间柔和自然过渡。
2. **音频与逐字歌词解析（`scripts/extract_features.py`）**
   - 直接读取 FLAC VorbisComment `LYRICS` 标签，提取演职员表与 44 行逐字精确时间轴（`build/lyrics.json`）。
   - 提取全频响度、低频鼓点（`25–140 Hz`）、人声中频（`280–3200 Hz`）、高频泛音（`4.8–10.2 kHz`）、瞬态冲击（`onset`）及 8 段频谱（`build/features.bin`）。
3. **线性光 HDR 引擎与思源宋体逐字字幕**
   - 线性浮点 HDR 大气着色 + 2-Pass 可分离高斯泛光 + 水平变形镜头眩光 + ACES Filmic 色调映射 + 阴影安全 35mm 胶片微粒。
   - 底部轻柔渐隐雾面护目层 + Pango/`Noto Serif CJK SC` 精确像素测宽逐字点亮。
4. **GitHub Actions 云端自动编译（`.github/workflows/build-mv.yml`）**
   - 推送即自动触发编译，输出 `haofengqi_mv_1080p.mp4` 与分镜总览图至 **Actions Artifacts** 及 **GitHub Releases (`latest-mv`)**。

---

## 本地构建与预览

```bash
sudo apt-get install -y g++ make pkg-config ffmpeg libcairo2-dev libpango1.0-dev fonts-noto-cjk python3-numpy python3-scipy
make previews   # 生成 24 张关键分镜预览图至 build/previews/
make render     # 全片编译并流式编码为 build/haofengqi_mv_1080p.mp4
```
