/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QAction>
#include <QtTest>
#include <QThread>
#include <QtConcurrentRun>


#include <Tests/UI/UIFixture.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/ResetSettingsDialog.h>
#include <EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>

#include <EMotionFX/Source/Actor.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/UI/ModalPopupHandler.h>

namespace EMotionFX
{

    TEST_F(UIFixture, CanResetFromFileMenu)
    {
        /*
        Test Case: C16302179:
        Can reset from file menu.
        Fills a blank workspace with: an Actor (and instance); AnimGraph; MotionSet and Motion, then resets the workspace via the file menu -> reset.
        The "Reset" action blocks control flow waiting for user input, so a second thread is opened to click the "Discard Changes" button.
        */

        RecordProperty("test_case_id", "C16302179");

        const AZStd::string motionAsset("@gemroot:EMotionFX@/Code/Tests/TestAssets/Rin/rin_idle.motion");
        const AZStd::string createAnimGraphCmd("CreateAnimGraph");
        const AZStd::string motionSetName("TestMotionSet");
        const AZStd::string createMotionSetCmd("CreateMotionSet -motionSetID 42 -name " + motionSetName);
        const AZStd::string createMotionCmd("ImportMotion -filename " + motionAsset);

        // Verify initial conditions
        ASSERT_EQ(GetActorManager().GetNumActors(), 0) << "Expected to see no actors yet";
        ASSERT_EQ(GetActorManager().GetNumActorInstances(), 0) << "Expected to see no actor instances";
        ASSERT_EQ(GetAnimGraphManager().GetNumAnimGraphs(), 0) << "Anim graph manager should contain 0 anim graphs.";
        const size_t oldNumMotionSets = GetMotionManager().GetNumMotionSets();
        ASSERT_EQ(oldNumMotionSets, 1) << "Expected only the default motion set";
        ASSERT_EQ(GetMotionManager().GetNumMotions(), 0) << "Expected exactly zero motions";

        // Create Actor, AnimGraph, MotionSet and Motion
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 2, "SampleActor");
        ActorInstance::Create(actorAsset->GetActor());
        {
            AZStd::string result;
            ASSERT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(createAnimGraphCmd, result)) << result.c_str();
            ASSERT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(createMotionSetCmd, result)) << result.c_str();
            ASSERT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(createMotionCmd, result)) << result.c_str();
        }

        // Verify pre-conditions
        ASSERT_EQ(GetActorManager().GetNumActors(), 1) << "Expected to see one actor";
        ASSERT_EQ(GetActorManager().GetNumActorInstances(), 1) << "Expected to see one actor instance";
        ASSERT_EQ(GetAnimGraphManager().GetNumAnimGraphs(), 1) << "Anim graph manager should contain 1 anim graph.";
        ASSERT_EQ(EMotionFX::GetMotionManager().GetNumMotionSets(), oldNumMotionSets + 1) << "Expected the default and the newly created motion set";
        ASSERT_EQ(GetMotionManager().GetNumMotions(), 1) << "Expected exactly one motion";

        // Trigger reset from menu
        QAction* reset = GetNamedAction(EMStudio::GetMainWindow(), "&Reset");
        ASSERT_TRUE(reset) << "Could not find 'reset' action";

        {
            ModalPopupHandler handler;
            handler.WaitForPopupPressDialogButton<QDialog*>(QDialogButtonBox::Discard);
            // Click File -> Reset which will show the modal dialog box
            reset->trigger();
        }

        // Find and accept Reset Settings Dialog box
        auto dialogBox = qobject_cast<EMStudio::ResetSettingsDialog*>(FindTopLevelWidget("EMFX.MainWindow.ResetSettingsDialog"));
        ASSERT_TRUE(dialogBox) << "Could not find ResetSettingsDialog widget";
        dialogBox->accept();

        // Ensure workspace was cleared
        EXPECT_EQ(GetActorManager().GetNumActors(), 0) << "Expected to see no actors";
        EXPECT_EQ(GetActorManager().GetNumActorInstances(), 0) << "Expected to see no actor instances";
        EXPECT_EQ(EMotionFX::GetAnimGraphManager().GetNumAnimGraphs(), 0) << "Anim graph manager should contain 0 anim graphs.";
        ASSERT_EQ(GetMotionManager().GetNumMotionSets(), 1) << "Expected only the default motion set";
        EXPECT_EQ(GetMotionManager().GetNumMotions(), 0) << "Expected exactly zero motions";

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    } // Test: CanResetFromFileMenu

} // namespace EMotionFX
