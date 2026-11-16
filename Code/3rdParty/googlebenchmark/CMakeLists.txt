#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(googlebenchmark
    VERSION "1.10.0"
    LICENSE "Apache-2.0"
    URL "https://github.com/google/benchmark/archive/refs/tags/v1.10.0.tar.gz"
    URL_HASH "acc5c7c0fbed855c4aaa2d7c3c36fb82b0c15c28f4e151e3f679c2fc0aa404af"
    GIT "https://github.com/google/benchmark.git"
    GIT_HASH "a6b4a9f00b36c19af9997d68a1cf6e0b5b3d032c"
    PATCH_FILES "${CMAKE_CURRENT_LIST_DIR}/googlebenchmark.patch"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(googlebenchmark)

file(COPY
    "${googlebenchmark_SOURCE_DIR}/include/benchmark"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

find_package(Threads REQUIRED)

include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckLibraryExists)

set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
file(READ "${googlebenchmark_SOURCE_DIR}/cmake/pthread_affinity.cpp" pthread_affinity_source)

if(CMAKE_CROSSCOMPILING)
    check_cxx_source_compiles("${pthread_affinity_source}" O3DE_BENCHMARK_HAS_PTHREAD_AFFINITY)
else()
    check_cxx_source_runs("${pthread_affinity_source}" O3DE_BENCHMARK_HAS_PTHREAD_AFFINITY)
endif()

unset(CMAKE_REQUIRED_LIBRARIES)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "QNX")
    check_library_exists(rt shm_open "" O3DE_BENCHMARK_HAS_LIB_RT)
endif()

o3de_disable_warnings()

ly_add_target(
    NAME GoogleBenchmark STATIC
    NAMESPACE 3rdParty
    OUTPUT_NAME benchmark
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE "${googlebenchmark_SOURCE_DIR}/src"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            BENCHMARK_VERSION="v1.10.0"
            HAVE_STD_REGEX
            HAVE_STEADY_CLOCK
            $<$<BOOL:${O3DE_BENCHMARK_HAS_PTHREAD_AFFINITY}>:BENCHMARK_HAS_PTHREAD_AFFINITY>
        PUBLIC
            HAVE_BENCHMARK
            BENCHMARK_STATIC_DEFINE
    BUILD_DEPENDENCIES
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
)

set_target_properties(GoogleBenchmark PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

# Keep exception handling private so consumers retain their own compiler settings.
set(exception_options ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})
list(REMOVE_ITEM exception_options PRIVATE PUBLIC INTERFACE)

target_compile_options(GoogleBenchmark PRIVATE ${exception_options})

if(WIN32)
    ly_target_link_libraries(GoogleBenchmark PRIVATE shlwapi)
elseif(O3DE_BENCHMARK_HAS_LIB_RT)
    ly_target_link_libraries(GoogleBenchmark PRIVATE rt)
endif()

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/googlebenchmark
)
