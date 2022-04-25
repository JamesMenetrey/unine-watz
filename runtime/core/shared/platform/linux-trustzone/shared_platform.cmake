# Copyright (C) 2021 University of Neuchatel.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (PLATFORM_SHARED_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions(-DBH_PLATFORM_LINUX_TRUSTZONE)

include_directories(${PLATFORM_SHARED_DIR})
include_directories(${PLATFORM_SHARED_DIR}/../include)

include (${CMAKE_CURRENT_LIST_DIR}/../common/math/platform_api_math.cmake)

if ("$ENV{OPTEE_DIR}" STREQUAL "")
  set (OPTEE_DIR "/opt/watz")
else()
  set (OPTEE_DIR $ENV{OPTEE_DIR})
endif()

#include_directories (${OPTEE_DIR}/optee_os/out/arm/export-ta_arm64/include)
include_directories (${OPTEE_DIR}/optee_os/lib/libutee/include)
include_directories (${OPTEE_DIR}/optee_os/lib/libutils/ext/include)

file (GLOB source_all ${PLATFORM_SHARED_DIR}/*.c)

set (PLATFORM_SHARED_SOURCE ${source_all} ${PLATFORM_COMMON_MATH_SOURCE})


# Cross-compilation to ARM (from samples/simple/profiles/arm-interp/toolchain.cmake)
INCLUDE(CMakeForceCompiler)

SET(CMAKE_SYSTEM_NAME Linux) # this one is important
SET(CMAKE_SYSTEM_VERSION 1) # this one not so much

set (WAMR_BUILD_TARGET AARCH64)
