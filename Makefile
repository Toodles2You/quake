ENGINE_NAME ?= quake
ENGINE_VERSION ?= 1.0.9
ENGINE_BASEDIR ?= id1

INSTALL_DIR ?= /opt/$(ENGINE_NAME)

SDL_LIBS ?= $(shell pkg-config --libs sdl2)
SDL_CFLAGS ?= $(shell pkg-config --cflags sdl2)

AL_LIBS ?= $(shell pkg-config --libs openal)
AL_CFLAGS ?= $(shell pkg-config --cflags openal)

ifdef VERBOSE
Q =
else
Q = @
endif

CC := gcc -std=gnu11

REL_DIR = build/release
DBG_DIR = build/debug

# ==============================================================

COMMON_SRC = \
	common/crc.c \
	common/md4.c \
	common/mathlib.c \

COMMON_OBJ = $(patsubst %.c, %.o, $(COMMON_SRC))

# ==============================================================

ENGINE_COMMON_SRC = \
	engine/common/cmd.c \
	engine/common/cmodel.c \
	engine/common/common.c \
	engine/common/cvar.c \
	engine/common/host_cmd.c \
	engine/common/host.c \
	engine/common/net_chan.c \
	engine/common/net_udp.c \
	engine/common/pmove.c \
	engine/common/pmovetst.c \
	engine/common/sys_linux.c \
	engine/common/zone.c \

ENGINE_COMMON_OBJ = $(patsubst %.c, %.o, $(ENGINE_COMMON_SRC))

# ==============================================================

ENGINE_CLIENT_SRC = \
	engine/client/al_sound.c \
	engine/client/al_vorbis.c \
	engine/client/al_wave.c \
	engine/client/chase.c \
	engine/client/cl_cam.c \
	engine/client/cl_demo.c \
	engine/client/cl_ents.c \
	engine/client/cl_input.c \
	engine/client/cl_main.c \
	engine/client/cl_parse.c \
	engine/client/cl_pred.c \
	engine/client/cl_tent.c \
	engine/client/console.c \
	engine/client/gl_draw.c \
	engine/client/gl_mesh.c \
	engine/client/gl_model.c \
	engine/client/gl_ngraph.c \
	engine/client/gl_refrag.c \
	engine/client/gl_rlight.c \
	engine/client/gl_rmain.c \
	engine/client/gl_rmisc.c \
	engine/client/gl_rsurf.c \
	engine/client/gl_screen.c \
	engine/client/gl_vidsdl.c \
	engine/client/gl_warp.c \
	engine/client/in_sdl.c \
	engine/client/keys.c \
	engine/client/menu.c \
	engine/client/part.c \
	engine/client/sbar.c \
	engine/client/skin.c \
	engine/client/stb_vorbis.c \
	engine/client/view.c \
	engine/client/wad.c \

ENGINE_CLIENT_OBJ = $(patsubst %.c, %.o, $(ENGINE_CLIENT_SRC))

# ==============================================================

ENGINE_SERVER_SRC = \
	engine/server/progs.c \
	engine/server/pr_cmds.c \
	engine/server/pr_edict.c \
	engine/server/pr_exec.c \
	engine/server/sv_ccmds.c \
	engine/server/sv_ents.c \
	engine/server/sv_init.c \
	engine/server/sv_main.c \
	engine/server/sv_move.c \
	engine/server/sv_nchan.c \
	engine/server/sv_phys.c \
	engine/server/sv_send.c \
	engine/server/sv_user.c \
	engine/server/world.c \

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

ENGINE_LIBS = -lm -lGL -ldl $(SDL_LIBS) $(AL_LIBS)

ENGINE_CFLAGS = \
	-ffast-math \
	-DQUAKE_VERSION=\"$(ENGINE_VERSION)\" \
	-DQUAKE_BASEDIR=\"$(ENGINE_BASEDIR)\" \
	-Icommon \
	-Iengine/common \
	-Iengine/client \
	-Iengine/server \
	$(SDL_CFLAGS) \
	$(AL_LIBS) \

REL_ENGINE_CFLAGS = \
	-O6 \
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
	$(Q)$(CC) -o $@ $(REL_ENGINE_OBJ) $(ENGINE_LIBS)

$(REL_DIR)/%.o : %.c
	@echo $@
	$(Q)$(CC) $(REL_ENGINE_CFLAGS) -c $< -o $@

debug : dirs $(DBG_ENGINE)

$(DBG_ENGINE) : $(DBG_ENGINE_OBJ)
	@echo $@
	$(Q)$(CC) -o $@ $(DBG_ENGINE_OBJ) $(ENGINE_LIBS)

$(DBG_DIR)/%.o : %.c
	@echo $<
	$(Q)$(CC) $(DBG_ENGINE_CFLAGS) -c $< -o $@

dirs :
	$(Q)mkdir -p $(REL_DIR)
	$(Q)mkdir -p $(REL_DIR)/common
	$(Q)mkdir -p $(REL_DIR)/engine/common
	$(Q)mkdir -p $(REL_DIR)/engine/client
	$(Q)mkdir -p $(REL_DIR)/engine/server
	$(Q)mkdir -p $(DBG_DIR)
	$(Q)mkdir -p $(DBG_DIR)/common
	$(Q)mkdir -p $(DBG_DIR)/engine/common
	$(Q)mkdir -p $(DBG_DIR)/engine/client
	$(Q)mkdir -p $(DBG_DIR)/engine/server

clean :
	$(Q)-rm -rf $(REL_DIR)
	$(Q)-rm -rf $(DBG_DIR)

install :
	@echo Installing to \'$(INSTALL_DIR)\'...
	$(Q)mkdir $(INSTALL_DIR)
	$(Q)cp --no-preserve=all $(REL_ENGINE) $(INSTALL_DIR)
