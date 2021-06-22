#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
