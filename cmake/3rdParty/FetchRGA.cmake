# This script encapsulates fetching RGA as a dependency and providing the
# requested targets. To use it, simply include this script from the CMakeLists.txt
# file of the target platform.

message(STATUS "started FetchRGA")

if (NOT FETCHED_RGA)
    include(FetchContent)
    FetchContent_Declare(
        RGA
        URL https://github.com/GPUOpen-Tools/radeon_gpu_analyzer/releases/download/2.6.2/rga-windows-x64-2.6.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )

    FetchContent_MakeAvailable(RGA)
    message("rga.exe will be stored in ${rga_SOURCE_DIR}")

    set(FETCHED_RGA ON)
endif()