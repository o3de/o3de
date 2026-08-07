#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
set(MESHOPTIMIZER_TARGET meshoptimizer)

if (TARGET 3rdParty::${MESHOPTIMIZER_TARGET})
    return()
endif()
block()
    o3de_fetch_content(${MESHOPTIMIZER_TARGET}
        VERSION "v1.2"
        LICENSE "MIT"
        URL "https://github.com/zeux/meshoptimizer/archive/refs/tags/v1.2.tar.gz"
        URL_HASH "e40f71b809cdf3361b9a4def85fd44534e8733ce29d4b943c145b76859e4c2b4"
        GIT "https://github.com/zeux/meshoptimizer.git"
        GIT_HASH "9d9890c73011d75920af614485296d1e03e95448"
    )
    set(MESHOPT_INSTALL OFF)
    FetchContent_MakeAvailable(meshoptimizer)
endblock()

get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
ly_get_engine_relative_source_dir(${this_gem_root} relative_this_gem_root)
set_property(TARGET ${MESHOPTIMIZER_TARGET} PROPERTY FOLDER "${relative_this_gem_root}/External")

add_library(3rdParty::${MESHOPTIMIZER_TARGET} ALIAS ${MESHOPTIMIZER_TARGET})
ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findmeshoptimizer.cmake DESTINATION cmake/3rdParty)

set(meshoptimizer_FOUND TRUE PARENT_SCOPE)