#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_BUILD_EXTERNAL_CRASH_HANDLER_SUPPORTED)
    return()
endif()

# The sentry-native backend replaces this gem's Crashpad plumbing (GameCrashHandler /
# GameCrashUploader) with a system component that self-initializes, so the two paths share no
# sources and are selected here rather than #ifdef'd together.
if(PAL_TRAIT_CRASH_HANDLER_BACKEND STREQUAL "SentryNative")

    # Target references are resolved lazily by ly_delayed_target_link_libraries, but CMake
    # functions are not - so pull the definition in directly if this gem is processed before
    # Code/Tools/CrashHandler. FetchContent_MakeAvailable is idempotent, so this is a no-op then.
    if(NOT COMMAND o3de_stage_crashpad_handler)
        include(${LY_ROOT_FOLDER}/cmake/3rdParty/sentry-native.cmake)
    endif()

    ly_add_target(
        NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            crashreporting_sentry_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::SentryCrashCore
                AZ::AzCore
                AZ::AzFramework
    )

    ly_add_source_properties(
        SOURCES
            Source/SentryCrashReportingModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
    ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
    ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})
    ly_create_alias(NAME ${gem_name}.Tools   NAMESPACE Gem TARGETS Gem::${gem_name})
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name})

    # Packaged launchers need crashpad_handler beside them too; the gem module lands in the same
    # binary folder as the launcher that loads it.
    o3de_stage_crashpad_handler(${gem_name})

    return()
endif()

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        crashreporting_static_files.cmake
        Platform/${PAL_PLATFORM_NAME}/crashreporting_static_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::CrashHandler
            AZ::AzCore
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/GameCrashHandler.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# Load the "Gem::${gem_name}" module in Clients and Servers
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

if(NOT LY_MONOLITHIC_GAME)

    ly_add_target(
        NAME ${gem_name}.Uploader APPLICATION
        NAMESPACE AZ
        FILES_CMAKE
            game_crash_uploader_files.cmake
            Platform/${PAL_PLATFORM_NAME}/game_crash_uploader_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::CrashUploaderSupport
    )

endif()
