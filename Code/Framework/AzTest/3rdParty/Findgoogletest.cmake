#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# GoogleTest and GoogleMock come from one repository, so a single fetch builds both.
# Installer/Findgoogletest.cmake replaces this file in pre-built installer mode.

if(TARGET 3rdParty::googletest::GTest OR TARGET 3rdParty::googletest::GMock)
    return()
endif()

# Do not depend on googletest (via AzTest) where it cannot compile.
# See cmake/Platform/<platform>/PAL_<platform>.cmake
if(NOT PAL_TRAIT_TEST_GOOGLE_TEST_SUPPORTED)
    return()
endif()

block()
    set(ADDITIONAL_FETCHCONTENT_FLAGS "")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.25")
        list(APPEND ADDITIONAL_FETCHCONTENT_FLAGS "SYSTEM") # do not apply our warning level to googletest headers
    endif()

    o3de_fetch_content(googletest
        VERSION "v1.18.0"
        LICENSE "BSD-3-Clause"
        URL "https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz"
        URL_HASH "6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5"
        GIT "https://github.com/google/googletest.git"
        GIT_HASH "063de7e9578f82b369302001269680b4b1553359"
        ${ADDITIONAL_FETCHCONTENT_FLAGS}
    )

    # FetchContent ignores CMAKE_ARGS by design (https://gitlab.kitware.com/cmake/cmake/-/issues/20799),
    # so configure googletest through normal variables that its option() calls pick up via CMP0077.
    set(BUILD_GMOCK ON)
    set(BUILD_SHARED_LIBS OFF)
    set(INSTALL_GTEST OFF) # AzTest installs what it needs below
    set(gmock_build_tests OFF)
    set(gtest_build_samples OFF)
    set(gtest_build_tests OFF)
    set(gtest_force_shared_crt ON)
    set(gtest_hide_internal_symbols OFF) # AzTest modifies internal flags such as FLAGS_gtest_catch_exceptions

    set(OLD_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})

    FetchContent_MakeAvailable(googletest)

    set(CMAKE_MESSAGE_LOG_LEVEL ${OLD_LOG_LEVEL})

    foreach(targetname gtest gmock gtest_main gmock_main)
        if(NOT TARGET ${targetname})
            continue()
        endif()
        set_target_properties(${targetname} PROPERTIES FOLDER "3rdParty Dependencies")
        # Fast math is incompatible with googletest.
        target_compile_options(${targetname} ${O3DE_TARGET_COMPILE_OPTION_DISABLE_FAST_MATH})
    endforeach()

    # Not ly_create_alias: it also registers these for auto-generated imported targets in the installer,
    # which would duplicate the find-file installed below.
    add_library(3rdParty::googletest::GTest ALIAS gtest)
    add_library(3rdParty::googletest::GMock ALIAS gmock)

    # Both are public API for anything depending on AZ::AzTest, so the installer has to ship the headers and static libs, neither of which happens by default.
    # Headers go under include/gmock_gtest/ so adding that as an include path keeps #include <gtest/gtest.h> working without exposing the rest of include/.
    FetchContent_GetProperties(googletest SOURCE_DIR googletest_source_dir)

    ly_install(DIRECTORY ${googletest_source_dir}/googletest/include/gtest DESTINATION include/gmock_gtest COMPONENT CORE)
    ly_install(DIRECTORY ${googletest_source_dir}/googlemock/include/gmock DESTINATION include/gmock_gtest COMPONENT CORE)
    ly_install(FILES ${googletest_source_dir}/LICENSE COMPONENT CORE DESTINATION include/gmock_gtest/gtest)
    ly_install(FILES ${googletest_source_dir}/LICENSE COMPONENT CORE DESTINATION include/gmock_gtest/gmock)

    # Installer mode resolves googletest through this find-file instead.
    ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findgoogletest.cmake DESTINATION cmake/3rdParty)

    set(BASE_LIBRARY_FOLDER "lib/${PAL_PLATFORM_NAME}")
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} UCONF)
        ly_install(TARGETS gtest gmock
            ARCHIVE
                DESTINATION "${BASE_LIBRARY_FOLDER}/${conf}"
                COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                CONFIGURATIONS ${conf}
        )
    endforeach()
endblock()

set(googletest_FOUND TRUE)
