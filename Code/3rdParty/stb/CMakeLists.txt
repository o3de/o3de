#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(stb
    VERSION "4e9b368"
    LICENSE "MIT OR Unlicense"
    URL "https://github.com/nothings/stb/archive/4e9b368bfc071e97206ea5c6adba68dd0cbf8897.tar.gz"
    URL_HASH "e60706ab084e0cae35ee7282deb4a7f283e4a28be1896d097437205da01a0683"
    GIT "https://github.com/nothings/stb.git"
    GIT_HASH "4e9b368bfc071e97206ea5c6adba68dd0cbf8897"
    PATCH_FILES "${CMAKE_CURRENT_LIST_DIR}/stb.patch"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(stb)

file(COPY
    "${stb_SOURCE_DIR}/stb_c_lexer.h"
    "${stb_SOURCE_DIR}/stb_connected_components.h"
    "${stb_SOURCE_DIR}/stb_divide.h"
    "${stb_SOURCE_DIR}/stb_ds.h"
    "${stb_SOURCE_DIR}/stb_dxt.h"
    "${stb_SOURCE_DIR}/stb_easy_font.h"
    "${stb_SOURCE_DIR}/stb_herringbone_wang_tile.h"
    "${stb_SOURCE_DIR}/stb_hexwave.h"
    "${stb_SOURCE_DIR}/stb_image_write.h"
    "${stb_SOURCE_DIR}/stb_image.h"
    "${stb_SOURCE_DIR}/stb_include.h"
    "${stb_SOURCE_DIR}/stb_leakcheck.h"
    "${stb_SOURCE_DIR}/stb_perlin.h"
    "${stb_SOURCE_DIR}/stb_rect_pack.h"
    "${stb_SOURCE_DIR}/stb_textedit.h"
    "${stb_SOURCE_DIR}/stb_tilemap_editor.h"
    "${stb_SOURCE_DIR}/stb_truetype.h"
    "${stb_SOURCE_DIR}/stb_vorbis.c"
    "${stb_SOURCE_DIR}/stb_voxel_render.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include/stb"
)

ly_add_target(
    NAME stb HEADERONLY
    NAMESPACE 3rdParty
)

target_include_directories(stb SYSTEM
    INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/Include/stb/stb_vorbis.c"
    DESTINATION Code/3rdParty/stb/Include/stb
    COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/stb
)
