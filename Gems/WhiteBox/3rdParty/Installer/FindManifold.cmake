#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::Manifold)
    return()
endif()

# This file is used in the INSTALLER version of O3DE.
# This file is included in cmake/3rdParty, which is already part of the search path for Findxxxxx.cmake files.

# The Manifold library is used privately inside the WhiteBox gem (for the CSG boolean operations exposed
# via Api::MeshBoolean), so neither its headers nor its static library needs to be distributed here.
# It is not expected for people to link to it, but rather use it via the WhiteBox Gem's public API.

# It is still worth notifying people that they are accepting a 3rd Party Library here, what license it
# uses, and where to get it.
message(STATUS "WhiteBox Gem uses Manifold (Apache-2.0) from https://github.com/elalish/manifold.git")

# By providing both a "ManifoldInterface" and a "3rdParty::Manifold" target, we stop O3DE from doing anything
# automatically itself, such as attempting to invoke some other install script or find script or
# complaining about a missing target.
add_library(ManifoldInterface IMPORTED INTERFACE GLOBAL)
add_library(3rdParty::Manifold ALIAS ManifoldInterface)

# notify O3DE that we have satisfied the Manifold find_package requirements.
set(Manifold_FOUND TRUE)
