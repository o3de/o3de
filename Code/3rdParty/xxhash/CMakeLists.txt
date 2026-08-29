#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(xxhash
    VERSION "0.8.3"
    LICENSE "BSD-2-Clause"
    URL "https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.3.tar.gz"
    URL_HASH "aae608dfe8213dfd05d909a57718ef82f30722c392344583d3f39050c7f29a80"
    GIT "https://github.com/Cyan4973/xxHash.git"
    GIT_HASH "v0.8.3"
)
FetchContent_MakeAvailable(xxhash)
FetchContent_GetProperties(xxhash SOURCE_DIR xxhash_source_dir)

ly_add_3rdparty_target(
    NAME xxhash STATIC
    INCLUDE_DIRECTORIES
        PUBLIC
            "${xxhash_source_dir}"
            Include
)
