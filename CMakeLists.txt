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

project(quakeworld VERSION 2.3.3 LANGUAGES C)

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

set(QUAKE_DIRS QW/qcommon QW/client QW/server)

set(QUAKE_LIBRARIES m)

#===============================================================#
# Source files                                                  #
#===============================================================#

set(QCOMMON_SRC
    QW/qcommon/cmd.c
    QW/qcommon/common.c
    QW/qcommon/crc.c
    QW/qcommon/cvar.c
    QW/qcommon/mathlib.c
    QW/qcommon/md4.c
    QW/qcommon/zone.c
    QW/qcommon/pmove.c
    QW/qcommon/pmovetst.c
    QW/qcommon/net_chan.c
    QW/qcommon/net_udp.c
)

set(CLIENT_SRC
    QW/client/cl_demo.c
    QW/client/cl_ents.c
    QW/client/cl_input.c
    QW/client/cl_main.c
    QW/client/cl_parse.c
    QW/client/cl_pred.c
    QW/client/cl_tent.c
    QW/client/cl_cam.c
    QW/client/console.c
    QW/client/keys.c
    QW/client/menu.c
    QW/client/nonintel.c
    QW/client/r_part.c
    QW/client/sbar.c
    QW/client/skin.c
    QW/client/snd_dma.c
    QW/client/snd_mem.c
    QW/client/snd_mix.c
    QW/client/view.c
    QW/client/wad.c
    QW/client/cd_linux.c
    QW/client/sys_linux.c
    QW/client/snd_linux.c
    QW/client/gl_draw.c
    QW/client/gl_mesh.c
    QW/client/gl_model.c
    QW/client/gl_ngraph.c
    QW/client/gl_refrag.c
    QW/client/gl_rlight.c
    QW/client/gl_rmain.c
    QW/client/gl_rmisc.c
    QW/client/gl_rsurf.c
    QW/client/gl_screen.c
    QW/client/gl_vidlinuxglx.c
    QW/client/gl_warp.c
    QW/client/math.s
    QW/client/snd_mixa.s
    QW/client/sys_dosa.s
)

set(SERVER_SRC
    QW/server/pr_cmds.c 
    QW/server/pr_edict.c
    QW/server/pr_exec.c
    QW/server/sv_init.c
    QW/server/sv_main.c
    QW/server/sv_nchan.c
    QW/server/sv_ents.c
    QW/server/sv_send.c
    QW/server/sv_move.c
    QW/server/sv_phys.c
    QW/server/sv_user.c
    QW/server/sv_ccmds.c
    QW/server/world.c
    QW/server/sys_unix.c
    QW/server/model.c
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
