# This script encapsulates fetching RGA as a dependency and providing the
# requested targets. To use it, simply include this script from the CMakeLists.txt
# file of the target platform.

message(STATUS "started FetchRGA")

if (NOT FETCHED_RGA)
    include(FetchContent)
    if (WIN32)
        FetchContent_Declare(
            RGA
            URL https://github.com/GPUOpen-Tools/radeon_gpu_analyzer/releases/download/2.6.2/rga-windows-x64-2.6.2.zip
            DOWNLOAD_EXTRACT_TIMESTAMP true
        )
    else()
        FetchContent_Declare(
            RGA
            URL https://github.com/GPUOpen-Tools/radeon_gpu_analyzer/releases/download/2.6.2/rga-linux-2.6.2.tgz
            DOWNLOAD_EXTRACT_TIMESTAMP true
        )
    endif()
    FetchContent_MakeAvailable(RGA)
    message("rga will be stored in ${rga_SOURCE_DIR}")

    set(FETCHED_RGA ON)
endif()