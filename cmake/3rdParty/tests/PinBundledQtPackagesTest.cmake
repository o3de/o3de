#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# Run with: cmake -P cmake/3rdParty/tests/PinBundledQtPackagesTest.cmake
cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/../PinBundledQtPackages.cmake")

string(RANDOM LENGTH 10 ALPHABET 0123456789abcdef test_suffix)
set(test_root "${CMAKE_CURRENT_BINARY_DIR}/o3de-qt-isolation-${test_suffix}")
set(bundled_package "${test_root}/O3DE Qt package")
set(bundled_cmake "${bundled_package}/qt/lib/cmake")
set(system_cmake "${test_root}/External Qt/lib/cmake")

foreach(package Qt6 Qt6Core Qt6CoreTools Qt6GuiTools)
    file(MAKE_DIRECTORY "${bundled_cmake}/${package}" "${system_cmake}/${package}")
    file(WRITE "${system_cmake}/${package}/${package}Config.cmake"
        "message(FATAL_ERROR \"Incorrectly loaded external ${package}\")\n")
endforeach()
file(MAKE_DIRECTORY "${system_cmake}/TestSystemDependency")
file(WRITE "${system_cmake}/TestSystemDependency/TestSystemDependencyConfig.cmake"
    "set(O3DE_TEST_SYSTEM_DEPENDENCY_FOUND TRUE)\n")

# Model Qt6Config -> Qt6Core -> Qt6CoreTools, plus another tool component.
file(WRITE "${bundled_cmake}/Qt6/Qt6Config.cmake"
    "find_package(Qt6Core CONFIG REQUIRED)\nfind_package(Qt6GuiTools CONFIG REQUIRED)\n")
file(WRITE "${bundled_cmake}/Qt6Core/Qt6CoreConfig.cmake"
    "find_package(Qt6CoreTools CONFIG REQUIRED)\nfind_package(TestSystemDependency CONFIG REQUIRED)\n")
file(WRITE "${bundled_cmake}/Qt6CoreTools/Qt6CoreToolsConfig.cmake"
    "set(O3DE_TEST_BUNDLED_CORE_TOOLS TRUE)\n")
file(WRITE "${bundled_cmake}/Qt6GuiTools/Qt6GuiToolsConfig.cmake"
    "set(O3DE_TEST_BUNDLED_GUI_TOOLS TRUE)\n")

# Reproduce a Qt Creator or CMake GUI configuration with a different Qt in
# CMAKE_PREFIX_PATH and stale component cache variables from that Qt.
set(CMAKE_PREFIX_PATH "${test_root}/External Qt")
foreach(package Qt6 Qt6Core Qt6CoreTools Qt6GuiTools)
    set("${package}_DIR" "${system_cmake}/${package}" CACHE PATH "Stale system Qt path" FORCE)
endforeach()

function(test_qt_resolution)
    o3de_pin_bundled_qt_package_configs("${bundled_package}")
    find_package(Qt6 CONFIG REQUIRED)
    foreach(package Qt6 Qt6Core Qt6CoreTools Qt6GuiTools)
        if(NOT "${${package}_DIR}" STREQUAL "${bundled_cmake}/${package}")
            message(FATAL_ERROR "${package} was not pinned to the bundled Qt")
        endif()
    endforeach()
    if(NOT O3DE_TEST_BUNDLED_CORE_TOOLS OR NOT O3DE_TEST_BUNDLED_GUI_TOOLS)
        message(FATAL_ERROR "Nested tool dependencies did not resolve from the bundle")
    endif()
    if(NOT O3DE_TEST_SYSTEM_DEPENDENCY_FOUND)
        message(FATAL_ERROR "Unrelated system dependencies must remain discoverable")
    endif()
endfunction()

test_qt_resolution()
file(REMOVE_RECURSE "${test_root}")
message(STATUS "Bundled Qt package isolation test passed")
