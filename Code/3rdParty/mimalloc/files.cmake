#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${mimalloc_SOURCE_DIR}/src/alloc.c"
    "${mimalloc_SOURCE_DIR}/src/alloc-aligned.c"
    "${mimalloc_SOURCE_DIR}/src/alloc-posix.c"
    "${mimalloc_SOURCE_DIR}/src/arena.c"
    "${mimalloc_SOURCE_DIR}/src/bitmap.c"
    "${mimalloc_SOURCE_DIR}/src/heap.c"
    "${mimalloc_SOURCE_DIR}/src/init.c"
    "${mimalloc_SOURCE_DIR}/src/libc.c"
    "${mimalloc_SOURCE_DIR}/src/options.c"
    "${mimalloc_SOURCE_DIR}/src/os.c"
    "${mimalloc_SOURCE_DIR}/src/page.c"
    "${mimalloc_SOURCE_DIR}/src/page-map.c"
    "${mimalloc_SOURCE_DIR}/src/random.c"
    "${mimalloc_SOURCE_DIR}/src/stats.c"
    "${mimalloc_SOURCE_DIR}/src/subproc.c"
    "${mimalloc_SOURCE_DIR}/src/theap.c"
    "${mimalloc_SOURCE_DIR}/src/threadlocal.c"
    "${mimalloc_SOURCE_DIR}/src/prim/prim.c"
    "${mimalloc_SOURCE_DIR}/src/prim/prim-tls.c"
)

set_source_files_properties(${FILES} PROPERTIES LANGUAGE CXX)
