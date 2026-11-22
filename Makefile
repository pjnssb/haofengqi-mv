CXX      := g++
CXXFLAGS := -std=c++20 -O3 -fopenmp -march=native -Wall -Wextra -Wno-unused-parameter $(shell pkg-config --cflags pangocairo cairo)
LDFLAGS  := -fopenmp $(shell pkg-config --libs pangocairo cairo)

TARGET   := build/mv_render
FLAC     := assets/haofengqi.flac
FPS      := 30
OUT_MP4  := build/haofengqi_mv_1080p.mp4

.PHONY: all features previews render clean

all: $(TARGET)

$(TARGET): src/main.cpp src/audio_lyrics.hpp src/storyboard.hpp src/hdr_canvas.hpp src/sky_and_atmosphere.hpp src/cairo_scenes.hpp src/typography.hpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) src/main.cpp -o $(TARGET) $(LDFLAGS)

features:
	/usr/bin/python3 scripts/extract_features.py --flac $(FLAC) --fps $(FPS) --out-dir build

previews: $(TARGET) features
	./$(TARGET) --lyrics build/lyrics.json --features build/features.bin --preview-dir build/previews

render: $(TARGET) features
	./$(TARGET) --lyrics build/lyrics.json --features build/features.bin --stream | \
	ffmpeg -y -v warning -stats \
	  -f rawvideo -pix_fmt rgb24 -s 1920x1080 -r $(FPS) -i - \
	  -i $(FLAC) \
	  -map 0:v:0 -map 1:a:0 \
	  -c:v libx264 -preset medium -crf 17 -pix_fmt yuv420p -movflags +faststart \
	  -c:a aac -b:a 320k \
	  -shortest $(OUT_MP4)

clean:
	rm -rf build
