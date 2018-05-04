set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR powerpc)
set(WIIU ON)
set(CMAKE_CROSSCOMPILING ON)

if(NOT DEFINED ENV{DEVKITPPC})
   message(FATAL_ERROR "Please set DEVKITPPC in your environment. export DEVKITPPC=/path/to/devkitPPC")
endif()

if(NOT DEFINED ENV{DEVKITPRO})
   message(FATAL_ERROR "Please set DEVKITPRO in your environment. export DEVKITPRO=/path/to/devkitPRO")
endif()


set(CMAKE_WARN_DEPRECATED OFF)
INCLUDE(CMakeForceCompiler)
CMAKE_FORCE_C_COMPILER($ENV{DEVKITPPC}/bin/powerpc-eabi-gcc GNU)
CMAKE_FORCE_CXX_COMPILER($ENV{DEVKITPPC}/bin/powerpc-eabi-g++ GNU)
set(CMAKE_WARN_DEPRECATED ON)

SET(CMAKE_FIND_ROOT_PATH  $ENV{DEVKITPPC})
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# specify the cross compiler
#SET(CMAKE_C_COMPILER   $ENV{DEVKITPPC}/bin/powerpc-eabi-gcc)
#SET(CMAKE_CXX_COMPILER $ENV{DEVKITPPC}/bin/powerpc-eabi-g++)
#SET(CMAKE_ASM_COMPILER $ENV{DEVKITPPC}/bin/powerpc-eabi-as)

if(NOT DEFINED WIIU_ROOT)
   set(WIIU_ROOT ${CMAKE_SOURCE_DIR}/ext/wiiu)
endif()

include_directories(${WIIU_ROOT}/include)
include_directories($ENV{DEVKITPRO}/portlibs/ppc/include)
link_directories($ENV{DEVKITPRO}/portlibs/ppc/lib)


set(ARCH_FLAGS "-mwup -mcpu=750 -meabi -mhard-float")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARCH_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${ARCH_FLAGS} -mregnames")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -fPIE -z common-page-size=64 -z max-page-size=64 -T ${WIIU_ROOT}/link.ld -nostartfiles")
set(CMAKE_DL_LIBS "")

if(NOT WIIU_LIBRARIES_ADDED)
add_library(wiiu STATIC
   ${WIIU_ROOT}/missing_libc_functions.c
   ${WIIU_ROOT}/exception_handler.c
   ${WIIU_ROOT}/entry.c
   ${WIIU_ROOT}/logger.c
   ${WIIU_ROOT}/memory.c
   ${WIIU_ROOT}/fs_utils.c
   ${WIIU_ROOT}/sd_fat_devoptab.c
   ${WIIU_ROOT}/stubs.S

   ${WIIU_ROOT}/gthr-wup.cpp
   ${WIIU_ROOT}/std/pread.c
   ${WIIU_ROOT}/std/condition_variable.cc
   ${WIIU_ROOT}/std/mutex.cc
   ${WIIU_ROOT}/std/thread.cc
   ${WIIU_ROOT}/std/fstream-inst.cc
   ${WIIU_ROOT}/std/basic_file_stdio.cc
   )
add_library(fat STATIC
   ${WIIU_ROOT}/libfat/bit_ops.h
   ${WIIU_ROOT}/libfat/cache.c
   ${WIIU_ROOT}/libfat/cache.h
   ${WIIU_ROOT}/libfat/common.h
   ${WIIU_ROOT}/libfat/directory.c
   ${WIIU_ROOT}/libfat/directory.h
   ${WIIU_ROOT}/libfat/disc.c
   ${WIIU_ROOT}/libfat/disc.h
   ${WIIU_ROOT}/libfat/fatdir.c
   ${WIIU_ROOT}/libfat/fatdir.h
   ${WIIU_ROOT}/libfat/fatfile.c
   ${WIIU_ROOT}/libfat/fatfile.h
   ${WIIU_ROOT}/libfat/file_allocation_table.c
   ${WIIU_ROOT}/libfat/file_allocation_table.h
   ${WIIU_ROOT}/libfat/filetime.c
   ${WIIU_ROOT}/libfat/filetime.h
   ${WIIU_ROOT}/libfat/libfat.c
   ${WIIU_ROOT}/libfat/lock.c
   ${WIIU_ROOT}/libfat/lock.h
   ${WIIU_ROOT}/libfat/mem_allocate.h
   ${WIIU_ROOT}/libfat/partition.c
   ${WIIU_ROOT}/libfat/partition.h
   )
add_library(iosuhax STATIC
   ${WIIU_ROOT}/libiosuhax/iosuhax.c
   ${WIIU_ROOT}/libiosuhax/iosuhax_devoptab.c
   ${WIIU_ROOT}/libiosuhax/iosuhax_disc_interface.c
   )
set(WIIU_LIBRARIES_ADDED TRUE)
endif()

set(ELF2RPL "${WIIU_ROOT}/elf2rpl/elf2rpl")

macro(add_rpx_target target)
    add_custom_target(${target}.rpx ALL)
    add_dependencies(${target}.rpx ${target})
    add_custom_command(TARGET ${target}.rpx
                       COMMAND ${ELF2RPL} ${target} ${target}.rpx)
endmacro()
