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

add_subdirectory(Code)

if (NOT CMAKE_SCRIPT_MODE_FILE)
    # only installl package when it's not in script mode
    # this has same condition as \o3de\cmake\LYPython.cmake
    ly_pip_install_local_package_editable(${CMAKE_CURRENT_LIST_DIR}/Tools Atom_RPI_Tools)
endif()
