#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        3rdParty::OpenImageIO
        3rdParty::ilmbase
)

# [GFX-TODO] Add macro defintion in OpenImageIO 3rd party find cmake file
set(LY_COMPILE_DEFINITIONS
    PRIVATE
        OPEN_IMAGE_IO_ENABLED
)
