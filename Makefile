#---------------------------------------------------------------------------------
# Plants vs. Zombies TV Touch -- Nintendo Switch wrapper (32-bit / AArch32)
#
# Ships NO game code and NO game assets: the game's own APK (the player's
# copy, any name) is read at run time; its libraries are unpacked from it on
# the first launch (source/pvz_setup_plan.c). The mod (libHomura) is this
# port's own build of its GPL source (mod/), carried in the program.
#
# The build is the android32 runtime's (runtime/runtime.mk: devkitARM +
# libnx32 + mesa32 and ffmpeg32 from portlibs32/); ./build.sh runs it in the
# toolchain container. Output: pvz_nx.nsp, which the launcher NRO carries
# (launcher/).
#---------------------------------------------------------------------------------
TARGET               := pvz_nx
PORT_NPDM_PROGRAM_ID := 0x010000000000100E
# .fonts_failed records the build it was written by (pvz_setup_plan.c)
PORT_BUILD_H_USERS   := pvz_setup_plan
# The intro video: FFmpeg's MP4 demuxer and MPEG-4 / AAC decoders, built by
# ffmpeg32's build.sh (LGPL) and copied into portlibs32/. Without them
# pvz_video.c builds without pictures (the game skips its intro).
DCR_VIDEO := $(if $(wildcard portlibs32/lib/libavcodec.a),1,0)
ifeq ($(DCR_VIDEO),1)
PORT_LIBS := -L$(CURDIR)/portlibs32/lib -lavformat -lavcodec -lavutil
endif
include runtime/runtime.mk

# FFmpeg is built with int-sized enums (-fno-short-enums), and so is its one
# user here, whose own interface has no enums (hence --no-enum-size-warning).
$(BUILD)/pvz_video.o: $(SOURCES)/pvz_video.c $(RENDERER_STAMP) | $(BUILD)
	@echo $(notdir $<)
	@$(CC) -MMD -MP $(CFLAGS) -fno-short-enums -DDCR_VIDEO=$(DCR_VIDEO) -c $< -o $@

# The port's English lines, pictures and the mod, assembled in with .incbin
# (not seen by -MMD).
$(BUILD)/pvz_res.o: $(wildcard resources/english/*.txt) $(wildcard resources/logo/*.png) \
  $(wildcard resources/buttons/*.png) mod/out/libHomura.so

.PHONY: check
check:
	@echo "run on the host: python3 runtime/tools/gen_imports.py --check"
