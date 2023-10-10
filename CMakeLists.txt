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

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL REQUIRED sdl2)

#===============================================================#
# Project config                                                #
#===============================================================#

project(Quake VERSION 1.0.9 LANGUAGES C)

set(QUAKE_VERSION ${CMAKE_PROJECT_VERSION} CACHE STRING "Quake version.")
set(QUAKE_BASEDIR "id1" CACHE STRING "Quake base game directory.")

set(QUAKE_DEFS
    QUAKE_VERSION="${QUAKE_VERSION}"
    QUAKE_BASEDIR="${QUAKE_BASEDIR}"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND QUAKE_DEFS QUAKE_LINUX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND QUAKE_DEFS QUAKE_WINDOWS)
endif()

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

set(QUAKE_DIRS)

set(QUAKE_LIBRARIES m)

#===============================================================#
# Source files                                                  #
#===============================================================#

set(QUAKE_SRC
    nq/cl_demo.c
    nq/cl_input.c
    nq/cl_main.c
    nq/cl_parse.c
    nq/cl_tent.c
    nq/chase.c
    nq/cmd.c
    nq/common.c
    nq/console.c
    nq/crc.c
    nq/cvar.c
    nq/gl_draw.c
    nq/gl_mesh.c
    nq/gl_model.c
    nq/gl_refrag.c
    nq/gl_rlight.c
    nq/gl_rmain.c
    nq/gl_rmisc.c
    nq/gl_rsurf.c
    nq/gl_screen.c
    nq/gl_vidsdl.c
    nq/gl_warp.c
    nq/host.c
    nq/host_cmd.c
    nq/in_sdl.c
    nq/keys.c
    nq/menu.c
    nq/mathlib.c
    nq/net_dgrm.c
    nq/net_loop.c
    nq/net_main.c
    nq/net_vcr.c
    nq/net_udp.c
    nq/net_bsd.c
    nq/pr_cmds.c
    nq/pr_edict.c
    nq/pr_exec.c
    nq/part.c
    nq/sbar.c
    nq/sv_main.c
    nq/sv_phys.c
    nq/sv_move.c
    nq/sv_user.c
    nq/zone.c
    nq/view.c
    nq/wad.c
    nq/world.c
    nq/cd_vorbis.c
    nq/sys_linux.c
    nq/snd_dma.c
    nq/snd_mem.c
    nq/snd_mix.c
    nq/snd_sdl.c
    nq/stb_vorbis.c
)

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
# Quake                                                         #
#===============================================================#

add_executable(quake
    ${QUAKE_SRC}
)

target_compile_definitions(quake PRIVATE ${QUAKE_DEFS})

target_compile_options(quake PRIVATE ${QUAKE_FLAGS})

target_include_directories(quake PRIVATE ${QUAKE_DIRS} ${SDL_INCLUDE_DIRS})

target_link_directories(quake PRIVATE ${SDL_LIBRARY_DIRS})

target_link_libraries(quake PRIVATE ${QUAKE_LIBRARIES} ${SDL_LIBRARIES} GL dl)

#===============================================================#
# QuakeWorld client                                             #
#===============================================================#

add_executable(qwcl
    ${QCOMMON_SRC}
    ${CLIENT_SRC}
)

target_compile_definitions(qwcl PRIVATE GLQUAKE)

target_compile_options(qwcl PRIVATE ${QUAKE_FLAGS})

target_include_directories(qwcl PRIVATE ${QUAKE_DIRS})

target_link_libraries(qwcl PRIVATE ${QUAKE_LIBRARIES} GL X11 Xext dl)

#===============================================================#
# QuakeWorld dedicated server                                   #
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
