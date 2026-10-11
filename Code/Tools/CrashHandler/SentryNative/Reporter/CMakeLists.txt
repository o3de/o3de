#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME SentryCrashReporterCore STATIC
    NAMESPACE AZ
    FILES_CMAKE
        sentry_crash_reporter_core_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PUBLIC
            sentry::sentry
            AZ::AzCore
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME SentryCrashReporterCore.Tests EXECUTABLE
        NAMESPACE AZ
        FILES_CMAKE
            sentry_crash_reporter_core_tests_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::SentryCrashReporterCore
                AZ::AzTest
    )

    ly_add_googletest(
        NAME AZ::SentryCrashReporterCore.Tests
        TEST_COMMAND $<TARGET_FILE:AZ::SentryCrashReporterCore.Tests> --unittest
    )
endif()

ly_add_target(
    NAME SentryCrashReporter APPLICATION
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    FILES_CMAKE
        sentry_crash_reporter_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::SentryCrashReporterCore
            sentry::sentry
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
)

ly_add_dependencies(Editor SentryCrashReporter)

# SentryCrashReporter is an APPLICATION, so its output directory is the binary folder the Editor
# and the other tools are launched from - the place sentry-native expects to find the handler.
o3de_stage_crashpad_handler(SentryCrashReporter)
