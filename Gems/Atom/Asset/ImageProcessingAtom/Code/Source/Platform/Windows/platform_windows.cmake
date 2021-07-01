#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# windows requires the 3rd Party ISPCTexComp library.

ly_associate_package(PACKAGE_NAME ISPCTexComp-2021.3-rev1-windows    TARGETS ISPCTexComp     PACKAGE_HASH 324fb051a549bc96571530e63c01e18a4c860db45317734d86276fe27a45f6dd)

set(LY_BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::ISPCTexComp
    )
