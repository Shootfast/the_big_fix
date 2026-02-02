BUILD_DIR=_build
export N64_INST ?= ./ext/libdragon
include $(N64_INST)/include/n64.mk
include $(N64_INST)/include/t3d.mk

PROJECT_NAME=the_big_fix

N64_MKDFS_ROOT = filesystem
assets_bci = $(wildcard assets/*.bci.png) $(wildcard assets/skybox/*/*.bci.png)

assets_sprite = $(filter-out $(assets_bci), $(wildcard assets/*.png))

assets_font = $(wildcard assets/*.ttf)

assets_wav = $(wildcard assets/*.wav)

assets_glb = $(wildcard assets/*.glb)

assets_lvl = $(wildcard assets/*lvl.glb)

assets_conv =  $(patsubst assets/%,filesystem/%,$(assets_bci:%.png=%)) \
               $(patsubst assets/%,filesystem/%,$(assets_sprite:%.png=%.sprite)) \
               $(patsubst assets/%,filesystem/%,$(assets_font:%.ttf=%.font64)) \
               $(patsubst assets/%,filesystem/%,$(assets_wav:%.wav=%.wav64)) \
               $(patsubst assets/%,filesystem/%,$(assets_glb:%.glb=%.t3dm)) \
               $(patsubst assets/%,filesystem/%,$(assets_lvl:%.glb=%))

src = $(wildcard src/*.c) $(wildcard src/**/*.c)

MKSPRITE_FLAGS ?=
#T3DM_FLAGS ?= --bvh --base-scale=64

all: $(PROJECT_NAME).z64

# Convert wav files in the "assets" folder into wav64
filesystem/%.wav64: assets/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 1 -o "$(dir $@)" "$<"

# Convert glb files in the "assets" folder into t3dm
filesystem/%.t3dm: assets/%.glb
	@mkdir -p $(dir $@)
	@echo "    [T3D-MODEL] $@"
	$(T3D_GLTF_TO_3D) "$<" $@ $(T3DM_FLAGS)
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o $(dir $@) $@

# Convert ttf font files to font64
filesystem/%.font64: assets/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	$(N64_MKFONT) $(MKFONT_FLAGS) -o $(dir $@) "$<"

# Convert sprite.png files in the "assets" folder into sprite
filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o $(dir $@) "$<"

filesystem/%.bci: assets/%.bci.png tools/imgconv/imgconv
	@mkdir -p $(dir $@)
	@echo "    [HD-IMG] $@ $<"
	./tools/imgconv/imgconv "$<" $@
	$(N64_BINDIR)/mkasset -c 2 -o $(dir $@) $@

# Convert lvl.glb files in the "assets" folder into lvl
filesystem/%.lvl: assets/%.lvl.glb tools/glb_to_lvl/glb_to_lvl assets/%.lvl.json
	@mkdir -p $(dir $@)
	@echo "    [LVL] $@"
	if [ -f "assets/$*.lvl.json" ]; then \
		echo ./tools/glb_to_lvl/glb_to_lvl "$<" --json "assets/$*.lvl.json" $@; \
		./tools/glb_to_lvl/glb_to_lvl "$<" --json "assets/$*.lvl.json" $@; \
	else \
		echo ./tools/glb_to_lvl/glb_to_lvl "$<" $@;\
		./tools/glb_to_lvl/glb_to_lvl "$<" $@;\
	fi
	xxd $@
	$(N64_BINDIR)/mkasset -c 2 -o $(dir $@) $@

tools/imgconv/imgconv:
	@echo "    [BUILD] imgconv"
	@make -C tools/imgconv

tools/glb_to_lvl/glb_to_lvl: tools/glb_to_lvl/main.c
	@echo "    [BUILD] glb_to_lvl"
	@make -C tools/glb_to_lvl

$(BUILD_DIR)/$(PROJECT_NAME).dfs: $(assets_conv)
$(BUILD_DIR)/$(PROJECT_NAME).elf: $(src:%.c=$(BUILD_DIR)/%.o) $(BUILD_DIR)/src/tex.o $(BUILD_DIR)/src/rsp/rsp_fx.o

$(PROJECT_NAME).z64: CFLAGS+= -Isrc
$(PROJECT_NAME).z64: N64_ROM_TITLE=$(PROJECT_NAME)
$(PROJECT_NAME).z64: $(BUILD_DIR)/$(PROJECT_NAME).dfs

clean:
	rm -rf $(BUILD_DIR) filesystem $(PROJECT_NAME).z64
	make clean -C tools/imgconv
	make clean -C tools/glb_to_lvl

-include $(wildcard $(BUILD_DIR)/src/*.d)

run: $(PROJECT_NAME).z64
	ares --system 'Nintendo 64' $(PROJECT_NAME).z64

.PHONY: all clean
