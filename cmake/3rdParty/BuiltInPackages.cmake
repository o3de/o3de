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