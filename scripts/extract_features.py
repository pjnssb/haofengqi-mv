#!/usr/bin/env python3
"""
Extract embedded word-by-word LRC lyrics and frame-synced audio DSP features
directly from assets/haofengqi.flac for the C++ procedural MV engine.
"""

import argparse
import json
import os
import re
import struct
import subprocess
import tempfile
import numpy as np
from scipy.signal import butter, sosfilt


def parse_flac_vorbis(flac_path: str) -> dict:
    tags = {}
    duration = 0.0
    with open(flac_path, "rb") as f:
        if f.read(4) != b"fLaC":
            raise ValueError(f"Not a valid FLAC file: {flac_path}")
        while True:
            hdr = f.read(4)
            if len(hdr) < 4:
                break
            is_last = (hdr[0] & 0x80) != 0
            btype = hdr[0] & 0x7F
            length = struct.unpack(">I", b"\x00" + hdr[1:4])[0]
            data = f.read(length)
            if btype == 0 and len(data) >= 18:
                val = int.from_bytes(data[10:18], "big")
                sr = (val >> 44) & 0xFFFFF
                total_samples = val & 0xFFFFFFFFF
                if sr > 0:
                    duration = total_samples / float(sr)
            elif btype == 4:
                pos = 0
                vlen = struct.unpack("<I", data[pos : pos + 4])[0]
                pos += 4 + vlen
                count = struct.unpack("<I", data[pos : pos + 4])[0]
                pos += 4
                for _ in range(count):
                    clen = struct.unpack("<I", data[pos : pos + 4])[0]
                    pos += 4
                    entry = data[pos : pos + clen].decode("utf-8", "replace")
                    pos += clen
                    if "=" in entry:
                        k, v = entry.split("=", 1)
                        tags[k.upper()] = v
            if is_last:
                break
    return {"tags": tags, "duration": duration}


def ts_to_sec(mm: str, ss: str) -> float:
    return int(mm) * 60.0 + float(ss)


def parse_word_lrc(lrc_text: str, total_duration: float) -> dict:
    credits = []
    lyrics = []
    raw_lines = [l.strip() for l in lrc_text.splitlines() if l.strip()]

    parsed_lyrics = []
    for line in raw_lines:
        m = re.match(r"^\[(\d+):(\d+\.\d+)\](.*)$", line)
        if not m:
            continue
        line_start = ts_to_sec(m.group(1), m.group(2))
        body = m.group(3)
        tokens_raw = re.findall(r"<(\d+):(\d+\.\d+)>([^<]*)", body)

        chars = []
        for idx, (mm, ss, txt) in enumerate(tokens_raw):
            t0 = ts_to_sec(mm, ss)
            chars.append({"t": round(t0, 3), "text": txt})

        # Compute per-token end times
        for i in range(len(chars)):
            if i + 1 < len(chars):
                t1 = max(chars[i]["t"] + 0.08, chars[i + 1]["t"])
            else:
                t1 = chars[i]["t"] + 0.55
            chars[i]["t_end"] = round(t1, 3)

        full_text = "".join(c["text"] for c in chars).strip()
        if not full_text:
            continue

        item = {
            "start": round(line_start, 3),
            "text": full_text,
            "tokens": chars,
        }
        if line_start < 14.5:
            credits.append(item)
        else:
            parsed_lyrics.append(item)

    # Determine graceful line end times for lyrics
    for i, item in enumerate(parsed_lyrics):
        last_tok_start = item["tokens"][-1]["t"] if item["tokens"] else item["start"]
        natural_end = last_tok_start + 1.65
        if i + 1 < len(parsed_lyrics):
            next_start = parsed_lyrics[i + 1]["start"]
            gap = next_start - last_tok_start
            if gap > 4.0:
                natural_end = last_tok_start + 2.35
            end_t = min(natural_end, next_start - 0.06)
            end_t = max(end_t, min(last_tok_start + 0.25, next_start - 0.02))
        else:
            end_t = min(total_duration - 2.0, last_tok_start + 2.60)
        item["end"] = round(end_t, 3)
        if item["tokens"]:
            item["tokens"][-1]["t_end"] = round(min(end_t, last_tok_start + 0.90), 3)
        lyrics.append(item)

    return {"credits": credits, "lyrics": lyrics}


