ifdef VERBOSE
Q =
else
Q = @
endif

CC := gcc

DO_CC = $(CC) -std=gnu11
EXEC =
SHLIB = .so
REL_DIR = build/release
DBG_DIR = build/debug

APP_DIR = app
INSTALL_DIR := /opt/quake

# ==============================================================

COMMON_SRC = \
	src/common/crc.c \
	src/common/md4.c \
	src/common/mathlib.c \

COMMON_OBJ = $(patsubst %.c, %.o, $(COMMON_SRC))

# ==============================================================

ENGINE_NAME = quake
ENGINE_VERSION = 1.0.9
ENGINE_BASEDIR = id1

PKG_LIBS = $(shell pkg-config --libs sdl2 openal)
PKG_CFLAGS = $(shell pkg-config --cflags sdl2 openal)

# ==============================================================

ENGINE_COMMON_SRC = \
	src/engine/common/cmd.c \
	src/engine/common/cmodel.c \
	src/engine/common/common.c \
	src/engine/common/cvar.c \
	src/engine/common/host_cmd.c \
	src/engine/common/host.c \
	src/engine/common/net_chan.c \
	src/engine/common/net_udp.c \
	src/engine/common/pmove.c \
	src/engine/common/pmovetst.c \
	src/engine/common/sys_linux.c \
	src/engine/common/zone.c \

ENGINE_COMMON_OBJ = $(patsubst %.c, %.o, $(ENGINE_COMMON_SRC))

# ==============================================================

ENGINE_CLIENT_SRC = \
	src/engine/client/al_sound.c \
	src/engine/client/al_vorbis.c \
	src/engine/client/al_wave.c \
	src/engine/client/chase.c \
	src/engine/client/cl_demo.c \
	src/engine/client/cl_ents.c \
	src/engine/client/cl_input.c \
	src/engine/client/cl_main.c \
	src/engine/client/cl_parse.c \
	src/engine/client/cl_pred.c \
	src/engine/client/cl_tent.c \
	src/engine/client/console.c \
	src/engine/client/gl_draw.c \
	src/engine/client/gl_mesh.c \
	src/engine/client/gl_model.c \
	src/engine/client/gl_ngraph.c \
	src/engine/client/gl_refrag.c \
	src/engine/client/gl_rlight.c \
	src/engine/client/gl_rmain.c \
	src/engine/client/gl_rmisc.c \
	src/engine/client/gl_rsurf.c \
	src/engine/client/gl_screen.c \
	src/engine/client/gl_vidsdl.c \
	src/engine/client/gl_warp.c \
	src/engine/client/in_sdl.c \
	src/engine/client/keys.c \
	src/engine/client/menu.c \
	src/engine/client/part.c \
	src/engine/client/sbar.c \
	src/engine/client/stb_vorbis.c \
	src/engine/client/view.c \
	src/engine/client/wad.c \

ENGINE_CLIENT_OBJ = $(patsubst %.c, %.o, $(ENGINE_CLIENT_SRC))

# ==============================================================

ENGINE_SERVER_SRC = \
	src/engine/server/progs.c \
	src/engine/server/pr_cmds.c \
	src/engine/server/pr_edict.c \
	src/engine/server/pr_exec.c \
	src/engine/server/sv_ccmds.c \
	src/engine/server/sv_ents.c \
	src/engine/server/sv_init.c \
	src/engine/server/sv_main.c \
	src/engine/server/sv_move.c \
	src/engine/server/sv_nchan.c \
	src/engine/server/sv_phys.c \
	src/engine/server/sv_send.c \
	src/engine/server/sv_user.c \
	src/engine/server/world.c \

ENGINE_SERVER_OBJ = $(patsubst %.c, %.o, $(ENGINE_SERVER_SRC))

# ==============================================================

REL_ENGINE = $(REL_DIR)/$(ENGINE_NAME)
DBG_ENGINE = $(DBG_DIR)/$(ENGINE_NAME)

ENGINE_OBJ = \
	$(COMMON_OBJ) \
	$(ENGINE_COMMON_OBJ) \
	$(ENGINE_CLIENT_OBJ) \
	$(ENGINE_SERVER_OBJ) \

REL_ENGINE_OBJ = $(addprefix $(REL_DIR)/, $(ENGINE_OBJ))
DBG_ENGINE_OBJ = $(addprefix $(DBG_DIR)/, $(ENGINE_OBJ))

ENGINE_LIBS = -lm -lGL -ldl $(PKG_LIBS)

ENGINE_CFLAGS = \
	-ffast-math \
	-DENGINE_VERSION=\"$(ENGINE_VERSION)\" \
	-DENGINE_BASEDIR=\"$(ENGINE_BASEDIR)\" \
	-Isrc/common \
	-Isrc/engine/common \
	-Isrc/engine/client \
	-Isrc/engine/server \
	$(PKG_CFLAGS) \

REL_ENGINE_CFLAGS = \
	-funroll-loops \
	-fexpensive-optimizations \
	-fomit-frame-pointer \
	$(ENGINE_CFLAGS) \

DBG_ENGINE_CFLAGS = \
	-Og \
	$(ENGINE_CFLAGS) \

# ==============================================================

.PHONY : all release debug dirs clean install

all : release

release : dirs $(REL_ENGINE)

$(REL_ENGINE) : $(REL_ENGINE_OBJ)
	@echo $<
	$(Q)$(DO_CC) -DNDEBUG -o $@ $(REL_ENGINE_OBJ) $(ENGINE_LIBS)
	$(Q)cp $@ $(APP_DIR)

$(REL_DIR)/%.o : %.c
	@echo $@
	$(Q)$(DO_CC) -DNDEBUG $(REL_ENGINE_CFLAGS) -c $< -o $@

debug : dirs $(DBG_ENGINE)

$(DBG_ENGINE) : $(DBG_ENGINE_OBJ)
	@echo $@
	$(Q)$(DO_CC) -ggdb -o $@ $(DBG_ENGINE_OBJ) $(ENGINE_LIBS)
	$(Q)cp $@ $(APP_DIR)

$(DBG_DIR)/%.o : %.c
	@echo $<
	$(Q)$(DO_CC) -ggdb $(DBG_ENGINE_CFLAGS) -c $< -o $@

dirs :
	$(Q)mkdir -p $(REL_DIR)
	$(Q)mkdir -p $(REL_DIR)/src
	$(Q)mkdir -p $(REL_DIR)/src/common
	$(Q)mkdir -p $(REL_DIR)/src/engine/common
	$(Q)mkdir -p $(REL_DIR)/src/engine/client
	$(Q)mkdir -p $(REL_DIR)/src/engine/server
	$(Q)mkdir -p $(DBG_DIR)
	$(Q)mkdir -p $(DBG_DIR)/src
	$(Q)mkdir -p $(DBG_DIR)/src/common
	$(Q)mkdir -p $(DBG_DIR)/src/engine/common
	$(Q)mkdir -p $(DBG_DIR)/src/engine/client
	$(Q)mkdir -p $(DBG_DIR)/src/engine/server
	$(Q)mkdir -p $(APP_DIR)

clean :
	$(Q)-rm -rf $(REL_DIR)
	$(Q)-rm -rf $(DBG_DIR)

install :
	@echo Installing to \'$(INSTALL_DIR)\'...
	$(Q)mkdir -p $(INSTALL_DIR)
	$(Q)cp -rf $(APP_DIR) $(INSTALL_DIR)
