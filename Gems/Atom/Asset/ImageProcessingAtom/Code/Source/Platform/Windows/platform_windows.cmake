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

# windows requires the 3rd Party ISPCTexComp library.

ly_associate_package(PACKAGE_NAME ISPCTexComp-2021.3-rev1-windows    TARGETS ISPCTexComp     PACKAGE_HASH 324fb051a549bc96571530e63c01e18a4c860db45317734d86276fe27a45f6dd)

set(LY_BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::ISPCTexComp
    )
