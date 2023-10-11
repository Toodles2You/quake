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

set(QUAKE_FLAGS)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND QUAKE_FLAGS
        -O6
        -ffast-math
        -funroll-loopsF
        -fexpensive-optimizations
    )

    if(NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        list(APPEND QUAKE_FLAGS
            -fomit-frame-pointer
        )
    endif()
endif()

set(QUAKE_DIRS include)

set(QUAKE_LIBRARIES m)

#===============================================================#
# Source files                                                  #
#===============================================================#

set(QCOMMON_SRC
    nq/qcommon/cmd.c
    nq/qcommon/cmodel.c
    nq/qcommon/common.c
    nq/qcommon/crc.c
    nq/qcommon/cvar.c
    nq/qcommon/host_cmd.c
    nq/qcommon/host.c
    nq/qcommon/mathlib.c
    nq/qcommon/net_bsd.c
    nq/qcommon/net_dgrm.c
    nq/qcommon/net_loop.c
    nq/qcommon/net_main.c
    nq/qcommon/net_udp.c
    nq/qcommon/net_vcr.c
    nq/qcommon/sys_linux.c
    nq/qcommon/zone.c
)

set(CLIENT_SRC
    nq/client/cd_vorbis.c
    nq/client/chase.c
    nq/client/cl_demo.c
    nq/client/cl_input.c
    nq/client/cl_main.c
    nq/client/cl_parse.c
    nq/client/cl_tent.c
    nq/client/console.c
    nq/client/gl_draw.c
    nq/client/gl_mesh.c
    nq/client/gl_model.c
    nq/client/gl_refrag.c
    nq/client/gl_rlight.c
    nq/client/gl_rmain.c
    nq/client/gl_rmisc.c
    nq/client/gl_rsurf.c
    nq/client/gl_screen.c
    nq/client/gl_vidsdl.c
    nq/client/gl_warp.c
    nq/client/in_sdl.c
    nq/client/keys.c
    nq/client/menu.c
    nq/client/part.c
    nq/client/sbar.c
    nq/client/snd_dma.c
    nq/client/snd_mem.c
    nq/client/snd_mix.c
    nq/client/snd_sdl.c
    nq/client/stb_vorbis.c
    nq/client/view.c
    nq/client/wad.c
)

set(SERVER_SRC
    nq/server/pr_cmds.c 
    nq/server/pr_edict.c
    nq/server/pr_exec.c
    nq/server/sv_main.c
    nq/server/sv_move.c
    nq/server/sv_phys.c
    nq/server/sv_user.c
    nq/server/world.c
)

#===============================================================#
# Quake                                                         #
#===============================================================#

add_executable(quake
    ${QCOMMON_SRC}
    ${CLIENT_SRC}
    ${SERVER_SRC}
)

target_compile_definitions(quake PRIVATE ${QUAKE_DEFS})

target_compile_options(quake PRIVATE ${QUAKE_FLAGS})

target_include_directories(quake PRIVATE ${QUAKE_DIRS} ${SDL_INCLUDE_DIRS})

target_link_directories(quake PRIVATE ${SDL_LIBRARY_DIRS})

target_link_libraries(quake PRIVATE ${QUAKE_LIBRARIES} ${SDL_LIBRARIES} GL dl)

#===============================================================#
# Installation                                                  #
#===============================================================#

install(TARGETS quake DESTINATION .)