def band_rms(sig: np.ndarray, sr: int, low: float, high: float, fps: int, n_frames: int) -> np.ndarray:
    sos = butter(4, [low, high], btype="band", fs=sr, output="sos")
    filt = sosfilt(sos, sig)
    hop = sr // fps
    out = np.zeros(n_frames, dtype=np.float32)
    win = max(hop, int(sr * 0.04))
    for i in range(n_frames):
        c = i * hop + hop // 2
        a = max(0, c - win // 2)
        b = min(len(filt), c + win // 2)
        if b > a:
            out[i] = float(np.sqrt(np.mean(filt[a:b] ** 2) + 1e-12))
    return out


def smooth_ema(arr: np.ndarray, attack: float, release: float) -> np.ndarray:
    out = np.zeros_like(arr)
    cur = 0.0
    for i in range(len(arr)):
        v = float(arr[i])
        alpha = attack if v > cur else release
        cur = cur * (1.0 - alpha) + v * alpha
        out[i] = cur
    return out


def extract_audio_features(flac_path: str, fps: int, out_bin: str) -> dict:
    sr = 22050
    with tempfile.NamedTemporaryFile(suffix=".pcm") as tmp:
        subprocess.run(
            ["ffmpeg", "-y", "-v", "error", "-i", flac_path, "-ac", "1", "-ar", str(sr), "-f", "f32le", tmp.name],
            check=True,
        )
        pcm = np.fromfile(tmp.name, dtype=np.float32)

    duration = len(pcm) / float(sr)
    n_frames = int(np.ceil(duration * fps))

    rms_all = band_rms(pcm, sr, 25, 10000, fps, n_frames)
    rms_bass = band_rms(pcm, sr, 25, 140, fps, n_frames)
    rms_mid = band_rms(pcm, sr, 280, 3200, fps, n_frames)
    rms_air = band_rms(pcm, sr, 4800, 10200, fps, n_frames)

    # Normalize robustly against 99th percentile
    def norm99(x):
        p = float(np.percentile(x, 99.0)) + 1e-6
        return np.clip(x / p, 0.0, 1.35)

    n_all = norm99(rms_all)
    n_bass = norm99(rms_bass)
    n_mid = norm99(rms_mid)
    n_air = norm99(rms_air)

    # Transient onset detection from bass + full energy derivative
    flux = np.maximum(0.0, np.diff(n_bass, prepend=n_bass[0])) * 2.2 + np.maximum(0.0, np.diff(n_all, prepend=n_all[0])) * 1.1
    onset_raw = np.clip(flux / (float(np.percentile(flux, 98.5)) + 1e-6), 0.0, 1.2)
    onset = smooth_ema(onset_raw, attack=0.92, release=0.22)

    s_all = smooth_ema(n_all, attack=0.35, release=0.14)
    s_bass = smooth_ema(n_bass, attack=0.45, release=0.18)
    s_mid = smooth_ema(n_mid, attack=0.40, release=0.16)
    s_air = smooth_ema(n_air, attack=0.40, release=0.15)

    # 8-band spectrum via STFT centered on each frame
    band_edges = np.geomspace(50.0, 9500.0, 9)
    fft_n = 2048
    hann = np.hanning(fft_n).astype(np.float32)
    freqs = np.fft.rfftfreq(fft_n, 1.0 / sr)
    hop = sr // fps
    bands = np.zeros((n_frames, 8), dtype=np.float32)
    for i in range(n_frames):
        center = i * hop
        a = center - fft_n // 2
        b = a + fft_n
        chunk = np.zeros(fft_n, dtype=np.float32)
        sa = max(0, a)
        sb = min(len(pcm), b)
        if sb > sa:
            chunk[sa - a : sb - a] = pcm[sa:sb]
        spec = np.abs(np.fft.rfft(chunk * hann)) / (fft_n * 0.5)
        for bi in range(8):
            mask = (freqs >= band_edges[bi]) & (freqs < band_edges[bi + 1])
            if np.any(mask):
                bands[i, bi] = float(np.sqrt(np.mean(spec[mask] ** 2) + 1e-12))

    for bi in range(8):
        p = float(np.percentile(bands[:, bi], 98.5)) + 1e-6
        bands[:, bi] = smooth_ema(np.clip(bands[:, bi] / p, 0.0, 1.2), 0.42, 0.18)

    # Pack binary: magic "HFQF" (4 bytes), fps (uint32), n_frames (uint32), duration (float32),
    # then for each frame: 13 floats [all, bass, mid, air, onset, b0..b7]
    os.makedirs(os.path.dirname(out_bin), exist_ok=True)
    with open(out_bin, "wb") as f:
        f.write(b"HFQF")
        f.write(struct.pack("<IIf", fps, n_frames, float(duration)))
        frame_mat = np.column_stack([s_all, s_bass, s_mid, s_air, onset, bands]).astype(np.float32)
        f.write(frame_mat.tobytes())

    return {
        "duration": round(duration, 4),
        "fps": fps,
        "n_frames": n_frames,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--flac", default="assets/haofengqi.flac")
    parser.add_argument("--fps", type=int, default=30)
    parser.add_argument("--out-dir", default="build")
    args = parser.parse_args()

    meta = parse_flac_vorbis(args.flac)
    lrc_text = meta["tags"].get("LYRICS", "")
    if not lrc_text:
        raise RuntimeError("No embedded LYRICS tag found in FLAC file")

    os.makedirs(args.out_dir, exist_ok=True)
    audio_info = extract_audio_features(args.flac, args.fps, os.path.join(args.out_dir, "features.bin"))
    parsed = parse_word_lrc(lrc_text, audio_info["duration"])
    parsed["meta"] = {
        "title": meta["tags"].get("TITLE", "好风起"),
        "artist": meta["tags"].get("ARTIST", "周深"),
        "album": meta["tags"].get("ALBUM", "好风起"),
        "duration": audio_info["duration"],
        "fps": audio_info["fps"],
        "n_frames": audio_info["n_frames"],
    }

    lyrics_path = os.path.join(args.out_dir, "lyrics.json")
    with open(lyrics_path, "w", encoding="utf-8") as f:
        json.dump(parsed, f, ensure_ascii=False, indent=2)

    print(
        f"Extracted {len(parsed['credits'])} credit lines, {len(parsed['lyrics'])} lyric lines, "
        f"{audio_info['n_frames']} audio frames @ {args.fps}fps (duration={audio_info['duration']}s)"
    )


if __name__ == "__main__":
    main()
