include(FetchContent)

if (TARGET 3rdParty::OpenMesh)
    if (TARGET OpenMeshCore OR TARGET OpenMeshCoreStatic)
        return()
    endif()
endif()

function(GetOpenMesh)
    set(OPENMESH_GIT_REPO "https://github.com/o3de/openmesh.git")
    set(OPENMESH_GIT_TAG "8078b5bd01b568e61c6b8dd84af8673e7d9e6cae")

    message(STATUS "WhiteBox Gem uses OpenMesh 11.0 (BSD-3-Clause) ${OPENMESH_GIT_REPO}")
    message(STATUS "    - This is a mirror of: https://gitlab.vci.rwth-aachen.de:9000/OpenMesh/OpenMesh.git")

    set(OLD_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    set(ORIGINAL_CMAKE_SUPPRESS_DEVELOPER_WARNINGS ${CMAKE_SUPPRESS_DEVELOPER_WARNINGS})
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)

    # ── INSTALLED ENGINE: stubs exist, no real targets ────────────────────────
    # ── INSTALLED ENGINE: stubs exist, no real targets ────────────────────────
    if (TARGET OpenMesh AND NOT TARGET OpenMeshCore AND NOT TARGET OpenMeshCoreStatic)

        FetchContent_GetProperties(OpenMesh)
        if(NOT openmesh_POPULATED)
            FetchContent_Declare(
                OpenMesh
                GIT_REPOSITORY ${OPENMESH_GIT_REPO}
                GIT_TAG        ${OPENMESH_GIT_TAG}
                GIT_SHALLOW
            )
            FetchContent_Populate(OpenMesh)
        endif()

        # --- apply the engine patch (unchanged) ---
        if(DEFINED LY_ROOT_FOLDER)
            set(_om_patch "${LY_ROOT_FOLDER}/cmake/3rdParty/openmesh-o3de-11.0.patch")
        else()
            set(_om_patch "D:/O3DE/25.10.2/cmake/3rdParty/openmesh-o3de-11.0.patch")
        endif()
        if(EXISTS "${_om_patch}")
            execute_process(
                COMMAND git apply --check --ignore-whitespace "${_om_patch}"
                WORKING_DIRECTORY "${openmesh_SOURCE_DIR}"
                RESULT_VARIABLE _om_patch_can_apply OUTPUT_QUIET ERROR_QUIET)
            if(_om_patch_can_apply EQUAL 0)
                message(STATUS "    - applying O3DE OpenMesh patch: ${_om_patch}")
                execute_process(
                    COMMAND git apply --ignore-whitespace "${_om_patch}"
                    WORKING_DIRECTORY "${openmesh_SOURCE_DIR}")
            endif()
        endif()

        # --- compile OpenMesh Core into a real static library ---
        # The stub provides no compiled code, so OpenMesh's non-template symbols
        # (ArrayKernel, IOManager, IO readers/writers, etc.) are unresolved at link.
        # Build the Core sources ourselves under a UNIQUE target name so there's no
        # collision with the engine's stub OpenMesh / OpenMeshTools targets.
        file(GLOB_RECURSE _om_core_sources CONFIGURE_DEPENDS
            "${openmesh_SOURCE_DIR}/src/OpenMesh/Core/*.cc")

        add_library(OpenMeshCore_o3de STATIC ${_om_core_sources})
        target_include_directories(OpenMeshCore_o3de PUBLIC "${openmesh_SOURCE_DIR}/src")
        target_compile_definitions(OpenMeshCore_o3de
            PUBLIC  OM_STATIC_BUILD
            PRIVATE WIN32_LEAN_AND_MEAN _USE_MATH_DEFINES)
        target_compile_options(OpenMeshCore_o3de PRIVATE
            ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS}
            ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

        get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
        ly_get_engine_relative_source_dir(${this_gem_root} relative_this_gem_root)
        set_property(TARGET OpenMeshCore_o3de PROPERTY FOLDER "${relative_this_gem_root}/External")

        # Wire headers + compiled lib into the stub so anything depending on
        # 3rdParty::OpenMesh links the real symbols.
        set_target_properties(OpenMesh PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${openmesh_SOURCE_DIR}/src"
            INTERFACE_COMPILE_DEFINITIONS "OM_STATIC_BUILD"
            INTERFACE_LINK_LIBRARIES "OpenMeshCore_o3de")
        if (TARGET OpenMeshTools)
            set_target_properties(OpenMeshTools PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${openmesh_SOURCE_DIR}/src")
        endif()

        set(OpenMesh_FOUND TRUE PARENT_SCOPE)
        set(CMAKE_MESSAGE_LOG_LEVEL ${OLD_LOG_LEVEL})
        set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)
        set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ${ORIGINAL_CMAKE_SUPPRESS_DEVELOPER_WARNINGS} CACHE BOOL "" FORCE)
        return()
    endif()

    # ── SOURCE ENGINE: no stubs, run the full build ───────────────────────────
    set(OPENMESH_BUILD_SHARED OFF)
    set(OPENMESH_DOCS OFF)
    set(OPENMESH_BUILD_UNIT_TESTS OFF)
    set(DISABLE_QMAKE_BUILD ON)
    set(BUILD_APPS OFF)
    set(VCI_NO_LIBRARY_INSTALL ON)
    set(VCI_COMMON_DO_NOT_COPY_POST_BUILD ON)

    FetchContent_Declare(
        OpenMesh
        GIT_REPOSITORY ${OPENMESH_GIT_REPO}
        GIT_TAG        ${OPENMESH_GIT_TAG}
        GIT_SHALLOW
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(OpenMesh)

    set(CMAKE_MESSAGE_LOG_LEVEL ${OLD_LOG_LEVEL})
    set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)

    set(OPENMESH_TARGETS OpenMeshCore OpenMeshTools OpenMeshCoreStatic OpenMeshToolsStatic)
    foreach(OpenMesh_Target ${OPENMESH_TARGETS})
        if (NOT TARGET ${OpenMesh_Target})
            continue()
        endif()
        target_compile_options(${OpenMesh_Target}
            ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS}
            ${O3DE_COMPILE_OPTION_EXPORT_SYMBOLS}
            ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})
        target_compile_definitions(${OpenMesh_Target}
            PRIVATE WIN32_LEAN_AND_MEAN)
        get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
        ly_get_engine_relative_source_dir(${this_gem_root} relative_this_gem_root)
        set_property(TARGET ${OpenMesh_Target} PROPERTY FOLDER "${relative_this_gem_root}/External")
    endforeach()

    set(OpenMesh_Primary_Target OpenMeshCore)
    if (TARGET OpenMeshCoreStatic)
        set(OpenMesh_Primary_Target OpenMeshCoreStatic)
    endif()

    add_library(3rdParty::OpenMesh ALIAS ${OpenMesh_Primary_Target})
    add_library(3rdParty::OpenMeshTools ALIAS OpenMeshTools)

    ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/FindOpenMesh.cmake DESTINATION cmake/3rdParty)

    set(OpenMesh_FOUND TRUE PARENT_SCOPE)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ${ORIGINAL_CMAKE_SUPPRESS_DEVELOPER_WARNINGS} CACHE BOOL "" FORCE)
endfunction()

GetOpenMesh()
unset(GetOpenMesh)