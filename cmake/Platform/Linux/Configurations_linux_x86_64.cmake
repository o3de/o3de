#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

    include(cmake/Platform/Common/Clang/Configurations_clang.cmake)

    if(DEFINED LY_LINKER)
        set(SPECIFY_LINKER_FLAG "-fuse-ld=${LY_LINKER}")
    elseif(NOT ${LLD_LINKER_INSTALLED} STREQUAL "LLD_LINKER_INSTALLED-NOTFOUND")
        set(SPECIFY_LINKER_FLAG "-fuse-ld=lld")
    endif()
    
    if ($ENV{O3DE_SNAP})
        ly_append_configurations_options(
            DEFINES
                LINUX
                __linux__
                LINUX64
            COMPILATION
                -msse4.1
            LINK_NON_STATIC
                ${SPECIFY_LINKER_FLAG}
                -Wl,--no-undefined
                -fpie
                -Wl,-z,relro,-z,now
                -Wl,-z,noexecstack
                -L$ENV{SNAP}/lib/x86_64-linux-gnu
                -L$ENV{SNAP}/usr/lib/x86_64-linux-gnu
            LINK_EXE
                ${SPECIFY_LINKER_FLAG}
                -fpie
                -Wl,-z,relro,-z,now
                -Wl,-z,noexecstack
                -Wl,--disable-new-dtags
                -L$ENV{SNAP}/lib/x86_64-linux-gnu
                -L$ENV{SNAP}/usr/lib/x86_64-linux-gnu
        )
    else()
        ly_append_configurations_options(
            DEFINES
                LINUX
                __linux__
                LINUX64
            COMPILATION
                -msse4.1
            LINK_NON_STATIC
                ${SPECIFY_LINKER_FLAG}
                -Wl,--no-undefined
                -fpie
                -Wl,-z,relro,-z,now
                -Wl,-z,noexecstack
            LINK_EXE
                ${SPECIFY_LINKER_FLAG}
                -fpie
                -Wl,-z,relro,-z,now
                -Wl,-z,noexecstack
                -Wl,--disable-new-dtags
        )
    endif()

    ly_set(CMAKE_CXX_EXTENSIONS OFF)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

    include(cmake/Platform/Common/GCC/Configurations_gcc.cmake)

    if(LY_GCC_BUILD_FOR_GCOV)
        set(LY_GCC_GCOV_LFLAGS "-lgcov")
    endif()
    if(LY_GCC_BUILD_FOR_GPROF)
        set(LY_GCC_GPROF_LFLAGS "-pg")
    endif()

    ly_append_configurations_options(
        DEFINES
            LINUX
            __linux__
            LINUX64
        COMPILATION
            -msse4.1
        LINK_NON_STATIC
            ${LY_GCC_GCOV_LFLAGS}
            ${LY_GCC_GPROF_LFLAGS}
            -Wl,--no-undefined
            -lpthread
    )
    ly_set(CMAKE_CXX_EXTENSIONS OFF)
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported in ${PAL_PLATFORM_NAME}")
endif()

ly_set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
ly_set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
ly_set(CMAKE_INSTALL_RPATH "$ORIGIN")
