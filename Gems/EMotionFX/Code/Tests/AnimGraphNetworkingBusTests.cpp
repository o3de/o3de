/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Components/TransformComponent.h>
#include <Include/Integration/AnimGraphNetworkingBus.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/Components/AnimGraphComponent.h>
#include <Tests/Integration/EntityComponentFixture.h>

namespace EMotionFX
{
    class AnimGraphNetworkingBusTests
        : public EntityComponentFixture
    {
    public:
        void SetUp() override
        {
            EntityComponentFixture::SetUp();

            m_entity = AZStd::make_unique<AZ::Entity>();
            m_entityId = AZ::EntityId(740216387);
            m_entity->SetId(m_entityId);

            auto transformComponent = m_entity->CreateComponent<AzFramework::TransformComponent>();
            auto actorComponent = m_entity->CreateComponent<Integration::ActorComponent>();
            auto animGraphComponent = m_entity->CreateComponent<Integration::AnimGraphComponent>();

            m_entity->Init();

            m_entity->Activate();
            AnimGraphInstance* animGraphInstance = animGraphComponent->GetAnimGraphInstance();
            EXPECT_EQ(animGraphInstance, nullptr) << "Expecting an invalid anim graph instance as no asset has been set.";
        }

        void TearDown() override
        {
            m_entity->Deactivate();
            EntityComponentFixture::TearDown();
        }

        AZ::EntityId m_entityId;
        AZStd::unique_ptr<AZ::Entity> m_entity;
    };

    TEST_F(AnimGraphNetworkingBusTests, AnimGraphNetworkingBus_GetActiveStates_Test)
    {
        NodeIndexContainer result;
        EMotionFX::AnimGraphComponentNetworkRequestBus::EventResult(result, m_entityId, &EMotionFX::AnimGraphComponentNetworkRequestBus::Events::GetActiveStates);
    }

    TEST_F(AnimGraphNetworkingBusTests, AnimGraphNetworkingBus_GetMotionPlaytimes_Test)
    {
        MotionNodePlaytimeContainer result;
        EMotionFX::AnimGraphComponentNetworkRequestBus::EventResult(result, m_entityId, &EMotionFX::AnimGraphComponentNetworkRequestBus::Events::GetMotionPlaytimes);
    }
} // end namespace EMotionFX
