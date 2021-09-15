#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(LY_MONOLITHIC_GAME) # Do not use OpenImageIO in monolithic game
    return()
endif()

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        3rdParty::ilmbase
)
