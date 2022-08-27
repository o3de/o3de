# This script encapsulates fetching FSR2 as a dependency and providing the
# requested targets. To use it, simply include this script from the CMakeLists.txt
# file of the target platform.

if(NOT TARGET ffx_fsr2_api_x64)
    # Save existing compile options
    get_property(
        compile_options
        DIRECTORY
        PROPERTY COMPILE_OPTIONS
    )

    # Query the PAL directory to determine which RHI backends should be built
    o3de_pal_dir(
        pal_dir
        ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
        "${gem_restricted_path}"
        "${gem_path}"
        "${gem_parent_relative_path}")

    include(${pal_dir}/fsr2_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

    set(FSR2_API_LIBS "ffx_fsr2_api_x64")
    # The ATOMFSR2_VK and ATOMFSR2_DX12 PAL traits determine if we should compile the VK and DX12 FSR2 backends

    # TODO: FIX FSR2 VK
    if(OFF)
        set(FFX_FSR2_API_VK ON CACHE BOOL "")
        list(APPEND FSR2_API_LIBS "ffx_fsr2_api_vk_x64")
    else()
        set(FFX_FSR2_API_VK OFF CACHE BOOL "")
    endif()

    if(PAL_TRAIT_ATOMFSR2_DX12)
        set(FFX_FSR2_API_DX12 ON CACHE BOOL "")
        list(APPEND FSR2_API_LIBS "ffx_fsr2_api_dx12_x64")
    else()
        set(FFX_FSR2_API_DX12 OFF CACHE BOOL "")
    endif()

    if(PAL_TRAIT_COMPILER_ID_LOWERCASE STREQUAL "gcc")
        set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -w)
    elseif(PAL_TRAIT_COMPILER_ID_LOWERCASE STREQUAL "clang")
        set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -Wno-everything)
    elseif(PAL_TRAIT_COMPILER_ID_LOWERCASE STREQUAL "msvc")
        set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS /W3)
    endif()

    include(FetchContent)
    # NOTE: FSR2 does not yet build on linux because of a tool used to compile and embed shaders
    # that is not yet open sourced by the team at AMD.
    FetchContent_Declare(
        FSR2
        GIT_REPOSITORY https://github.com/jeremyong-az/FidelityFX-FSR2.git
        GIT_TAG d5e2d05
        GIT_SUBMODULES
        GIT_SUBMODULES_RECURSE OFF
        GIT_SHALLOW ON
        GIT_PROGRESS ON
    )
    FetchContent_MakeAvailable(FSR2)

    # FSR2's cmake project doesn't properly attach the include directory to the target
    set(FSR2_INCLUDE_PATH ${fsr2_SOURCE_DIR}/src/ffx-fsr2-api CACHE STRING "")
    message(STATUS "FSR2 Fetched: ${FSR2_INCLUDE_PATH}")
    target_include_directories(ffx_fsr2_api_x64 PUBLIC ${FSR2_INCLUDE_PATH})

    # Attach several defines to indicate which FSR2 RHI backends are enabled
    if(FFX_FSR2_API_VK)
        set(RHI_CODE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../RHI/Vulkan/Code")
        o3de_pal_dir(vk_pal_dir ${RHI_CODE_PATH}/Include/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
        target_compile_definitions(ffx_fsr2_api_vk_x64 PUBLIC O3DE_FSR2_VK=1)
        target_include_directories(
            ffx_fsr2_api_vk_x64
            PUBLIC
            # FSR2 needs to compile using the vulkan header internally, so explicitly add this
            # include directory containing the Glad emitted vulkan headers
            ${RHI_CODE_PATH}/Include/Atom/RHI.Loader/Glad
            ${vk_pal_dir}
        )
        target_link_libraries(ffx_fsr2_api_vk_x64 PUBLIC 3rdParty::glad_vulkan)
        add_library(3rdParty::ffx_fsr2_api_vk_x64 ALIAS ffx_fsr2_api_vk_x64)
    else()
        target_compile_definitions(ffx_fsr2_api_x64 PUBLIC O3DE_FSR2_VK=0)
    endif()
    if(FFX_FSR2_API_DX12)
        target_compile_definitions(ffx_fsr2_api_dx12_x64 PUBLIC O3DE_FSR2_DX12=1)
        add_library(3rdParty::ffx_fsr2_api_dx12_x64 ALIAS ffx_fsr2_api_dx12_x64)
    else()
        target_compile_definitions(ffx_fsr2_api_x64 PUBLIC O3DE_FSR2_DX12=0)
    endif()
    add_library(3rdParty::ffx_fsr2_api_x64 ALIAS ffx_fsr2_api_x64)

    set(FSR2_GEM_ROOT "Gems/Atom/RHI/External/FidelityFX_FSR2")
    set(FSR2_TARGETS "ffx_fsr2_api_dx12_x64;ffx_fsr2_api_vk_x64;shader_permutations_dx12;shader_permutations_vk;ffx_fsr2_api_x64")
    foreach(FSR2_TARGET ${FSR2_TARGETS})
        if(TARGET ${FSR2_TARGET})
            set_property(TARGET ${FSR2_TARGET} PROPERTY FOLDER "${FSR2_GEM_ROOT}")
        endif()
    endforeach()

    set_property(DIRECTORY PROPERTY COMPILE_OPTIONS ${compile_options})
endif()