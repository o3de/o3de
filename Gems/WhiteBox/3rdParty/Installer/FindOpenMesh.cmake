#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::OpenMesh)
    return()
endif()

# This file is used in the INSTALLER version of O3DE.  
# This file is included in cmake/3rdParty, which is already part of the search path for Findxxxxx.cmake files.

# The OpenMesh library is used privately inside the WhiteBox gem, so neither its headers nor its static library needs
# to actually be distributed here.  It is not expected for people to link to it, but rather use it via the WhiteBox Gem's
# public API.

# It is still worth notifying people that they are accepting a 3rd Party Library here, and what license it uses, and
# where to get it.
message(STATUS "WhiteBox Gem uses OpenMesh-11.0 (BSD-3-Clause) from https://gitlab.vci.rwth-aachen.de:9000/OpenMesh/OpenMesh.git")
message(STATUS "    - patched with ${CMAKE_CURRENT_LIST_DIR}/openmesh-o3de-11.0.patch")

# By providing both an "OpenMesh" and a "3rdParty::OpenMesh target, we stop O3DE from doing anything automatically
# itself, such as attempting to invoke some other install script or find script or complaining about a missing target.
add_library(OpenMesh IMPORTED INTERFACE GLOBAL)
add_library(3rdParty::OpenMesh ALIAS OpenMesh)

# Same thing to future-proof 3rdParty::OpenMeshTools
add_library(OpenMeshTools IMPORTED INTERFACE GLOBAL)
add_library(3rdParty::OpenMeshTools ALIAS OpenMeshTools)

# notify O3DE That we have satisfied the OpenMesh find_package requirements.
set(OpenMesh_FOUND TRUE)
