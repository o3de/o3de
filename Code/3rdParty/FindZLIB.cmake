#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_resolve_3rdparty_target(3rdParty::zlib zlib_resolved)

ly_de_alias_target(3rdParty::zlib zlib_target)

if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS ${zlib_target})
endif()

if(NOT TARGET 3rdParty::ZLIB)
    add_library(3rdParty::ZLIB ALIAS ${zlib_target})
endif()

get_target_property(ZLIB_INCLUDE_DIR 3rdParty::zlib INTERFACE_INCLUDE_DIRECTORIES)
set(ZLIB_INCLUDE_DIRS "${ZLIB_INCLUDE_DIR}")
set(ZLIB_LIBRARY 3rdParty::zlib)
set(ZLIB_LIBRARIES 3rdParty::zlib)
set(ZLIB_VERSION "1.3.2")
set(ZLIB_VERSION_STRING "${ZLIB_VERSION}")
set(ZLIB_VERSION_MAJOR 1)
set(ZLIB_VERSION_MINOR 3)
set(ZLIB_VERSION_PATCH 2)
set(ZLIB_MAJOR_VERSION "${ZLIB_VERSION_MAJOR}")
set(ZLIB_MINOR_VERSION "${ZLIB_VERSION_MINOR}")
set(ZLIB_PATCH_VERSION "${ZLIB_VERSION_PATCH}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZLIB
    REQUIRED_VARS ZLIB_LIBRARY ZLIB_INCLUDE_DIR
    VERSION_VAR ZLIB_VERSION
)
