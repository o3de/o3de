#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${minizip_SOURCE_DIR}/compat/ioapi.c"
    "${minizip_SOURCE_DIR}/compat/unzip.c"
    "${minizip_SOURCE_DIR}/compat/zip.c"
    "${minizip_SOURCE_DIR}/mz_crypt.c"
    "${minizip_SOURCE_DIR}/mz_os.c"
    "${minizip_SOURCE_DIR}/mz_strm_buf.c"
    "${minizip_SOURCE_DIR}/mz_strm_mem.c"
    "${minizip_SOURCE_DIR}/mz_strm_split.c"
    "${minizip_SOURCE_DIR}/mz_strm_zlib.c"
    "${minizip_SOURCE_DIR}/mz_strm.c"
    "${minizip_SOURCE_DIR}/mz_zip_rw.c"
    "${minizip_SOURCE_DIR}/mz_zip.c"
)

if(WIN32)
    list(APPEND FILES
        "${minizip_SOURCE_DIR}/mz_os_win32.c"
        "${minizip_SOURCE_DIR}/mz_strm_os_win32.c"
    )
else()
    list(APPEND FILES
        "${minizip_SOURCE_DIR}/mz_os_posix.c"
        "${minizip_SOURCE_DIR}/mz_strm_os_posix.c"
    )
endif()
