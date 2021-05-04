#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
ly_set(CMAKE_WARN_DEPRECATED ON)

set(LY_EXTERNAL_SUBDIRS "" CACHE STRING "Additional list of subdirectory to recurse into via the cmake `add_subdirectory()` command. \
    The subdirectories are included after the restricted platform folders have been visited by a call to `add_subdirectory(restricted/\${restricted_platform})`")

ly_set(LY_ROOT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})