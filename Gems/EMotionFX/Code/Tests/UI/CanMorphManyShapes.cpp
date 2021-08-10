/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <AzQtComponents/Components/Widgets/SliderCombo.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTreeMorphTargetNode.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/MorphTargetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/MorphTargetGroupWidget.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/UI/UIFixture.h>

namespace EMotionFX
{
    class CanMorphManyShapesFixture
        : public UIFixture
    {
    public:
        void ScaleMesh(Mesh* mesh)
        {
            const uint32 vertexCount = mesh->GetNumVertices();
            AZ::Vector3* positions = static_cast<AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
            for (uint32 vertexNum = 0; vertexNum < vertexCount; ++vertexNum)
            {
                positions[vertexNum] *= m_scaleFactor;
            }
        }

        void AddParam(const char* name, const AZ::TypeId& type, const AZStd::string& defaultValue)
        {
            EMotionFX::Parameter* parameter = EMotionFX::ParameterFactory::Create(type);
            parameter->SetName(name);
            MCore::ReflectionSerializer::DeserializeIntoMember(parameter, "defaultValue", defaultValue);
            m_animGraph->AddParameter(parameter);
        }

        void SetUp() override
        {
            UIFixture::SetUp();

            m_actor = ActorFactory::CreateAndInit<PlaneActor>("testActor");

            m_morphSetup = MorphSetup::Create();
            m_actor->SetMorphSetup(0, m_morphSetup);

            AZStd::unique_ptr<Actor> morphActor = m_actor->Clone();
            Mesh* morphMesh = morphActor->GetMesh(0, 0);
            ScaleMesh(morphMesh);
            MorphTargetStandard* morphTarget = MorphTargetStandard::Create(
                /*captureTransforms=*/ false,
                m_actor.get(),
                morphActor.get(),
                "morphTarget"
            );
            m_morphSetup->AddMorphTarget(morphTarget);

            // Without this call, the bind pose does not know about newly added morph target (m_morphWeights.size() == 0)
            m_actor->ResizeTransformData();
            m_actor->PostCreateInit(/*makeGeomLodsCompatibleWithSkeletalLODs=*/false, /*convertUnitType=*/false);

            m_animGraph = AZStd::make_unique<AnimGraph>();

            AddParam("FloatParam", azrtti_typeid<EMotionFX::FloatSliderParameter>(), "0.0");

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();

            BlendTreeMorphTargetNode* morphTargetNode = aznew BlendTreeMorphTargetNode();
            morphTargetNode->SetMorphTargetNames({ "morphTarget" });

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            BlendTree* blendTree = aznew BlendTree();
            blendTree->SetName("testBlendTree");
            blendTree->AddChildNode(parameterNode);
            blendTree->AddChildNode(morphTargetNode);
            blendTree->AddChildNode(finalNode);
            blendTree->SetFinalNodeId(finalNode->GetId());

            m_stateMachine = aznew AnimGraphStateMachine();
            m_stateMachine->SetName("rootStateMachine");
            m_animGraph->SetRootStateMachine(m_stateMachine);
            m_stateMachine->AddChildNode(blendTree);
            m_stateMachine->SetEntryState(blendTree);

            m_stateMachine->InitAfterLoading(m_animGraph.get());

            // Create the connections once the port indices are known. The parameter node's ports are not known until after
            // InitAfterLoading() is called
            morphTargetNode->AddConnection(
                parameterNode,
                aznumeric_caster(parameterNode->FindOutputPortIndex("FloatParam")),
                BlendTreeMorphTargetNode::PORTID_INPUT_WEIGHT
            );
            finalNode->AddConnection(
                morphTargetNode,
                BlendTreeMorphTargetNode::PORTID_OUTPUT_POSE,
                BlendTreeFinalNode::PORTID_INPUT_POSE
            );

            m_motionSet = AZStd::make_unique<MotionSet>();
            m_motionSet->SetName("testMotionSet");

            m_actorInstance = Integration::EMotionFXPtr<ActorInstance>::MakeFromNew(ActorInstance::Create(m_actor.get()));

            m_animGraphInstance = AnimGraphInstance::Create(m_animGraph.get(), m_actorInstance.get(), m_motionSet.get());

            m_actorInstance->SetAnimGraphInstance(m_animGraphInstance);
        }

        void TearDown() override
        {
            m_actor.reset();
            m_actorInstance.reset();
            m_motionSet.reset();
            m_animGraph.reset();

            UIFixture::TearDown();
        }
    protected:
        AZStd::unique_ptr<Actor> m_actor;
        MorphSetup* m_morphSetup = nullptr;
        const float m_scaleFactor = 10.0f;
        AZStd::unique_ptr<AnimGraph> m_animGraph;
        AnimGraphStateMachine* m_stateMachine = nullptr;
        Integration::EMotionFXPtr<ActorInstance> m_actorInstance;
        AnimGraphInstance* m_animGraphInstance = nullptr;
        AZStd::unique_ptr<MotionSet> m_motionSet;
    };


    TEST_F(CanMorphManyShapesFixture, CanMorphManyShapes)
    {
        RecordProperty("test_case_id", "C1559259");

        // Change the Editor mode to Character
        EMStudio::GetMainWindow()->ApplicationModeChanged("Character");

        // Find the MorphTargetsWindowPlugin
        EMStudio::MorphTargetsWindowPlugin* morphTargetWindow = static_cast<EMStudio::MorphTargetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MorphTargetsWindowPlugin::CLASS_ID));
        ASSERT_TRUE(morphTargetWindow) << "MorphTargetsWindow plugin not found!";

        // Select the newly created actor instance
        AZStd::string result;
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(AZStd::string::format(("Select -actorInstanceID %s"), AZStd::to_string(m_actorInstance->GetID()).c_str()), result)) << result.c_str();

        EMStudio::MorphTargetGroupWidget* morphTargetGroupWidget = morphTargetWindow->GetDockWidget()->findChild<EMStudio::MorphTargetGroupWidget*>("EMFX.MorphTargetsWindowPlugin.MorphTargetGroupWidget");
        EXPECT_TRUE(morphTargetGroupWidget) << "Morph target Group not found";

        const EMStudio::MorphTargetGroupWidget::MorphTarget* morphTarget = morphTargetGroupWidget->GetMorphTarget(0);
        EXPECT_TRUE(morphTarget) << "Cannot access MorphTarget";

        // Switch the morphTarget to manual mode
        morphTarget->m_manualMode->click();

        // Set the slider to 0.5f;
        morphTarget->m_sliderWeight->slider()->setValue(0.5f);

        // Get the instance of the MorphTargetInstance
        EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance = morphTarget->m_morphTargetInstance;
        ASSERT_TRUE(morphTargetInstance) << "Cannot get Instance of Morph Target";
        EXPECT_EQ(morphTargetInstance->GetWeight(), 0.5f) << "Morph Taget Instance is not set to the correct value";
    }
}   // namespace EMotionFX

