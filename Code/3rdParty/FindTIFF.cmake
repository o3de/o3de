#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_resolve_3rdparty_target(3rdParty::TIFF tiff_resolved)

if(NOT TARGET TIFF::TIFF)
    ly_de_alias_target(3rdParty::TIFF tiff_target)
    add_library(TIFF::TIFF ALIAS ${tiff_target})
    add_library(TIFF::tiff ALIAS ${tiff_target})
    add_library(3rdParty::tiff ALIAS ${tiff_target})
endif()

get_target_property(TIFF_INCLUDE_DIR 3rdParty::TIFF INTERFACE_INCLUDE_DIRECTORIES)
set(TIFF_INCLUDE_DIRS "${TIFF_INCLUDE_DIR}")
set(TIFF_LIBRARY TIFF::TIFF)
set(TIFF_LIBRARIES TIFF::TIFF)
set(TIFF_VERSION "4.7.2")
set(TIFF_VERSION_STRING "${TIFF_VERSION}")
set(TIFF_CXX_FOUND FALSE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TIFF
    REQUIRED_VARS TIFF_LIBRARY TIFF_INCLUDE_DIR
    VERSION_VAR TIFF_VERSION
    HANDLE_COMPONENTS
)
