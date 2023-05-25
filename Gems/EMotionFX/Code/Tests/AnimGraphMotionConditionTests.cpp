/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/SystemComponentFixture.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/TwoStringEventData.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

namespace EMotionFX
{
    // This class has the Version 1 of the AnimGraphMotionCondition fields
    // reflected. Instances of this class can be serialized, then the
    // production EMotionFX::AnimGraphMotionCondition can be used to
    // deserialize. The converter should be able to convert from version 1 to
    // 2.
    class AnimGraphMotionConditionV1
        : public AnimGraphTransitionCondition
    {
    public:
        // It is important that this contain the same UUID as the production
        // class, so that the deserialization code will use the converter
        AZ_RTTI(AnimGraphMotionConditionV1, "{0E2EDE4E-BDEE-4383-AB18-208CE7F7A784}", AnimGraphTransitionCondition)
        AZ_CLASS_ALLOCATOR(AnimGraphMotionConditionV1, AnimGraphAllocator)

        enum TestFunction : AZ::u8
        {
            FUNCTION_EVENT                  = 0,
            FUNCTION_HASENDED               = 1,
            FUNCTION_HASREACHEDMAXNUMLOOPS  = 2,
            FUNCTION_PLAYTIME               = 3,
            FUNCTION_PLAYTIMELEFT           = 4,
            FUNCTION_ISMOTIONASSIGNED       = 5,
            FUNCTION_ISMOTIONNOTASSIGNED    = 6,
            FUNCTION_NONE                   = 7
        };

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<AnimGraphMotionConditionV1, AnimGraphTransitionCondition>()
                ->Version(1)
                ->Field("motionNodeId", &AnimGraphMotionConditionV1::m_motionNodeId)
                ->Field("testFunction", &AnimGraphMotionConditionV1::m_testFunction)
                ->Field("numLoops", &AnimGraphMotionConditionV1::m_numLoops)
                ->Field("playTime", &AnimGraphMotionConditionV1::m_playTime)
                ->Field("eventType", &AnimGraphMotionConditionV1::m_eventType)
                ->Field("eventParameter", &AnimGraphMotionConditionV1::m_eventParameter)
                ;
        }

        const char* GetPaletteName() const override { return "Motion Condition"; }
        bool TestCondition(AnimGraphInstance* ) const override { return false; }

        AZStd::string           m_eventType;
        AZStd::string           m_eventParameter;
        AZ::u64                 m_motionNodeId;
        AZ::u32                 m_numLoops;
        float                   m_playTime;
        TestFunction            m_testFunction;
    };

    TEST_F(SystemComponentFixture, TestAnimGraphMotionConditionV1Conversion)
    {
        // Make a new serialization context for the V1 MotionCondition
        AZ::SerializeContext v1context;
        AnimGraphObject::Reflect(&v1context);
        AnimGraphTransitionCondition::Reflect(&v1context);
        AnimGraphMotionConditionV1::Reflect(&v1context);

        AnimGraphMotionConditionV1 v1condition;
        v1condition.m_motionNodeId = 42;
        v1condition.m_numLoops = 5;
        v1condition.m_playTime = 2.43f;
        v1condition.m_testFunction = AnimGraphMotionConditionV1::FUNCTION_EVENT;
        v1condition.m_eventType = "My event type";
        v1condition.m_eventParameter = "My parameters";

        // Serialize using the v1 definition
        AZStd::vector<char> charBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > charStream(&charBuffer);
        AZ::Utils::SaveObjectToStream(charStream, AZ::ObjectStream::ST_XML, &v1condition, &v1context);

        // Load using the v2 definition
        AnimGraphMotionCondition* v2condition = AZ::Utils::LoadObjectFromBuffer<AnimGraphMotionCondition>(&charBuffer[0], charBuffer.size(), GetSerializeContext());

        // Ensure the correct EventData was created. The eventType and
        // eventParameter fields get packed into the EventData
        const AZStd::shared_ptr<const TwoStringEventData> expectedEventData = GetEventManager().FindOrCreateEventData<TwoStringEventData>("My event type", "My parameters");
        const AZStd::shared_ptr<const EventData>& gotEventData = v2condition->GetEventDatas()[0];
        ASSERT_TRUE(gotEventData);
        EXPECT_EQ(*gotEventData.get(), *expectedEventData.get());

        delete v2condition;
    }
} // end namespace EMotionFX
