/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/TransformComponent.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionFX/Source/Parameter/RotationParameter.h>
#include <EMotionFX/Source/Parameter/StringParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Include/Integration/AnimGraphComponentBus.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/Components/AnimGraphComponent.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AzCoreConversions.h>
#include <Tests/Integration/EntityComponentFixture.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/TestActorAssets.h>

namespace EMotionFX
{
    class AnimGraphComponentNotificationTestBus
        : Integration::AnimGraphComponentNotificationBus::Handler
    {
    public:
        AnimGraphComponentNotificationTestBus(AZ::EntityId entityId)
        {
            Integration::AnimGraphComponentNotificationBus::Handler::BusConnect(entityId);
        }

        ~AnimGraphComponentNotificationTestBus()
        {
            Integration::AnimGraphComponentNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(OnAnimGraphInstanceCreated, void(EMotionFX::AnimGraphInstance*));
        MOCK_METHOD1(OnAnimGraphInstanceDestroyed, void(EMotionFX::AnimGraphInstance*));
        MOCK_METHOD4(OnAnimGraphFloatParameterChanged, void(EMotionFX::AnimGraphInstance*, size_t, float, float));
        MOCK_METHOD4(OnAnimGraphBoolParameterChanged, void(EMotionFX::AnimGraphInstance*, size_t, bool, bool));
        MOCK_METHOD4(OnAnimGraphStringParameterChanged, void(EMotionFX::AnimGraphInstance*, size_t, const char*, const char*));
        MOCK_METHOD4(OnAnimGraphVector2ParameterChanged, void(EMotionFX::AnimGraphInstance*, size_t, const AZ::Vector2&, const AZ::Vector2&));
        MOCK_METHOD4(OnAnimGraphVector3ParameterChanged, void(EMotionFX::AnimGraphInstance*, size_t, const AZ::Vector3&, const AZ::Vector3&));
        MOCK_METHOD4(OnAnimGraphRotationParameterChanged, void(EMotionFX::AnimGraphInstance*, size_t, const AZ::Quaternion&, const AZ::Quaternion&));
    };

    class AnimGraphComponentBusTests
        : public EntityComponentFixture
    {
    public:
        void SetUp() override
        {
            EntityComponentFixture::SetUp();

            m_entity = AZStd::make_unique<AZ::Entity>();
            m_entityId = AZ::EntityId(740216387);
            m_entity->SetId(m_entityId);

            AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
            AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
            AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, AZStd::move(actor));
            Integration::ActorComponent::Configuration actorConf;
            actorConf.m_actorAsset = actorAsset;

            m_entity->CreateComponent<AzFramework::TransformComponent>();
            m_actorComponent = m_entity->CreateComponent<Integration::ActorComponent>(&actorConf);
            m_animGraphComponent = m_entity->CreateComponent<Integration::AnimGraphComponent>();

            m_entity->Init();

            // Anim graph asset.
            AZ::Data::AssetId animGraphAssetId("{37629818-5166-4B96-83F5-5818B6A1F449}");
            m_animGraphComponent->SetAnimGraphAssetId(animGraphAssetId);
            AZ::Data::Asset<Integration::AnimGraphAsset> animGraphAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::AnimGraphAsset>(animGraphAssetId);
            AZStd::unique_ptr<TwoMotionNodeAnimGraph> motionNodeAnimGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
            m_animGraph = motionNodeAnimGraph.get();
            motionNodeAnimGraph.release();
            animGraphAsset.GetAs<Integration::AnimGraphAsset>()->SetData(m_animGraph);
            EXPECT_EQ(animGraphAsset.IsReady(), true) << "Anim graph asset is not ready yet.";
            m_animGraphComponent->OnAssetReady(animGraphAsset);

            // Motion set asset.
            AZ::Data::AssetId motionSetAssetId("{224BFF5F-D0AD-4216-9CEF-42F419CC6265}");
            m_animGraphComponent->SetMotionSetAssetId(motionSetAssetId);
            AZ::Data::Asset<Integration::MotionSetAsset> motionSetAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::MotionSetAsset>(motionSetAssetId);
            motionSetAsset.GetAs<Integration::MotionSetAsset>()->SetData(new MotionSet());
            EXPECT_EQ(motionSetAsset.IsReady(), true) << "Motion set asset is not ready yet.";
            m_animGraphComponent->OnAssetReady(motionSetAsset);
        }

        void TearDown() override
        {
            m_entity.reset();
            EntityComponentFixture::TearDown();
        }

        void ActivateEntity()
        {
            // Set the actor asset and create the actor instance.
            m_actorComponent->SetActorAsset(m_actorComponent->GetActorAsset());

            m_entity->Activate();

            // Run one tick so that the actor asset has time to finish activating.
            // (Actor initialization is deferred to the next tick after the OnAssetReady call)
            AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.0f, AZ::ScriptTimePoint());

            m_animGraphInstance = m_animGraphComponent->GetAnimGraphInstance();
            EXPECT_NE(m_animGraphInstance, nullptr) << "Expecting valid anim graph instance.";
        }

