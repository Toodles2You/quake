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

set(QUAKE_FLAGS -Wno-format-overflow)

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
# QuakeC Compiler                                               #
#===============================================================#

add_executable(qcc
    common/cmdlib.c
    utils/qcc/pr_comp.c
    utils/qcc/pr_lex.c
    utils/qcc/qcc.c
)
target_include_directories(qcc PRIVATE ${QUAKE_DIRS})
target_link_libraries(qcc PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# BSP                                                           #
#===============================================================#

add_executable(qbsp
    common/bspfile.c
    common/cmdlib.c
    common/mathlib.c
    utils/qbsp/brush.c
    utils/qbsp/csg4.c
    utils/qbsp/map.c
    utils/qbsp/merge.c
    utils/qbsp/nodraw.c
    utils/qbsp/outside.c
    utils/qbsp/portals.c
    utils/qbsp/qbsp.c
    utils/qbsp/region.c
    utils/qbsp/solidbsp.c
    utils/qbsp/surfaces.c
    utils/qbsp/tjunc.c
    utils/qbsp/writebsp.c
)

target_include_directories(qbsp PRIVATE ${QUAKE_DIRS})
target_link_libraries(qbsp PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Light                                                         #
#===============================================================#

add_executable(light
    common/bspfile.c
    common/cmdlib.c
    common/mathlib.c
    common/trilib.c
    utils/light/entities.c
    utils/light/light.c
    utils/light/ltface.c
    utils/light/threads.c
    utils/light/trace.c
)

target_include_directories(light PRIVATE ${QUAKE_DIRS})
target_link_libraries(light PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Vis                                                           #
#===============================================================#

add_executable(vis
    common/bspfile.c
    common/cmdlib.c
    common/mathlib.c
    utils/vis/flow.c
    utils/vis/soundpvs.c
    utils/vis/vis.c
)

target_include_directories(vis PRIVATE ${QUAKE_DIRS})
target_link_libraries(vis PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Info                                                          #
#===============================================================#

add_executable(bspinfo
    common/bspfile.c
    common/cmdlib.c
    utils/bspinfo/bspinfo.c
)

target_include_directories(bspinfo PRIVATE ${QUAKE_DIRS})
target_link_libraries(bspinfo PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Tex Make                                                      #
#===============================================================#

add_executable(texmake
    common/cmdlib.c
    common/lbmlib.c
    common/mathlib.c
    common/scriplib.c
    common/trilib.c
    utils/texmake/texmake.c
)

target_include_directories(texmake PRIVATE ${QUAKE_DIRS})
target_link_libraries(texmake PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Model Gen                                                     #
#===============================================================#

add_executable(modelgen
    common/cmdlib.c
    common/lbmlib.c
    common/mathlib.c
    common/scriplib.c
    common/trilib.c
    utils/modelgen/modelgen.c
)

target_include_directories(modelgen PRIVATE ${QUAKE_DIRS})
target_link_libraries(modelgen PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Sprite Gen                                                    #
#===============================================================#

add_executable(sprgen
    common/cmdlib.c
    common/lbmlib.c
    common/scriplib.c
    utils/sprgen/sprgen.c
)

target_include_directories(sprgen PRIVATE ${QUAKE_DIRS})
target_link_libraries(sprgen PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Lumpy                                                         #
#===============================================================#

add_executable(qlumpy
    common/cmdlib.c
    common/lbmlib.c
    common/scriplib.c
    common/wadlib.c
    utils/qlumpy/qlumpy.c
    utils/qlumpy/quakegrb.c
)

target_include_directories(qlumpy PRIVATE ${QUAKE_DIRS})
target_link_libraries(qlumpy PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Files                                                         #
#===============================================================#

add_executable(qfiles
    common/cmdlib.c
    utils/qfiles/qfiles.c
)

target_include_directories(qfiles PRIVATE ${QUAKE_DIRS})
target_link_libraries(qfiles PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Installation                                                  #
#===============================================================#

install(TARGETS quake DESTINATION .)
