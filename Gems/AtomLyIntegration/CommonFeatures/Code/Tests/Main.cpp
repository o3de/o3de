/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Component.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzTest/AzTest.h>
#include <AzTest/GemTestEnvironment.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

#include <CoreLights/EditorAreaLightComponent.h>

#include <QApplication>

class AtomLyIntegrationHook : public AZ::Test::GemTestEnvironment
{
    // AZ::Test::GemTestEnvironment overrides ...
    void AddGemsAndComponents() override;
    AZ::ComponentApplication* CreateApplicationInstance() override;
    void PostSystemEntityActivate() override;

public:
    AtomLyIntegrationHook() = default;
    ~AtomLyIntegrationHook() override = default;
};

void AtomLyIntegrationHook::AddGemsAndComponents()
{
    AddDynamicModulePaths({ "LmbrCentral.Editor" });
    AddComponentDescriptors({
        AZ::Render::AreaLightComponent::CreateDescriptor(),
        AZ::Render::EditorAreaLightComponent::CreateDescriptor()});
}

AZ::ComponentApplication* AtomLyIntegrationHook::CreateApplicationInstance()
{
    // Using ToolsTestApplication to have AzFramework and AzToolsFramework components.
    return aznew UnitTest::ToolsTestApplication("EditorAtomLyIntegration");
}

void AtomLyIntegrationHook::PostSystemEntityActivate()
{
    AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequestBus::Events::DisableSaveOnFinalize);
}

// required to support running integration tests with Qt
AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    QApplication app(argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({ new AtomLyIntegrationHook });
    int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN();
