#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify download and find_package commands for 
# packages which apply to all platforms (usually header-only)
# individual platforms can enumerate packages in for example
# cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake

#include the platform-specific 3rd party packages.
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${O3DE_ENGINE_RESTRICTED_PATH}" "${LY_ROOT_FOLDER}")
set(LY_PAL_PACKAGE_FILE_NAME ${pal_dir}/BuiltInPackages_${PAL_PLATFORM_NAME_LOWERCASE}${LY_ARCHITECTURE_NAME_EXTENSION}.cmake)
include(${LY_PAL_PACKAGE_FILE_NAME})

# add the above file to the ALLFILES list, so that they show up in IDEs
set(ALLFILES ${ALLFILES} ${LY_PAL_PACKAGE_FILE_NAME})

# temporary compatibility: 
# Some 3p libraries may still refer to zlib as "3rdParty::zlib" instead of
# the correct "3rdParty::ZLIB" (Case difference).  Until those libraries are updated
# we alias the casing here.  This also provides backward compatibility for Gems that use 3rdParty::zlib
# that are not part of the core O3DE repo.

if (NOT O3DE_SCRIPT_ONLY)
    ly_download_associated_package(ZLIB)
    find_package(ZLIB)
else()
    add_library(3rdParty::ZLIB IMPORTED INTERFACE GLOBAL)
endif()

add_library(3rdParty::zlib ALIAS 3rdParty::ZLIB)
