#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Include/tiff.h"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/tiffconf.h"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/tiffio.h"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/tiffvers.h"
    "${CMAKE_CURRENT_BINARY_DIR}/tif_config.h"
    "${tiff_SOURCE_DIR}/libtiff/tif_aux.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_close.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_codec.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_color.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_compress.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_dir.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_dirinfo.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_dirread.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_dirwrite.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_dumpmode.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_error.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_extension.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_fax3.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_fax3sm.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_flush.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_getimage.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_hash_set.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_jbig.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_jpeg_12.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_jpeg.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_lerc.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_luv.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_lzma.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_lzw.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_next.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_ojpeg.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_open.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_packbits.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_pixarlog.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_predict.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_print.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_read.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_strip.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_swab.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_thunder.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_tile.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_version.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_warning.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_webp.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_write.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_zip.c"
    "${tiff_SOURCE_DIR}/libtiff/tif_zstd.c"
)

if(WIN32)
    list(APPEND FILES
        "${tiff_SOURCE_DIR}/libtiff/tif_win32.c"
        "${tiff_SOURCE_DIR}/libtiff/tif_win32_versioninfo.rc"
    )
else()
    list(APPEND FILES "${tiff_SOURCE_DIR}/libtiff/tif_unix.c")
endif()
