/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/SimulatedObjectHelpers.h>
#include "EMotionFX/CommandSystem/Source/CommandManager.h"
#include "EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h"
#include "EMotionFX/Source/Actor.h"
#include "EMotionFX/Source/Node.h"
#include "EMotionFX/Source/Skeleton.h"
#include "EMotionStudio/EMStudioSDK/Source/EMStudioManager.h"
#include "Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h"
#include <QtWidgets/QApplication>

#include <Editor/SimulatedObjectModel.h>
#include <AzTest/AzTest.h>

#include <Tests/UI/UIFixture.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/TestActorAssets.h>

namespace EMotionFX
{
    using SimulatedObjectModelTestsFixture = UIFixture;
    TEST_F(SimulatedObjectModelTestsFixture, CanUndoAddSimulatedObjectAndSimulatedJointWithChildren)
    {
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 3, "simulatedObjectModelTestActor");
        const Actor* actor = actorAsset->GetActor();

        EMotionFX::SimulatedObjectWidget* simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
        ASSERT_TRUE(simulatedObjectWidget) << "Simulated Object plugin not loaded";
        simulatedObjectWidget->ActorSelectionChanged(actorAsset->GetActor());

        SimulatedObjectModel* model = simulatedObjectWidget->GetSimulatedObjectModel();

        EXPECT_EQ(model->rowCount(), 0) << "Failed to add the simulated object to the model";

        AZStd::string result;

        // Add one simulated object with no joints
        MCore::CommandGroup commandGroup(AZStd::string::format("Add simulated object"));
        EMotionFX::SimulatedObjectHelpers::AddSimulatedObject(actor->GetID(), "testSimulatedObject", &commandGroup);
        EMotionFX::SimulatedObjectHelpers::AddSimulatedJoints({}, actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), false, &commandGroup);
        ASSERT_TRUE(EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        EXPECT_EQ(model->rowCount(), 1) << "Failed to add the simulated object to the model";
        EXPECT_STREQ(model->index(0, 0).data().toString().toUtf8().data(), "testSimulatedObject");

        // Add another simulated object with 3 joints
        commandGroup = MCore::CommandGroup(AZStd::string::format("Add simulated object and joints"));
        EMotionFX::SimulatedObjectHelpers::AddSimulatedObject(actor->GetID(), "testSimulatedObject2", &commandGroup);
        CommandSimulatedObjectHelpers::AddSimulatedJoints(actor->GetID(), {0}, actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), true, &commandGroup);
        ASSERT_TRUE(EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        EXPECT_EQ(model->rowCount(), 2) << "Failed to add the simulated object to the model";
        EXPECT_STREQ(model->index(1, 0).data().toString().toUtf8().data(), "testSimulatedObject2");

        // Undo the second add
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_EQ(model->rowCount(), 1) << "Failed to remove the second simulated object from the model";

        // Undo the first add
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_EQ(model->rowCount(), 0) << "Failed to remove the first simulated object from the model";

        model->SetActor(nullptr); // Reset the model as otherwise when we destroy the plugin it will still try to use the actor that isn't valid anymore.
    }
} // namespace EMotionFX
