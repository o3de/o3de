#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

find_library(IOKIT_LIBRARY IOKit)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${IOKIT_LIBRARY}
)
