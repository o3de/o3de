#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

    include(cmake/Platform/Common/Clang/Configurations_clang.cmake)

    ly_append_configurations_options(
        DEFINES
            LINUX
            __linux__
            LINUX64
        COMPILATION
            -fPIC
            -msse4.1
    )
    ly_set(CMAKE_CXX_EXTENSIONS OFF)
else()

    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported in ${PAL_PLATFORM_NAME}")

endif()

ly_set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
ly_set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
ly_set(CMAKE_INSTALL_RPATH "$ORIGIN")

if(CMAKE_GENERATOR MATCHES "Ninja")
    if(LY_PARALLEL_LINK_JOBS)
        set_property(GLOBAL APPEND PROPERTY JOB_POOLS link_job_pool=${LY_PARALLEL_LINK_JOBS})
        ly_set(CMAKE_JOB_POOL_LINK link_job_pool)
    endif()
endif()
