/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzTest/AzTest.h>

#include <QApplication>

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

AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    QApplication app(argc, argv);
    auto styleManager = AZStd::make_unique<AzQtComponents::StyleManager>(&app);
    AZ::IO::FixedMaxPath engineRootPath;
    {
        AZ::ComponentApplication componentApplication(argc, argv);
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "SystemComponentsDeactivated");
        AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "ConsoleUnavailable");
        AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "SettingsRegistryUnavailable");
        AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "SystemAllocatorPendingDestruction");
    }
    AZ::AllocatorManager::Instance().GarbageCollect();
    styleManager->initialize(&app, engineRootPath);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({ DEFAULT_UNIT_TEST_ENV, new ToolsFrameworkHook });
    int result = RUN_ALL_TESTS();
    styleManager.release();
    return result;
}

#if defined(HAVE_BENCHMARK)
AZ_BENCHMARK_HOOK();
#else
IMPLEMENT_TEST_EXECUTABLE_MAIN();
#endif
