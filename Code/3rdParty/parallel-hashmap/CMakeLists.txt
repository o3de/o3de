#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(parallel-hashmap
    VERSION "2.1.0"
    LICENSE "Apache-2.0"
    URL "https://github.com/greg7mdp/parallel-hashmap/archive/refs/tags/v2.1.0.tar.gz"
    URL_HASH "d2c94659a5301c94cd5bbd6cf4b01c1dd635bba15c76cc188cdd82b928136838"
    GIT "https://github.com/greg7mdp/parallel-hashmap.git"
    GIT_HASH "2a6e2aca3b129b907d3a108ecc562ef9ddc478b4"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(parallel-hashmap)

file(
    COPY "${parallel-hashmap_SOURCE_DIR}/parallel_hashmap"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_add_target(
    NAME parallel-hashmap HEADERONLY
    NAMESPACE 3rdParty
    INCLUDE_DIRECTORIES
        INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

set_target_properties(parallel-hashmap PROPERTIES SYSTEM TRUE)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/parallel-hashmap
)
