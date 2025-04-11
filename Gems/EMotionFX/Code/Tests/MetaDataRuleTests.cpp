/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/InitSceneAPIFixture.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>

#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Pipeline/SceneAPIExt/Rules/MetaDataRule.h>
#include <EMotionFX/Pipeline/RCExt/Motion/MotionGroupExporter.h>
#include <Integration/System/PipelineComponent.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>

namespace EMotionFX
{
    using MetaDataRuleBaseClass = InitSceneAPIFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        EMotionFX::Integration::SystemComponent,
        EMotionFX::Pipeline::MotionGroupExporter
    >;
    class MetaDataRuleTestsPipelineFixture
        : public MetaDataRuleBaseClass
    {

    public:
        void SetUp() override
        {
            MetaDataRuleBaseClass::SetUp();
            m_commandManager = AZStd::make_unique<CommandSystem::CommandManager>();
        }

        void TearDown() override
        {
            m_commandManager.reset();
            MetaDataRuleBaseClass::TearDown();
        }
    private:
        AZStd::unique_ptr<CommandSystem::CommandManager> m_commandManager;
    };

    TEST_F(MetaDataRuleTestsPipelineFixture, TestVersion1Import)
    {
        EMotionFX::Pipeline::Rule::MetaDataRule::Reflect(GetSerializeContext());
        const AZStd::string sourceText {
"<ObjectStream version=\"3\">\n"
"    <Class name=\"MetaDataRule\" version=\"1\" type=\"{8D759063-7D2E-4543-8EB3-AB510A5886CF}\">\n"
"        <Class name=\"AZStd::string\" field=\"metaData\" value='AdjustMotion -motionID $(MOTIONID) -motionExtractionFlags 0\n"
"ClearMotionEvents -motionID $(MOTIONID)\n"
"CreateMotionEventTrack -motionID $(MOTIONID) -eventTrackName \"Sync\"\n"
"AdjustMotionEventTrack -motionID $(MOTIONID) -eventTrackName \"Sync\" -enabled true\n"
"CreateMotionEvent -motionID $(MOTIONID) -eventTrackName \"Sync\" -startTime 0.022680 -endTime 0.022680 -eventType \"RightFoot\" -parameters \"\" -mirrorType \"LeftFoot\"\n"
"' type=\"{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}\"/>\n"
"    </Class>\n"
"</ObjectStream>\n"
        };

        const Pipeline::Rule::MetaDataRule* metaDataRule = MCore::ReflectionSerializer::Deserialize<Pipeline::Rule::MetaDataRule>(sourceText);
        const AZStd::vector<MCore::Command*>& commands = metaDataRule->GetMetaData<const AZStd::vector<MCore::Command*>&>();

        EXPECT_EQ(commands.size(), 5) << "There should be 5 commands";
        EMotionFX::Motion* motion = aznew EMotionFX::Motion("TestMotion");
        CommandSystem::MetaData::ApplyMetaDataOnMotion(motion, commands);
        ASSERT_EQ(motion->GetEventTable()->GetNumTracks(), 1);
        EMotionFX::MotionEventTrack* eventTrack = motion->GetEventTable()->GetTrack(0);
        EXPECT_STREQ(eventTrack->GetName(), "Sync");
        ASSERT_EQ(eventTrack->GetNumEvents(), 1);
        EMotionFX::MotionEvent& event = eventTrack->GetEvent(0);
        const EventDataSet& eventDatas = event.GetEventDatas();
        ASSERT_EQ(eventDatas.size(), 1);
        const AZStd::shared_ptr<const TwoStringEventData> eventData = AZStd::rtti_pointer_cast<const TwoStringEventData>(eventDatas[0]);
        ASSERT_TRUE(eventData);
        EXPECT_STREQ(eventData->GetSubject().c_str(), "RightFoot");
        EXPECT_STREQ(eventData->GetParameters().c_str(), "");
        EXPECT_STREQ(eventData->GetMirrorSubject().c_str(), "LeftFoot");
        motion->Destroy();
        delete metaDataRule;
    }

} // end namespace EMotionFX
