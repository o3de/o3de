/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <QApplication>

namespace UnitTest
{
    // Handle asserts
    class ToolsFrameworkHook
        : public AZ::Test::ITestEnvironment
    {
    public:
        void SetupEnvironment() override
        {
        }

        void TeardownEnvironment() override
        {
        }
    };

} // namespace UnitTest 

AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    AzQtComponents::PrepareQtPaths();
    QApplication app(argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({ new UnitTest::ToolsFrameworkHook });
    int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN();
