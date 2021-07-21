#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify download and find_package commands for 
# packages which apply to all platforms (usually header-only)
# individual platforms can enumerate packages in for example
# cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake

#include the platform-specific 3rd party packages.
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

set(LY_PAL_PACKAGE_FILE_NAME ${CMAKE_CURRENT_LIST_DIR}/${pal_dir}/BuiltInPackages_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
include(${LY_PAL_PACKAGE_FILE_NAME})

# add the above file to the ALLFILES list, so that they show up in IDEs
set(ALLFILES ${ALLFILES} ${LY_PAL_PACKAGE_FILE_NAME})