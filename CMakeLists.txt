#===============================================================#
# CMake config                                                  #
#===============================================================#

cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL REQUIRED sdl2)

#===============================================================#
# Project config                                                #
#===============================================================#

project(Quake VERSION 1.0.9 LANGUAGES C)

set(QUAKE_VERSION ${CMAKE_PROJECT_VERSION} CACHE STRING "Quake version.")
set(QUAKE_BASEDIR "id1" CACHE STRING "Quake base game directory.")
option(QUAKE_REGISTERED "Quake full version checks." OFF)

set(QUAKE_DEFS
    QUAKE_VERSION="${QUAKE_VERSION}"
    QUAKE_BASEDIR="${QUAKE_BASEDIR}"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND QUAKE_DEFS QUAKE_LINUX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND QUAKE_DEFS QUAKE_WINDOWS)
endif()

if(QUAKE_REGISTERED)
    list(APPEND QUAKE_DEFS QUAKE_REGISTERED)
endif()

set(QUAKE_FLAGS -Wno-format-overflow -Wno-discarded-qualifiers)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND QUAKE_FLAGS
        -O6
        -ffast-math
        -funroll-loops
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

set(COMMON_SRC
    common/crc.c
    common/md4.c
    common/mathlib.c
)

set(ENGINE_COMMON_SRC
    engine/common/cmd.c
    engine/common/cmodel.c
    engine/common/common.c
    engine/common/cvar.c
    engine/common/host_cmd.c
    engine/common/host.c
    engine/common/net_chan.c
    engine/common/net_udp.c
    engine/common/pmove.c
    engine/common/pmovetst.c
    engine/common/sys_linux.c
    engine/common/zone.c
)

set(ENGINE_CLIENT_SRC
    engine/client/cd_vorbis.c
    engine/client/chase.c
    engine/client/cl_cam.c
    engine/client/cl_demo.c
    engine/client/cl_ents.c
    engine/client/cl_input.c
    engine/client/cl_main.c
    engine/client/cl_parse.c
    engine/client/cl_pred.c
    engine/client/cl_tent.c
    engine/client/console.c
    engine/client/gl_draw.c
    engine/client/gl_mesh.c
    engine/client/gl_model.c
    engine/client/gl_ngraph.c
    engine/client/gl_refrag.c
    engine/client/gl_rlight.c
    engine/client/gl_rmain.c
    engine/client/gl_rmisc.c
    engine/client/gl_rsurf.c
    engine/client/gl_screen.c
    engine/client/gl_vidsdl.c
    engine/client/gl_warp.c
    engine/client/in_sdl.c
    engine/client/keys.c
    engine/client/menu.c
    engine/client/part.c
    engine/client/sbar.c
    engine/client/skin.c
    engine/client/snd_dma.c
    engine/client/snd_mem.c
    engine/client/snd_mix.c
    engine/client/snd_sdl.c
    engine/client/stb_vorbis.c
    engine/client/view.c
    engine/client/wad.c
)

set(ENGINE_SERVER_SRC
    engine/server/progs.c
    engine/server/pr_cmds.c 
    engine/server/pr_edict.c
    engine/server/pr_exec.c
    engine/server/sv_ccmds.c
    engine/server/sv_ents.c
    engine/server/sv_init.c
    engine/server/sv_main.c
    engine/server/sv_move.c
    engine/server/sv_nchan.c
    engine/server/sv_phys.c
    engine/server/sv_send.c
    engine/server/sv_user.c
    engine/server/world.c
)

#===============================================================#
# Quake                                                         #
#===============================================================#

add_executable(quake
    ${COMMON_SRC}
    ${ENGINE_COMMON_SRC}
    ${ENGINE_CLIENT_SRC}
    ${ENGINE_SERVER_SRC}
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

target_compile_definitions(qcc PRIVATE ${QUAKE_DEFS})
target_compile_options(qcc PRIVATE ${QUAKE_FLAGS})
target_include_directories(qcc PRIVATE ${QUAKE_DIRS})
target_link_libraries(qcc PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# BSP                                                           #
#===============================================================#

add_executable(qbsp
    ${COMMON_SRC}
    common/bspfile.c
    common/cmdlib.c
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

target_compile_definitions(qbsp PRIVATE ${QUAKE_DEFS})
target_compile_options(qbsp PRIVATE ${QUAKE_FLAGS})
target_include_directories(qbsp PRIVATE ${QUAKE_DIRS})
target_link_libraries(qbsp PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Light                                                         #
#===============================================================#

add_executable(light
    ${COMMON_SRC}
    common/bspfile.c
    common/cmdlib.c
    common/trilib.c
    utils/light/entities.c
    utils/light/light.c
    utils/light/ltface.c
    utils/light/threads.c
    utils/light/trace.c
)

target_compile_definitions(light PRIVATE ${QUAKE_DEFS})
target_compile_options(light PRIVATE ${QUAKE_FLAGS})
target_include_directories(light PRIVATE ${QUAKE_DIRS})
target_link_libraries(light PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Vis                                                           #
#===============================================================#

add_executable(vis
    ${COMMON_SRC}
    common/bspfile.c
    common/cmdlib.c
    utils/vis/flow.c
    utils/vis/soundpvs.c
    utils/vis/vis.c
)

target_compile_definitions(vis PRIVATE ${QUAKE_DEFS})
target_compile_options(vis PRIVATE ${QUAKE_FLAGS})
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

target_compile_definitions(bspinfo PRIVATE ${QUAKE_DEFS})
target_compile_options(bspinfo PRIVATE ${QUAKE_FLAGS})
target_include_directories(bspinfo PRIVATE ${QUAKE_DIRS})
target_link_libraries(bspinfo PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Tex Make                                                      #
#===============================================================#

add_executable(texmake
    ${COMMON_SRC}
    common/cmdlib.c
    common/lbmlib.c
    common/scriplib.c
    common/trilib.c
    utils/texmake/texmake.c
)

target_compile_definitions(texmake PRIVATE ${QUAKE_DEFS})
target_compile_options(texmake PRIVATE ${QUAKE_FLAGS})
target_include_directories(texmake PRIVATE ${QUAKE_DIRS})
target_link_libraries(texmake PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Model Gen                                                     #
#===============================================================#

add_executable(modelgen
    ${COMMON_SRC}
    common/cmdlib.c
    common/lbmlib.c
    common/scriplib.c
    common/trilib.c
    utils/modelgen/modelgen.c
)

target_compile_definitions(modelgen PRIVATE ${QUAKE_DEFS})
target_compile_options(modelgen PRIVATE ${QUAKE_FLAGS})
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

target_compile_definitions(sprgen PRIVATE ${QUAKE_DEFS})
target_compile_options(sprgen PRIVATE ${QUAKE_FLAGS})
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

target_compile_definitions(qlumpy PRIVATE ${QUAKE_DEFS})
target_compile_options(qlumpy PRIVATE ${QUAKE_FLAGS})
target_include_directories(qlumpy PRIVATE ${QUAKE_DIRS})
target_link_libraries(qlumpy PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Files                                                         #
#===============================================================#

add_executable(qfiles
    common/cmdlib.c
    utils/qfiles/qfiles.c
)

target_compile_definitions(qfiles PRIVATE ${QUAKE_DEFS})
target_compile_options(qfiles PRIVATE ${QUAKE_FLAGS})
target_include_directories(qfiles PRIVATE ${QUAKE_DIRS})
target_link_libraries(qfiles PRIVATE ${QUAKE_LIBRARIES})

#===============================================================#
# Installation                                                  #
#===============================================================#

install(TARGETS quake DESTINATION .)
