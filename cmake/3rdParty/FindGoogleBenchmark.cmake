#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(GoogleBenchmark_FOUND TRUE)

if(TARGET 3rdParty::GoogleBenchmark)
    return()
endif()

if(INSTALLED_ENGINE)
    block()
        find_package(Threads REQUIRED)

        set(BASE_LIBRARY_FOLDER "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")

        add_library(GoogleBenchmark::benchmark STATIC IMPORTED GLOBAL)
        set_target_properties(GoogleBenchmark::benchmark PROPERTIES
            IMPORTED_LOCATION
                "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}benchmark${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG
                "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}benchmark${CMAKE_STATIC_LIBRARY_SUFFIX}"
            INTERFACE_LINK_LIBRARIES
                "Threads::Threads"
        )

        if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            set_property(TARGET GoogleBenchmark::benchmark APPEND PROPERTY INTERFACE_LINK_LIBRARIES shlwapi)
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            set_property(TARGET GoogleBenchmark::benchmark APPEND PROPERTY INTERFACE_LINK_LIBRARIES rt)
        endif()

        add_library(3rdParty::GoogleBenchmark INTERFACE IMPORTED GLOBAL)
        target_link_libraries(3rdParty::GoogleBenchmark INTERFACE GoogleBenchmark::benchmark)
        target_compile_definitions(3rdParty::GoogleBenchmark INTERFACE HAVE_BENCHMARK BENCHMARK_STATIC_DEFINE)
        ly_target_include_system_directories(
            TARGET 3rdParty::GoogleBenchmark
            INTERFACE "${LY_ROOT_FOLDER}/include/googlebenchmark"
        )
    endblock()

    return()
endif()

block()
    set(ADDITIONAL_FETCHCONTENT_FLAGS EXCLUDE_FROM_ALL)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.25")
        list(APPEND ADDITIONAL_FETCHCONTENT_FLAGS SYSTEM)
    endif()

    o3de_fetch_content(googlebenchmark
        VERSION "v1.7.0"
        LICENSE "Apache-2.0"
        URL "https://github.com/google/benchmark/archive/refs/tags/v1.7.0.tar.gz"
        URL_HASH "3aff99169fa8bdee356eaa1f691e835a6e57b1efeadb8a0f9f228531158246ac"
        GIT "https://github.com/google/benchmark.git"
        GIT_HASH "361e8d1cfe0c6c36d30b39f1b61302ece5507320"
        ${ADDITIONAL_FETCHCONTENT_FLAGS}
    )

    set(BUILD_SHARED_LIBS OFF)
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF)
    set(BENCHMARK_ENABLE_ASSEMBLY_TESTS OFF)
    set(BENCHMARK_ENABLE_DOXYGEN OFF)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    set(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_ENABLE_LIBPFM OFF)
    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_WERROR OFF)
    set(BENCHMARK_FORCE_WERROR OFF)
    set(BENCHMARK_INSTALL_DOCS OFF)
    set(BENCHMARK_USE_BUNDLED_GTEST OFF)

    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})
    set(CMAKE_POLICY_VERSION_MINIMUM "3.10")

    FetchContent_MakeAvailable(googlebenchmark)

    foreach(targetname benchmark benchmark_main)
        if(TARGET ${targetname})
            set_target_properties(${targetname} PROPERTIES FOLDER "3rdParty Dependencies")
            target_compile_options(${targetname} ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS})
        endif()
    endforeach()

    add_library(3rdParty::GoogleBenchmark INTERFACE IMPORTED GLOBAL)
    target_link_libraries(3rdParty::GoogleBenchmark INTERFACE benchmark::benchmark)
    target_compile_definitions(3rdParty::GoogleBenchmark INTERFACE HAVE_BENCHMARK BENCHMARK_STATIC_DEFINE)

    FetchContent_GetProperties(googlebenchmark
        SOURCE_DIR googlebenchmark_source_dir
        BINARY_DIR googlebenchmark_binary_dir
    )
    file(MAKE_DIRECTORY "${googlebenchmark_binary_dir}/include")
    ly_target_include_system_directories(
        TARGET 3rdParty::GoogleBenchmark
        INTERFACE
            "${googlebenchmark_source_dir}/include"
            "${googlebenchmark_binary_dir}/include"
    )

    ly_install(
        DIRECTORY "${googlebenchmark_source_dir}/include/benchmark"
        DESTINATION include/googlebenchmark
        COMPONENT CORE
    )
    ly_install(
        FILES "${googlebenchmark_source_dir}/LICENSE"
        DESTINATION include/googlebenchmark
        COMPONENT CORE
    )

    set(BASE_LIBRARY_FOLDER "lib/${PAL_PLATFORM_NAME}")
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} UCONF)
        ly_install(TARGETS benchmark ARCHIVE
            DESTINATION "${BASE_LIBRARY_FOLDER}/${conf}"
            COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
            CONFIGURATIONS ${conf}
        )
    endforeach()
endblock()
