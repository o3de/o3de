#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
    if(conf STREQUAL debug)
        string(APPEND VCPKG_CONFIGURATION_MAPPING "        <VcpkgConfiguration Condition=\"'$(Configuration)' == '${conf}'\">Debug</VcpkgConfiguration>\n")
    else()
        string(APPEND VCPKG_CONFIGURATION_MAPPING "        <VcpkgConfiguration Condition=\"'$(Configuration)' == '${conf}'\">Release</VcpkgConfiguration>\n")
    endif()
endforeach()
  
configure_file("${CMAKE_CURRENT_LIST_DIR}/Directory.Build.props" "${CMAKE_BINARY_DIR}/Directory.Build.props" @ONLY)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CodeAnalysis.ruleset" DESTINATION "${CMAKE_BINARY_DIR}")
