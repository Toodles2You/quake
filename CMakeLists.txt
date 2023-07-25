#===============================================================#
# CMake config                                                  #
#===============================================================#

cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_SHARED_LIBRARY_PREFIX_C "")
set(CMAKE_SHARED_MODULE_PREFIX_C "")
set(CMAKE_SHARED_LIBRARY_PREFIX_CXX "")
set(CMAKE_SHARED_MODULE_PREFIX_CXX "")

#===============================================================#
# Project config                                                #
#===============================================================#

project(quakeworld VERSION 2.4.0 LANGUAGES C)

set(QUAKE_FLAGS
    -Dstricmp=strcasecmp
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND QUAKE_FLAGS
        -O6
        -ffast-math
        -funroll-loopsF
        -fomit-frame-pointer
        -fexpensive-optimizations
    )
endif()

set(QUAKE_DIRS qw/qcommon qw/client qw/server)

set(QUAKE_LIBRARIES m)

#===============================================================#
# Source files                                                  #
#===============================================================#

set(QCOMMON_SRC
    qw/qcommon/cmd.c
    qw/qcommon/common.c
    qw/qcommon/crc.c
    qw/qcommon/cvar.c
    qw/qcommon/mathlib.c
    qw/qcommon/md4.c
    qw/qcommon/zone.c
    qw/qcommon/pmove.c
    qw/qcommon/pmovetst.c
    qw/qcommon/net_chan.c
    qw/qcommon/net_udp.c
)

set(CLIENT_SRC
    qw/client/cl_demo.c
    qw/client/cl_ents.c
    qw/client/cl_input.c
    qw/client/cl_main.c
    qw/client/cl_parse.c
    qw/client/cl_pred.c
    qw/client/cl_tent.c
    qw/client/cl_cam.c
    qw/client/console.c
    qw/client/keys.c
    qw/client/menu.c
    qw/client/nonintel.c
    qw/client/r_part.c
    qw/client/sbar.c
    qw/client/skin.c
    qw/client/snd_dma.c
    qw/client/snd_mem.c
    qw/client/snd_mix.c
    qw/client/view.c
    qw/client/wad.c
    qw/client/cd_linux.c
    qw/client/sys_linux.c
    qw/client/snd_linux.c
    qw/client/gl_draw.c
    qw/client/gl_mesh.c
    qw/client/gl_model.c
    qw/client/gl_ngraph.c
    qw/client/gl_refrag.c
    qw/client/gl_rlight.c
    qw/client/gl_rmain.c
    qw/client/gl_rmisc.c
    qw/client/gl_rsurf.c
    qw/client/gl_screen.c
    qw/client/gl_vidlinuxglx.c
    qw/client/gl_warp.c
    qw/client/math.s
    qw/client/snd_mixa.s
    qw/client/sys_dosa.s
)

set(SERVER_SRC
    qw/server/pr_cmds.c 
    qw/server/pr_edict.c
    qw/server/pr_exec.c
    qw/server/sv_init.c
    qw/server/sv_main.c
    qw/server/sv_nchan.c
    qw/server/sv_ents.c
    qw/server/sv_send.c
    qw/server/sv_move.c
    qw/server/sv_phys.c
    qw/server/sv_user.c
    qw/server/sv_ccmds.c
    qw/server/world.c
    qw/server/sys_unix.c
    qw/server/model.c
)

#===============================================================#
# Client                                                        #
#===============================================================#

add_executable(qwcl
    ${QCOMMON_SRC}
    ${CLIENT_SRC}
)

target_compile_definitions(qwcl PRIVATE GLQUAKE)

target_compile_options(qwcl PRIVATE ${QUAKE_FLAGS})

target_include_directories(qwcl PRIVATE ${QUAKE_DIRS})

target_link_libraries(qwcl PRIVATE ${QUAKE_LIBRARIES} GL X11 Xext)

#===============================================================#
# Dedicated server                                              #
#===============================================================#

add_executable(qwsv
    ${QCOMMON_SRC}
    ${SERVER_SRC}
)

target_compile_definitions(qwsv PRIVATE SERVERONLY)

target_compile_options(qwsv PRIVATE ${QUAKE_FLAGS})

target_include_directories(qwsv PRIVATE ${QUAKE_DIRS})

target_link_libraries(qwsv PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Installation                                                  #
#===============================================================#

install(TARGETS qwcl qwsv DESTINATION .)
