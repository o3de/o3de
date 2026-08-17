#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(TARGET 3rdParty::antlr4)
    return()
endif()

block(SCOPE_FOR VARIABLES)
    set(ANTLR_BUILD_CPP_TESTS OFF)
    set(ANTLR_BUILD_SHARED OFF)
    set(ANTLR_BUILD_STATIC ON)
    set(DISABLE_WARNINGS ON)
    set(TRACE_ATN OFF)
    set(WITH_STATIC_CRT OFF)

    o3de_fetch_content(antlr4
        VERSION "4.13.2"
        LICENSE "BSD-3-Clause"
        URL "https://github.com/antlr/antlr4/archive/8e6fd9147b3c9d36b60e2b6656871a55227efb1b.tar.gz" # 2026-01-01
        URL_HASH "c52fb2a90ae082ab5b5ec6b41f6c0e66f691962e4b4f772580e5da678fe51fed"
        GIT "https://github.com/antlr/antlr4.git"
        GIT_HASH "8e6fd9147b3c9d36b60e2b6656871a55227efb1b"
        PATCH_FILES "${CMAKE_CURRENT_LIST_DIR}/antlr4.patch"
        SOURCE_SUBDIR runtime/Cpp
        DOWNLOAD_NO_PROGRESS ON
        EXCLUDE_FROM_ALL
    )

    FetchContent_MakeAvailable(antlr4)

    add_library(3rdParty::antlr4 ALIAS antlr4)

    get_target_property(_antlr4_interface_includes antlr4 INTERFACE_INCLUDE_DIRECTORIES)
    if(_antlr4_interface_includes)
        set_target_properties(antlr4 PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_antlr4_interface_includes}")
    endif()

    target_compile_options(antlr4 PRIVATE
        ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS}
        ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS}
    )

    cmake_path(RELATIVE_PATH CMAKE_CURRENT_LIST_DIR BASE_DIRECTORY "${LY_ROOT_FOLDER}" OUTPUT_VARIABLE relative_source_root)
    set_property(TARGET antlr4 PROPERTY FOLDER "${relative_source_root}")

    FetchContent_GetProperties(antlr4 SOURCE_DIR antlr4_source_dir)
    ly_install(FILES
        "${antlr4_source_dir}/LICENSE.txt"
        DESTINATION Code/Tools/ShaderCompiler/3rdParty/antlr4
        COMPONENT CORE
    )
endblock()
