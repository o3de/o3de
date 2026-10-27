#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${mimalloc_SOURCE_DIR}/src/prim/os.c"
    "${mimalloc_SOURCE_DIR}/src/prim/prim.c"
    "${mimalloc_SOURCE_DIR}/src/prim/prim-tls.c"
    "${mimalloc_SOURCE_DIR}/src/util/libc.c"
    "${mimalloc_SOURCE_DIR}/src/util/options.c"
    "${mimalloc_SOURCE_DIR}/src/util/random.c"
    "${mimalloc_SOURCE_DIR}/src/util/stats.c"
    "${mimalloc_SOURCE_DIR}/src/util/threadlocal.c"
    "${mimalloc_SOURCE_DIR}/src/arena/arena.c"
    "${mimalloc_SOURCE_DIR}/src/arena/arena-alloc.c"
    "${mimalloc_SOURCE_DIR}/src/arena/arena-page.c"
    "${mimalloc_SOURCE_DIR}/src/arena/bitmap-chunk.c"
    "${mimalloc_SOURCE_DIR}/src/arena/bitmap.c"
    "${mimalloc_SOURCE_DIR}/src/alloc/alloc.c"
    "${mimalloc_SOURCE_DIR}/src/alloc/alloc-generic.c"
    "${mimalloc_SOURCE_DIR}/src/alloc/alloc-aligned.c"
    "${mimalloc_SOURCE_DIR}/src/alloc/alloc-posix.c"
    "${mimalloc_SOURCE_DIR}/src/alloc/page.c"
    "${mimalloc_SOURCE_DIR}/src/alloc/page-queue.c"
    "${mimalloc_SOURCE_DIR}/src/heap/heap.c"
    "${mimalloc_SOURCE_DIR}/src/heap/init.c"
    "${mimalloc_SOURCE_DIR}/src/heap/page-map.c"
    "${mimalloc_SOURCE_DIR}/src/heap/subproc.c"
    "${mimalloc_SOURCE_DIR}/src/heap/theap.c"
    "${mimalloc_SOURCE_DIR}/src/profile/sample-guarded.c"
    "${mimalloc_SOURCE_DIR}/src/profile/sample-profile.c"
    "${mimalloc_SOURCE_DIR}/src/profile/pprof.c"
)

set_source_files_properties(${FILES} PROPERTIES LANGUAGE CXX)
