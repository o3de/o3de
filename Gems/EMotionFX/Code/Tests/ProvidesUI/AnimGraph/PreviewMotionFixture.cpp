/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <Tests/UI/AnimGraphUIFixture.h>
#include <Tests/ProvidesUI/AnimGraph/PreviewMotionFixture.h>

namespace EMotionFX
{
    void PreviewMotionFixture::SetUp()
    {
        AnimGraphUIFixture::SetUp();

        //Create one motion set, and import one motion and add to the motion set.
        ExecuteCommands({
        R"str(CreateMotionSet -name MotionSet0)str"
            });

        EMStudio::MotionSetsWindowPlugin* motionSetsWindowPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionSetsWindowPlugin) << "Motion Window plugin not loaded";

        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(0);
        ASSERT_TRUE(motionSet) << "Motion set with id 0 does not exist";
        motionSetsWindowPlugin->SetSelectedSet(motionSet);

        ExecuteCommands({
        "ImportMotion -filename @engroot@/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion",
        "MotionSetAddMotion -motionSetID 0 -motionFilenamesAndIds @engroot@/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion;rin_idle"
            });

        char resolvedPath[AZ::IO::MaxPathLength];
        EXPECT_TRUE(AZ::IO::FileIOBase::GetInstance()->ResolvePath("@engroot@/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion", resolvedPath, AZ_ARRAY_SIZE(resolvedPath)));
        m_motionFileName = resolvedPath;
        AzFramework::ApplicationRequests::Bus::Broadcast([](AzFramework::ApplicationRequests* requests, AZStd::string& path) { requests->NormalizePathKeepCase(path); }, m_motionFileName);
        m_motionName = "rin_idle";
    }
}
