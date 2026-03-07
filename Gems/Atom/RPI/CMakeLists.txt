#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_gem_setup("Atom_RPI")

# when compiling from source, use the 3rdParty/FindOpenMesh.cmake file
# do not use ly_add_external_target_path here, as this would copy that cmake file
# for the installer.  We have our own installer cmake file.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty)

add_subdirectory(Code)
add_subdirectory(Tools)