        void PrepareParameterTest(ValueParameter* parameter)
        {
            ActivateEntity();

            m_animGraphInstance = m_animGraphComponent->GetAnimGraphInstance();
            EXPECT_NE(m_animGraphInstance, nullptr) << "Expecting valid anim graph instance.";

            m_parameterName = "Test Parameter";
            parameter->SetName(m_parameterName.c_str());
            m_animGraph->AddParameter(parameter);
            m_animGraphInstance->AddMissingParameterValues();

            // FindParameterIndex() test
            Integration::AnimGraphComponentRequestBus::EventResult(m_parameterIndex, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::FindParameterIndex, m_parameterName.c_str());
            EXPECT_EQ(m_parameterIndex, 0) << "Expected the index for the first parameter.";

            // FindParameterName() test
            const char* foundParameterName = nullptr;
            Integration::AnimGraphComponentRequestBus::EventResult(foundParameterName, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::FindParameterName, m_parameterIndex);
            EXPECT_STREQ(foundParameterName, m_parameterName.c_str()) << "Expected the index for the first parameter.";
        }

        AZ::EntityId m_entityId;
        AZStd::unique_ptr<AZ::Entity> m_entity;
        TwoMotionNodeAnimGraph* m_animGraph = nullptr;
        Integration::ActorComponent* m_actorComponent = nullptr;
        Integration::AnimGraphComponent* m_animGraphComponent = nullptr;
        AnimGraphInstance* m_animGraphInstance = nullptr;
        size_t m_parameterIndex = InvalidIndex;
        std::string m_parameterName;
    };

    TEST_F(AnimGraphComponentBusTests, GetAnimGraphInstance)
    {
        ActivateEntity();

        AnimGraphInstance* animgrGraphInstance = nullptr;
        Integration::AnimGraphComponentRequestBus::EventResult(animgrGraphInstance, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetAnimGraphInstance);
        EXPECT_NE(animgrGraphInstance, nullptr) << "Expecting valid anim graph instance.";
        EXPECT_EQ(animgrGraphInstance, m_animGraphInstance) << "Expecting the anim graph instance from our anim graph component.";
    }

    TEST_F(AnimGraphComponentBusTests, FloatParameter)
    {
        AnimGraphComponentNotificationTestBus testBus(m_entityId);
        EXPECT_CALL(testBus, OnAnimGraphInstanceCreated(testing::_));

        PrepareParameterTest(aznew FloatSliderParameter());

        {
            testing::InSequence parameterChangedCallSequence;
            EXPECT_CALL(testBus, OnAnimGraphFloatParameterChanged(m_animGraphInstance, m_parameterIndex, testing::_, 3.0f));
            EXPECT_CALL(testBus, OnAnimGraphFloatParameterChanged(m_animGraphInstance, m_parameterIndex, 3.0f, 4.0f));
        }

        // SetParameterFloat/GetParameterFloat() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetParameterFloat, m_parameterIndex, 3.0f);
        float newValue = 0.0f;
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetParameterFloat, m_parameterIndex);
        EXPECT_EQ(newValue, 3.0f) << "Expected a parameter value of 3.0.";

        // SetNamedParameterFloat/GetNamedParameterFloat() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetNamedParameterFloat, m_parameterName.c_str(), 4.0f);
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetNamedParameterFloat, m_parameterName.c_str());
        EXPECT_EQ(newValue, 4.0f) << "Expected a parameter value of 3.0.";
    }

    TEST_F(AnimGraphComponentBusTests, BoolParameter)
    {
        AnimGraphComponentNotificationTestBus testBus(m_entityId);
        EXPECT_CALL(testBus, OnAnimGraphInstanceCreated(testing::_));

        PrepareParameterTest(aznew BoolParameter());

        {
            testing::InSequence parameterChangedCallSequence;
            EXPECT_CALL(testBus, OnAnimGraphBoolParameterChanged(m_animGraphInstance, m_parameterIndex, testing::_, true));
            EXPECT_CALL(testBus, OnAnimGraphBoolParameterChanged(m_animGraphInstance, m_parameterIndex, true, false));
        }

        
        // SetParameterBool/GetParameterBool() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetParameterBool, m_parameterIndex, true);
        bool newValue = false;
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetParameterBool, m_parameterIndex);
        EXPECT_EQ(newValue, true) << "Expected true as parameter value.";

        // SetNamedParameterBool/GetNamedParameterBool() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetNamedParameterBool, m_parameterName.c_str(), false);
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetNamedParameterBool, m_parameterName.c_str());
        EXPECT_EQ(newValue, false) << "Expected false as parameter value.";
    }

    TEST_F(AnimGraphComponentBusTests, StringParameter)
    {
        AnimGraphComponentNotificationTestBus testBus(m_entityId);
        EXPECT_CALL(testBus, OnAnimGraphInstanceCreated(testing::_));

        PrepareParameterTest(aznew StringParameter());

        EXPECT_CALL(testBus, OnAnimGraphStringParameterChanged(m_animGraphInstance, m_parameterIndex, testing::_, testing::_))
            .Times(2);

        // SetParameterString/GetParameterString() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetParameterString, m_parameterIndex, "Test String");
        AZStd::string newValue;
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetParameterString, m_parameterIndex);
        EXPECT_STREQ(newValue.c_str(), "Test String") << "Expected the test string parameter.";

        // SetNamedParameterString/GetNamedParameterString() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetNamedParameterString, m_parameterName.c_str(), "Yet Another String");
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetNamedParameterString, m_parameterName.c_str());
        EXPECT_STREQ(newValue.c_str(), "Yet Another String") << "Expected yet another string.";
    }

    TEST_F(AnimGraphComponentBusTests, Vector2Parameter)
    {
        AnimGraphComponentNotificationTestBus testBus(m_entityId);
        EXPECT_CALL(testBus, OnAnimGraphInstanceCreated(testing::_));

        PrepareParameterTest(aznew Vector2Parameter());

        {
            testing::InSequence parameterChangedCallSequence;
            EXPECT_CALL(testBus, OnAnimGraphVector2ParameterChanged(m_animGraphInstance, m_parameterIndex, testing::_, AZ::Vector2(1.0f, 2.0f)));
            EXPECT_CALL(testBus, OnAnimGraphVector2ParameterChanged(m_animGraphInstance, m_parameterIndex, AZ::Vector2(1.0f, 2.0f), AZ::Vector2(3.0f, 4.0f)));
        }

        // SetParameterVector2/GetParameterVector2() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetParameterVector2, m_parameterIndex, AZ::Vector2(1.0f, 2.0f));
        AZ::Vector2 newValue;
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetParameterVector2, m_parameterIndex);
        EXPECT_EQ(newValue, AZ::Vector2(1.0f, 2.0f));

        // SetNamedParameterVector2/GetNamedParameterVector2() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetNamedParameterVector2, m_parameterName.c_str(), AZ::Vector2(3.0f, 4.0f));
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetNamedParameterVector2, m_parameterName.c_str());
        EXPECT_EQ(newValue, AZ::Vector2(3.0f, 4.0f));
    }

    TEST_F(AnimGraphComponentBusTests, Vector3Parameter)
    {
        AnimGraphComponentNotificationTestBus testBus(m_entityId);
        EXPECT_CALL(testBus, OnAnimGraphInstanceCreated(testing::_));

        PrepareParameterTest(aznew Vector3Parameter());

        {
            testing::InSequence parameterChangedCallSequence;
            EXPECT_CALL(testBus, OnAnimGraphVector3ParameterChanged(m_animGraphInstance, m_parameterIndex, testing::_, AZ::Vector3(1.0f, 2.0f, 3.0f)));
            EXPECT_CALL(testBus, OnAnimGraphVector3ParameterChanged(m_animGraphInstance, m_parameterIndex, AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Vector3(4.0f, 5.0f, 6.0f)));
        }

        // SetParameterVector3/GetParameterVector3() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetParameterVector3, m_parameterIndex, AZ::Vector3(1.0f, 2.0f, 3.0f));
        AZ::Vector3 newValue;
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetParameterVector3, m_parameterIndex);
        EXPECT_EQ(newValue, AZ::Vector3(1.0f, 2.0f, 3.0f));

        // SetNamedParameterVector3/GetNamedParameterVector3() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetNamedParameterVector3, m_parameterName.c_str(), AZ::Vector3(4.0f, 5.0f, 6.0f));
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetNamedParameterVector3, m_parameterName.c_str());
        EXPECT_EQ(newValue, AZ::Vector3(4.0f, 5.0f, 6.0f));
    }

    TEST_F(AnimGraphComponentBusTests, RotationParameterEuler)
    {
        AnimGraphComponentNotificationTestBus testBus(m_entityId);
        EXPECT_CALL(testBus, OnAnimGraphInstanceCreated(testing::_));

        PrepareParameterTest(aznew RotationParameter());

        EXPECT_CALL(testBus, OnAnimGraphRotationParameterChanged(m_animGraphInstance, m_parameterIndex, testing::_, testing::_))
            .Times(2);

        // SetParameterRotationEuler/GetParameterRotationEuler() test
        AZ::Vector3 expectedEuler(AZ::DegToRad(30.0f), AZ::DegToRad(20.0f), 0.0f);
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetParameterRotationEuler, m_parameterIndex, expectedEuler);
        AZ::Vector3 newValue;
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetParameterRotationEuler, m_parameterIndex);
        EXPECT_TRUE(newValue.IsClose(expectedEuler, 0.001f));

        // SetNamedParameterRotationEuler/GetNamedParameterRotationEuler() test
        expectedEuler = AZ::Vector3(AZ::DegToRad(45.0f), 0.0f, AZ::DegToRad(30.0f));
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetNamedParameterRotationEuler, m_parameterName.c_str(), expectedEuler);
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetNamedParameterRotationEuler, m_parameterName.c_str());
        EXPECT_TRUE(newValue.IsClose(expectedEuler, 0.001f));
    }

    TEST_F(AnimGraphComponentBusTests, RotationParameter)
    {
        const AZ::Vector3 firstExpected(AZ::DegToRad(30.0f), AZ::DegToRad(20.0f), 0.0f);
        const AZ::Quaternion firstExpectedQuat = MCore::AzEulerAnglesToAzQuat(firstExpected);
        const AZ::Vector3 secondExpected = AZ::Vector3(AZ::DegToRad(45.0f), 0.0f, AZ::DegToRad(30.0f));
        const AZ::Quaternion secondExpectedQuat = MCore::AzEulerAnglesToAzQuat(secondExpected);


        AnimGraphComponentNotificationTestBus testBus(m_entityId);
        EXPECT_CALL(testBus, OnAnimGraphInstanceCreated(testing::_));

        PrepareParameterTest(aznew RotationParameter());

        {
            testing::InSequence parameterChangedCallSequence;
            EXPECT_CALL(testBus, OnAnimGraphRotationParameterChanged(m_animGraphInstance, m_parameterIndex, testing::_, firstExpectedQuat));
            EXPECT_CALL(testBus, OnAnimGraphRotationParameterChanged(m_animGraphInstance, m_parameterIndex, testing::_, secondExpectedQuat));
        }

        // SetParameterRotation/GetParameterRotation() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetParameterRotation, m_parameterIndex, firstExpectedQuat);
        AZ::Quaternion newValue;
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetParameterRotation, m_parameterIndex);
        EXPECT_TRUE(newValue.IsClose(firstExpectedQuat, 0.001f));

        // SetNamedParameterRotation/GetNamedParameterRotation() test
        Integration::AnimGraphComponentRequestBus::Event(m_entityId, &Integration::AnimGraphComponentRequestBus::Events::SetNamedParameterRotation, m_parameterName.c_str(), secondExpectedQuat);
        Integration::AnimGraphComponentRequestBus::EventResult(newValue, m_entityId, &Integration::AnimGraphComponentRequestBus::Events::GetNamedParameterRotation, m_parameterName.c_str());
        EXPECT_TRUE(newValue.IsClose(secondExpectedQuat, 0.001f));
    }

    TEST_F(AnimGraphComponentBusTests, OnAnimGraphInstanceDestroyed)
    {
        AnimGraphComponentNotificationTestBus testBus(m_entityId);
        EXPECT_CALL(testBus, OnAnimGraphInstanceCreated(testing::_));

        ActivateEntity();

        EXPECT_CALL(testBus, OnAnimGraphInstanceDestroyed(m_animGraphInstance));

        m_entity->Deactivate();
    }
} // end namespace EMotionFX
