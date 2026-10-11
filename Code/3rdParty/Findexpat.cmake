#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

if(NOT TARGET 3rdParty::expat)
    o3de_resolve_3rdparty_target(3rdParty::expat expat_resolved)
endif()

get_target_property(expat_target 3rdParty::expat ALIASED_TARGET)

if(NOT TARGET expat::expat)
    add_library(expat::expat ALIAS ${expat_target})
endif()

if(NOT TARGET EXPAT::EXPAT)
    add_library(EXPAT::EXPAT ALIAS ${expat_target})
endif()

get_target_property(EXPAT_INCLUDE_DIRS 3rdParty::expat INTERFACE_INCLUDE_DIRECTORIES)
set(EXPAT_INCLUDE_DIR "${EXPAT_INCLUDE_DIRS}")
set(expat_INCLUDE_DIR "${EXPAT_INCLUDE_DIRS}")
set(EXPAT_LIBRARY 3rdParty::expat)
set(EXPAT_LIBRARIES "${EXPAT_LIBRARY}")
set(expat_LIBRARY "${EXPAT_LIBRARY}")
set(expat_VERSION 2.7.3)
set(EXPAT_VERSION "${expat_VERSION}")
set(EXPAT_VERSION_STRING "${expat_VERSION}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(expat
    REQUIRED_VARS EXPAT_LIBRARIES EXPAT_INCLUDE_DIRS
    VERSION_VAR expat_VERSION
)
set(EXPAT_FOUND "${expat_FOUND}")
