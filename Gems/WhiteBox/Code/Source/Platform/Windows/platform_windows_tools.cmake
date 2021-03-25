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

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev1-windows TARGETS OpenMesh PACKAGE_HASH 1c1df639358526c368e790dfce40c45cbdfcfb1c9a041b9d7054a8949d88ee77)

    set(LY_BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::OpenMesh)
endif()
