#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(FetchContent)

if (TARGET 3rdParty::Manifold)
    return()
endif()

block()
    set(MANIFOLD_GIT_REPO "https://github.com/elalish/manifold.git")
    set(MANIFOLD_GIT_TAG "cc8a7f66d7d5a560da94346258c5b546af27811e")
    # Keep the human-readable tag to show the user in the console
    set(MANIFOLD_DISPLAY_VERSION "v3.5.1")

    message(STATUS "WhiteBox Gem uses Manifold ${MANIFOLD_DISPLAY_VERSION} (Apache-2.0) ${MANIFOLD_GIT_REPO}")
    message(STATUS "    - Manifold provides the CSG boolean operations (Api::MeshBoolean).")


    set(OLD_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    set(ORIGINAL_CMAKE_SUPPRESS_DEVELOPER_WARNINGS ${CMAKE_SUPPRESS_DEVELOPER_WARNINGS})
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)

    # manifold build options - library only, no tests/bindings/extras
    set(MANIFOLD_TEST OFF CACHE BOOL "" FORCE)
    set(MANIFOLD_CBIND OFF CACHE BOOL "" FORCE)
    set(MANIFOLD_PYBIND OFF CACHE BOOL "" FORCE)
    set(MANIFOLD_PAR OFF CACHE BOOL "" FORCE)
    set(MANIFOLD_DEBUG OFF CACHE BOOL "" FORCE)
    # Build manifold as a static library without forcing BUILD_SHARED_LIBS
    # globally - a plain (non-cache) variable set in this block scope is
    # inherited by the FetchContent subdirectory only.
    set(BUILD_SHARED_LIBS OFF)

    o3de_fetch_content(manifold
        VERSION "${MANIFOLD_DISPLAY_VERSION}"
        LICENSE "Apache-2.0"
        URL "https://github.com/elalish/manifold/archive/refs/tags/v3.5.1.tar.gz"
        URL_HASH "1b42f28d7c1c6d07df7244ca22cb7d82de980899001969c9985c2662a77eb43c"
        GIT "${MANIFOLD_GIT_REPO}"
        GIT_HASH "${MANIFOLD_GIT_TAG}"
        EXCLUDE_FROM_ALL
    )

    o3de_fetch_content(Clipper2
        VERSION "46f6391"
        LICENSE "BSL-1.0"
        URL "https://github.com/AngusJohnson/Clipper2/archive/46f639177fe418f9689e8ddb74f08a870c71f5b4.tar.gz"
        URL_HASH "73f5783e0c88299976334f48e3e356c756b96212c652ecc8bceec3bd99455bcb"
        GIT "https://github.com/AngusJohnson/Clipper2.git"
        GIT_HASH "46f639177fe418f9689e8ddb74f08a870c71f5b4"
        SOURCE_SUBDIR CPP
    )

    FetchContent_MakeAvailable(manifold)

    # O3DE promotes some off-by-default MSVC warnings to errors globally
    # (/we4265, /we5233 + /WX in Configurations_msvc.cmake) and manifold trips
    # two of them. Patch the fetched sources in place; idempotent, so safe to
    # run on every configure (same approach as the old mcut <chrono> patch).
    #
    # Patch 1: C4265 - CsgNode has virtual functions but no virtual destructor.
    set(manifold_csg_tree_header "${manifold_SOURCE_DIR}/src/csg_tree.h")
    if (EXISTS "${manifold_csg_tree_header}")
        file(READ "${manifold_csg_tree_header}" manifold_csg_tree_contents)
        string(FIND "${manifold_csg_tree_contents}" "virtual ~CsgNode" manifold_csg_dtor_found)
        if (manifold_csg_dtor_found EQUAL -1)
            string(REPLACE
                "class CsgNode : public std::enable_shared_from_this<CsgNode> {\n public:\n"
                "class CsgNode : public std::enable_shared_from_this<CsgNode> {\n public:\n  virtual ~CsgNode() = default; // patched by WhiteBox gem (see FindManifold.cmake): fixes MSVC C4265\n"
                manifold_csg_tree_contents "${manifold_csg_tree_contents}")
            file(WRITE "${manifold_csg_tree_header}" "${manifold_csg_tree_contents}")
            message(STATUS "WhiteBox Gem: patched manifold csg_tree.h to add virtual ~CsgNode (MSVC C4265 fix)")
        endif()
    endif()

    # Patch 2: C5233 - explicit lambda captures 'this' and 'vert' unused in
    # the sequential branch of SplitPinchedVerts (edge_op.cpp).
    set(manifold_edge_op_source "${manifold_SOURCE_DIR}/src/edge_op.cpp")
    if (EXISTS "${manifold_edge_op_source}")
        file(READ "${manifold_edge_op_source}" manifold_edge_op_contents)
        string(FIND "${manifold_edge_op_contents}" "ForVert(i, [&halfedgeProcessed](int current)" manifold_lambda_patch_found)
        if (manifold_lambda_patch_found EQUAL -1)
            string(REPLACE
                "vertProcessed[vert] = true;\n        ForVert(i, [this, &halfedgeProcessed, vert](int current) {"
                "vertProcessed[vert] = true;\n        // patched by WhiteBox gem (see FindManifold.cmake): fixes MSVC C5233\n        ForVert(i, [&halfedgeProcessed](int current) {"
                manifold_edge_op_contents "${manifold_edge_op_contents}")
            file(WRITE "${manifold_edge_op_source}" "${manifold_edge_op_contents}")
            message(STATUS "WhiteBox Gem: patched manifold edge_op.cpp to drop unused lambda captures (MSVC C5233 fix)")
        endif()
    endif()

    set(CMAKE_MESSAGE_LOG_LEVEL ${OLD_LOG_LEVEL})
    set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ${ORIGINAL_CMAKE_SUPPRESS_DEVELOPER_WARNINGS} CACHE BOOL "" FORCE)

    if (NOT TARGET manifold)
        message(FATAL_ERROR "WhiteBox Gem: failed to fetch/configure the manifold library")
    endif()

    target_compile_options(manifold PRIVATE
        ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS}
        ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

    # Clipper2 is fetched by manifold as a dependency (used for CrossSection)
    if (TARGET Clipper2)
        target_compile_options(Clipper2 PRIVATE ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS})
    endif()

    # Strip the bogus `-lm` entry from Clipper2 targets on Windows
    # (doesn't exist on Windows; leaks into link when using clang with MSVC ABI).
    if (WIN32)
        foreach (clipper2_target Clipper2 Clipper2Z)
            if (TARGET ${clipper2_target})
                foreach (link_prop LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
                    get_target_property(clipper2_libs ${clipper2_target} ${link_prop})
                    if (clipper2_libs)
                        list(REMOVE_ITEM clipper2_libs "-lm" "m")
                        set_target_properties(${clipper2_target} PROPERTIES ${link_prop} "${clipper2_libs}")
                    endif()
                endforeach()
            endif()
        endforeach()
    endif()

    # O3DE promotes some off-by-default MSVC warnings to errors via /weNNNN.
    # /W0 (from O3DE_COMPILE_OPTION_DISABLE_WARNINGS) does NOT cancel per-warning
    # /we flags - only an explicit /wd does. Disable the ones manifold trips over:
    #   C4265 - class has virtual functions but non-virtual destructor (csg_tree.h)
    #   C5233 - explicit lambda capture not used (edge_op.cpp)
    if (MSVC)
        set(manifold_msvc_warning_overrides /WX- /wd4265 /wd5233)
        target_compile_options(manifold PRIVATE ${manifold_msvc_warning_overrides})
        if (TARGET Clipper2)
            target_compile_options(Clipper2 PRIVATE ${manifold_msvc_warning_overrides})
        endif()
    endif()

    if (COMMAND ly_get_engine_relative_source_dir)
        get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
        ly_get_engine_relative_source_dir(${this_gem_root} relative_this_gem_root)
        set_property(TARGET manifold PROPERTY FOLDER "${relative_this_gem_root}/External")
        if (TARGET Clipper2)
            set_property(TARGET Clipper2 PROPERTY FOLDER "${relative_this_gem_root}/External")
        endif()
    endif()

    # interface wrapper that carries the headers alongside the static library
    add_library(ManifoldInterface INTERFACE IMPORTED GLOBAL)
    target_include_directories(ManifoldInterface INTERFACE
        "${manifold_SOURCE_DIR}/include"
        "${manifold_BINARY_DIR}/include") # generated manifold/version.h
    target_link_libraries(ManifoldInterface INTERFACE manifold)
endblock()

add_library(3rdParty::Manifold ALIAS ManifoldInterface)

set(Manifold_FOUND TRUE)

if (COMMAND ly_install)
    ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/FindManifold.cmake DESTINATION cmake/3rdParty)
endif()
