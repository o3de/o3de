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

ly_add_external_target(
    NAME Crashpad
    VERSION ""
    3RDPARTY_ROOT_DIRECTORY ${LY_ROOT_FOLDER}/Tools/Crashpad
    INCLUDE_DIRECTORIES
        include
        include/third_party/mini_chromium/mini_chromium/
)

ly_add_external_target(
    NAME Handler
    PACKAGE Crashpad
    VERSION ""
    3RDPARTY_ROOT_DIRECTORY ${LY_ROOT_FOLDER}/Tools/Crashpad
    INCLUDE_DIRECTORIES
        include
        include/third_party/mini_chromium/mini_chromium
        include/third_party/getopt
)
