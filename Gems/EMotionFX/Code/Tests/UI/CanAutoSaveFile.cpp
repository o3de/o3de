/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <QtTest>

#include <Tests/UI/UIFixture.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanAutoSaveAnimGraph)
    {
        RecordProperty("test_case_id", "C15192424");

        // Cache the anim graph manager
        AnimGraphManager& animGraphManager = GetAnimGraphManager();
        EMStudio::GetMainWindow()->ApplicationModeChanged("AnimGraph");

        // Load Rin anim graph.
        const char* rinGraph = "@engroot@/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin.animgraph";
        const AZStd::string rinGraphPath = ResolvePath(rinGraph);
        AZStd::string command = AZStd::string::format("LoadAnimGraph -filename \"%s\"", rinGraphPath.c_str());
        AZStd::string result;
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, result)) << result.c_str();

        // Expect the rin graph get loaded.
        AnimGraph* graphBeforeSave = animGraphManager.FindAnimGraphByFileName(rinGraphPath.c_str());
        EXPECT_TRUE(graphBeforeSave) << "Rin graph should be loaded";

        // Set the anim graph file dirty flag, so it will get picked up in autosave.
        graphBeforeSave->SetDirtyFlag(true);

        // Trigger auto save.
        EMStudio::GetMainWindow()->OnAutosaveTimeOut();

        // Verify that rin file is saved in the auto save.
        EMStudio::FileManager* fileManager = EMStudio::GetMainWindow()->GetFileManager();
        const AZStd::vector<AZStd::string>& savedSourceFile = fileManager->GetSavedSourceAssets();
        EXPECT_EQ(savedSourceFile.size(), 1) << "A graph should be auto saved in the file manager.";

        // Verify the source name, make sure it is the rin graph that get saved.
        const AZStd::string& autoSavedFile = savedSourceFile[0];
        size_t found = autoSavedFile.find("rin_Autosave");
        EXPECT_NE(found, AZStd::string::npos) << "The auto saved file should contain the rin_Autosave keyword.";

        // Verify if the autosave file exist on disk
        EXPECT_TRUE(QFile::exists(autoSavedFile.c_str())) << "Check that auto saved file exist.";
    }
} // namespace EMotionFX
