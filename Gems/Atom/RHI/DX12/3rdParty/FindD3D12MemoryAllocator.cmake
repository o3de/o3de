#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::D3D12MemoryAllocator)
    return()
endif()

block()
    # Variables inside a block are scoped to the block body.
    # Putting all of this inside a function lets us basically ensure that any variables set by us before invoking the
    # external 3rdParty CMake file do not have any effect on the outside world, and allows us not to have to save and restore anything
    # except for cache changes.

    # Part 1:  Where do you get the library from?  Make sure to inform the user of the source of the library and any patches applied.
    o3de_fetch_content(D3D12MemoryAllocator
        VERSION "v3.2.0"
        LICENSE "MIT"
        URL "https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator/archive/refs/tags/v3.2.0.tar.gz"
        URL_HASH "71d740ecb2d6cdf93b273ae571eb80097a530443993b0a5cb2bdb3cacc1548db"
        GIT "https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git"
        GIT_HASH "1d86c1130f61453634b1df85782e1fecfd59a525"
    )

    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(D3D12MemoryAllocator)

    set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)
endblock()

get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
ly_get_engine_relative_source_dir(${this_gem_root} relative_this_gem_root)

o3de_fixup_fetchcontent_targets(
    IDE_FOLDER 
        "${relative_this_gem_root}/External" 
    TARGETS 
        D3D12MemoryAllocator)

FetchContent_GetProperties(D3D12MemoryAllocator SOURCE_DIR d3d12memoryallocator_source_dir)
ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/FindD3D12MemoryAllocator.cmake DESTINATION cmake/3rdParty)
ly_install(FILES ${d3d12memoryallocator_source_dir}/include/D3D12MemAlloc.h DESTINATION include/D3D12MemoryAllocator COMPONENT CORE)
ly_install(FILES ${d3d12memoryallocator_source_dir}/LICENSE.txt DESTINATION include/D3D12MemoryAllocator COMPONENT CORE)

set(D3D12MemoryAllocator_FOUND TRUE PARENT_SCOPE)
