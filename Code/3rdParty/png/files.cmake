#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Include/png.h"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/pngconf.h"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/pnglibconf.h"
    "${png_SOURCE_DIR}/png.c"
    "${png_SOURCE_DIR}/pngerror.c"
    "${png_SOURCE_DIR}/pngget.c"
    "${png_SOURCE_DIR}/pngmem.c"
    "${png_SOURCE_DIR}/pngpread.c"
    "${png_SOURCE_DIR}/pngread.c"
    "${png_SOURCE_DIR}/pngrio.c"
    "${png_SOURCE_DIR}/pngrtran.c"
    "${png_SOURCE_DIR}/pngrutil.c"
    "${png_SOURCE_DIR}/pngset.c"
    "${png_SOURCE_DIR}/pngtrans.c"
    "${png_SOURCE_DIR}/pngwio.c"
    "${png_SOURCE_DIR}/pngwrite.c"
    "${png_SOURCE_DIR}/pngwtran.c"
    "${png_SOURCE_DIR}/pngwutil.c"
)

if(png_arm_neon)
    list(APPEND FILES
        "${png_SOURCE_DIR}/arm/arm_init.c"
        "${png_SOURCE_DIR}/arm/filter_neon_intrinsics.c"
        "${png_SOURCE_DIR}/arm/palette_neon_intrinsics.c"
    )
elseif(png_intel_sse)
    list(APPEND FILES
        "${png_SOURCE_DIR}/intel/filter_sse2_intrinsics.c"
        "${png_SOURCE_DIR}/intel/intel_init.c"
    )
endif()
