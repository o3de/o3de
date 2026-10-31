#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_resolve_3rdparty_target(3rdParty::PNG png_resolved)

get_target_property(PNG_INCLUDE_DIR 3rdParty::PNG INTERFACE_INCLUDE_DIRECTORIES)
set(PNG_INCLUDE_DIRS "${PNG_INCLUDE_DIR}")
set(PNG_LIBRARY 3rdParty::PNG)
set(PNG_LIBRARIES 3rdParty::PNG)
set(PNG_VERSION "1.6.59")
set(PNG_VERSION_STRING "${PNG_VERSION}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PNG
    REQUIRED_VARS PNG_LIBRARY PNG_INCLUDE_DIR
    VERSION_VAR PNG_VERSION
)
