#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::assimp)
    return()
endif()

# This file is used in the INSTALLER version of O3DE.  
# This file is included in cmake/3rdParty, which is already part of the search path for Findxxxxx.cmake files.

# The Assimp library is used privately inside the SDKWrapper tool, so neither its headers nor its static library needs
# to actually be distributed here.  It is not expected for people to link to it, but rather use it via the SDKWrapper Tool's
# public API.

# It is still worth notifying people that they are accepting a 3rd Party Library here, and what license it uses, and
# where to get it.
message(STATUS "SDKWrapper Tool uses Assimp v6.0.2 (Custom BSD-3-Clause) from https://github.com/assimp/assimp.git")

# By providing both an "assimp" and a "3rdParty::assimp target, we stop O3DE from doing anything automatically
# itself, such as attempting to invoke some other install script or find script or complaining about a missing target.
add_library(assimp IMPORTED INTERFACE GLOBAL)
add_library(3rdParty::assimp ALIAS assimp)

# notify O3DE That we have satisfied the Assimp find_package requirements.
set(assimp_FOUND TRUE)
