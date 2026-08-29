#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(cityhash
    VERSION "f5dc5414"
    LICENSE "MIT"
    URL "https://github.com/google/cityhash/archive/f5dc54147fcce12cefd16548c8e760d68ac04226.tar.gz"
    URL_HASH "22c24af73a3fec9402e3a183f2dee87d71f13a81859b0406472e671fb7d13a69"
    GIT "https://github.com/google/cityhash.git"
    GIT_HASH "f5dc54147fcce12cefd16548c8e760d68ac04226"
)
FetchContent_MakeAvailable(cityhash)
FetchContent_GetProperties(cityhash SOURCE_DIR cityhash_source_dir)

ly_add_3rdparty_target(
    NAME cityhash STATIC
    INCLUDE_DIRECTORIES
        PUBLIC "${cityhash_source_dir}/src"
        PRIVATE Source
)
