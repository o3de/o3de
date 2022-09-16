#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
ly_set(CMAKE_WARN_DEPRECATED ON)

set(O3DE_EXTERNAL_SUBDIRS "" CACHE STRING "Additional list of subdirectory to recurse into via the cmake `add_subdirectory()` command. \
    The subdirectories are included after the restricted platform folders have been visited by a call to `add_subdirectory(restricted/\${restricted_platform})`")

ly_set(LY_ROOT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
