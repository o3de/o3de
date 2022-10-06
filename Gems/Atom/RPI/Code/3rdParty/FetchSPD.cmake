# This script encapsulates fetching SPD as a dependency and providing the
# requested targets. To use it, simply include this script from the CMakeLists.txt
# file of the target platform.

message(STATUS "started FetchSPD")

if (NOT FETCHED_SPD)
    include(FetchContent)
    FetchContent_Declare(
        SPD
        GIT_REPOSITORY https://github.com/GPUOpen-Effects/FidelityFX-SPD.git
        GIT_TAG 7c796c6d9fa6a9439e3610478148cfd742d97daf # release v2.0
        GIT_SHALLOW ON
        GIT_PROGRESS ON
    )
    FetchContent_MakeAvailable(SPD)
    set(SPD_INCLUDE_PATH ${spd_SOURCE_DIR}/ffx-spd CACHE STRING "")
    message(STATUS "SPD header files fetched: ${SPD_INCLUDE_PATH}")

    # Copy header files to a shader compiler readable directory.
    set(SPD_COPY_SOURCE_DIR ${SPD_INCLUDE_PATH})
    set(SPD_COPY_DESTINATION_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../Feature/Common/Assets/ShaderLib/3rdParty/Features/PostProcessing/ffx-spd")
    set(header_files ffx_a.h ffx_spd.h)
    foreach(file IN LISTS header_files)
        message("copying ... ${file}")
        configure_file(
            ${SPD_COPY_SOURCE_DIR}/${file}
            ${SPD_COPY_DESTINATION_DIR}/${file}
            COPYONLY)
    endforeach()
    message("Copied SPD header files to ${SPD_COPY_DESTINATION_DIR}")

    set(FETCHED_SPD ON)
endif()
