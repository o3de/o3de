/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include <QApplication>

#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>

#include <Maestro/Editor/ITrackViewSequenceManager.h>
#include <Maestro/Editor/MaestroEditorServices.h>
#include <Maestro/Editor/MaestroEditorSystemComponent.h>

namespace
{
    class MaestroEditorTestEnvironment
        : public UnitTest::TraceBusHook
    {
    };

    class CompetingEditorSequenceSystemLifecycle
        : public AzToolsFramework::IEditorSequenceSystemLifecycle
    {
    public:
        void OnEditorGameEngineInitialized(bool) override {}
        void OnEditorGameEngineAttached() override {}
        void OnEditorGameEngineShutdown() override {}
        void OnEditorUpdate() override {}
        void OnBeginSceneOpen() override {}
        void OnBeginGameMode() override {}
        void OnGameModeStarted() override {}
        void OnGameModeStopped() override {}
        void OnEndGameMode(bool) override {}
        void OnSimulationModeChanged(bool) override {}
    };
}

TEST(MaestroEditorApiTests, SequenceManagerInterfaceHasExpectedTypeId)
{
    EXPECT_EQ(
        azrtti_typeid<ITrackViewSequenceManager>(),
        AZ::TypeId("{F49A421A-04C6-4F2A-BC73-BE205CD33019}"));
}

TEST(MaestroEditorApiTests, MaestroRegistersAsTheSingleEditorSequenceLifecycleProvider)
{
    ASSERT_EQ(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);

    Maestro::MaestroEditorSystemComponent component;
    component.Activate();
    EXPECT_NE(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);

    CompetingEditorSequenceSystemLifecycle competingProvider;
    EXPECT_FALSE(AzToolsFramework::EditorSequenceSystemLifecycleInterface::Register(&competingProvider).IsSuccess());

    component.Deactivate();
    EXPECT_EQ(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);
}

TEST(MaestroEditorApiTests, RepeatedActivationDoesNotLeaveAnInterfaceOrEditorEventHandler)
{
    ASSERT_EQ(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);
    const size_t initialHandlerCount = AzToolsFramework::EditorEvents::Bus::GetTotalNumOfEventHandlers();
    Maestro::MaestroEditorSystemComponent component;
    for (int iteration = 0; iteration < 20; ++iteration)
    {
        component.Activate();
        EXPECT_NE(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);
        EXPECT_EQ(AzToolsFramework::EditorEvents::Bus::GetTotalNumOfEventHandlers(), initialHandlerCount + 1);
        component.Deactivate();
        EXPECT_EQ(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);
        EXPECT_EQ(Maestro::Editor::GetAnimation(), nullptr);
        EXPECT_EQ(Maestro::Editor::GetSequenceManager(), nullptr);
        EXPECT_EQ(AzToolsFramework::EditorEvents::Bus::GetTotalNumOfEventHandlers(), initialHandlerCount);
        AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::NotifyRegisterViews);
    }
}

AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    QApplication application(argc, argv);
    AZ::Test::ApplyGlobalParameters(&argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({ new MaestroEditorTestEnvironment });
    const int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN()
