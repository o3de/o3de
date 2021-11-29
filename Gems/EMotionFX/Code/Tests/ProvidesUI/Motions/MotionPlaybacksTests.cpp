/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QTableWidget>
#include <QtTest>
#include <gtest/gtest.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
#include <Tests/ProvidesUI/AnimGraph/PreviewMotionFixture.h>
#include <Tests/UI/UIFixture.h>

namespace EMotionFX
{
    TEST_F(PreviewMotionFixture, MotionCanBePlayed)
    {
        /*
        Test Case: C1797320
        Motion playbacks with no corruption to animation
        Motion can be played in the open gl viewport.
        */

        RecordProperty("test_case_id", "C1797320");

        // Check motion exists in motions window.
        auto motionWindowPlugin = static_cast<EMStudio::MotionWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionWindowPlugin != nullptr) << "Could not find the Motion Window Plugin";
        QTableWidget* table = qobject_cast<QTableWidget*>(FindTopLevelWidget("EMFX.MotionListWindow.MotionTable"));
        ASSERT_TRUE(table) << "Could not find the Motion Table";
        EXPECT_EQ(GetMotionManager().GetNumMotions(), 1) << "Expected to have no motions for the Manager";
        EXPECT_EQ(table->rowCount(), 1) << "Expected the table to have no rows yet";

        // Create actor and actor instance.
        const char* actorFilename = "@engroot@/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.actor";
        AZStd::unique_ptr<EMotionFX::Actor> m_actor = EMotionFX::GetImporter().LoadActor(actorFilename);
        EXPECT_TRUE(m_actor.get() != nullptr) << "Actor not loaded.";
        EMotionFX::ActorInstance* m_actorInstance = ActorInstance::Create(m_actor.get());
        EXPECT_TRUE(m_actorInstance != nullptr) << "Actor instance not created.";
        AZStd::string result;
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(AZStd::string{ "Select -actorInstanceID " } + AZStd::to_string(m_actorInstance->GetID()), result)) << result.c_str();

        // Double click on motion in motions window.
        QTableWidgetItem* item = table->item(0, 0);
        const auto rect = table->visualItemRect(item);
        QTest::mouseClick(table->viewport(), Qt::LeftButton, {}, rect.center());
        QTest::mouseDClick(table->viewport(), Qt::LeftButton, {}, rect.center());

        // Check motion has been selected in render window.
        EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(static_cast<uint32>(EMStudio::RenderPlugin::CLASS_ID));
        EXPECT_TRUE(plugin != nullptr) << "Render plugin not found.";
        EMStudio::RenderPlugin* renderPlugin = static_cast<EMStudio::RenderPlugin*>(plugin);
        CommandSystem::SelectionList* renderPluginSelectionList = renderPlugin->GetCurrentSelection();
        EXPECT_TRUE(renderPluginSelectionList->GetSingleMotion()) << "Motion not selected in render window.";
        EXPECT_TRUE(renderPluginSelectionList->GetSingleMotion()->GetFileName() == m_motionFileName) << "Motion file name does not match.";
    }
}
