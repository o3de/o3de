#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(pugixml
    VERSION "1.16"
    LICENSE "MIT"
    URL "https://github.com/zeux/pugixml/archive/c8033ce9d039e7f9d134877c363397b3cfe20816.tar.gz"
    URL_HASH "4ae8941bc2c19e176c13153e5f53cefa8498aacab0063823b1efdebc719797d9"
    GIT "https://github.com/zeux/pugixml.git"
    GIT_HASH "c8033ce9d039e7f9d134877c363397b3cfe20816"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(pugixml)

file(COPY
    "${pugixml_SOURCE_DIR}/src/pugixml.hpp"
    "${pugixml_SOURCE_DIR}/src/pugiconfig.hpp"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME pugixml STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

set_target_properties(pugixml PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

target_compile_options(pugixml PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/pugixml
)
