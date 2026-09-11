#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Shared by the Qt tools (AZ::ToolsCrashHandler) and by packaged game/server launchers (the
# CrashReporting gem), so this target must stay free of Qt and of host-tools-only dependencies.
ly_add_target(
    NAME SentryCrashCore STATIC
    NAMESPACE AZ
    FILES_CMAKE
        sentry_crash_core_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            include
    COMPILE_DEFINITIONS
        PUBLIC
            # Every crash-handler caller in the engine (the Editor, ProjectManager, LuaIDE, the
            # Atom tools, ...) wraps its InitCrashHandler call in #if defined(EXTERNAL_CRASH_REPORTING).
            # The Crashpad backend defines it from its own target; without the same definition here
            # those call sites compile out and this backend would silently never start.
            EXTERNAL_CRASH_REPORTING
    BUILD_DEPENDENCIES
        PUBLIC
            sentry::sentry
            AZ::AzCore
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME SentryCrashCore.Tests EXECUTABLE
        NAMESPACE AZ
        FILES_CMAKE
            sentry_crash_core_tests_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::SentryCrashCore
                AZ::AzTest
    )

    ly_add_googletest(
        NAME AZ::SentryCrashCore.Tests
        TEST_COMMAND $<TARGET_FILE:AZ::SentryCrashCore.Tests> --unittest
    )
endif()
