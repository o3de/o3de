#
# Copyright (c) Contributors to the Open 3D Engine Project
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
