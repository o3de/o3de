/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AnimGraphFixture.h>
#include <Integration/Assets/AnimGraphAsset.h>
#include <MCore/Source/AttributeFloat.h>
#include <AzCore/Asset/AssetManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/BlendTreeTransformNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Tests/TestAssetCode/AnimGraphAssetFactory.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>
#include <Tests/Printers.h>

namespace EMotionFX
{
    // The smallest possible graph that contains a reference node
    class JustAReferenceNodeGraph
        : public EmptyAnimGraph
    {
    public:
        AZ_CLASS_ALLOCATOR(JustAReferenceNodeGraph, AnimGraphAllocator)
        JustAReferenceNodeGraph()
        {
            /*
            +--Root State---------------------------------------------+
            |                                                         |
            |  +-Blend Tree----------------------------------------+  |
            |  |                                                   |  |
            |  |  +-Reference Node----+----->+-Final Node------+   |  |
            |  |  +-------------------+      +-----------------+   |  |
            |  |                                                   |  |
            |  +---------------------------------------------------+  |
            +---------------------------------------------------------+
            */

            m_referenceNode = aznew AnimGraphReferenceNode();
            m_referenceNode->SetAnimGraph(this);
            m_referenceNode->SetName("ReferenceNodeInParentGraph");

            auto* finalNode = aznew BlendTreeFinalNode();
            finalNode->SetName("BlendTreeFinalNodeParentGraph");
            finalNode->AddUnitializedConnection(m_referenceNode, AnimGraphReferenceNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_blendTree = aznew BlendTree();
            m_blendTree->SetName("BlendTreeInParentGraph");
            m_blendTree->AddChildNode(m_referenceNode);
            m_blendTree->AddChildNode(finalNode);
            m_blendTree->SetFinalNodeId(finalNode->GetId());

            GetRootStateMachine()->AddChildNode(m_blendTree);
            GetRootStateMachine()->SetEntryState(m_blendTree);
        }

        BlendTree* GetBlendTree() const { return m_blendTree; }
        AnimGraphReferenceNode* GetReferenceNode() const { return m_referenceNode; }

    private:
        BlendTree* m_blendTree = nullptr;
        AnimGraphReferenceNode* m_referenceNode = nullptr;
    };

    class ReferenceNodeWithParameterGraph
        : public JustAReferenceNodeGraph
    {
    public:
        AZ_CLASS_ALLOCATOR(ReferenceNodeWithParameterGraph, AnimGraphAllocator)
        ReferenceNodeWithParameterGraph()
        {
            /*
            +--Root State---------------------------------------------------------------------+
            |                                                                                 |
            |  +-Blend Tree----------------------------------------------------------------+  |
            |  |                                                                           |  |
            |  |  +-ParameterNode---+---->+-Reference Node----+----->+-Final Node------+   |  |
            |  |  +-----------------+     +-------------------+      +-----------------+   |  |
            |  |                                                                           |  |
            |  +---------------------------------------------------------------------------+  |
            +---------------------------------------------------------------------------------+
            */
            m_parameter = static_cast<FloatSliderParameter*>(ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>()));
            AddParameter(m_parameter);

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
            parameterNode->SetName("ParameterNodeInParentGraph");

            GetBlendTree()->AddChildNode(parameterNode);
            GetReferenceNode()->AddUnitializedConnection(parameterNode, 0, 0);
        }

        FloatSliderParameter* GetParameter() const { return m_parameter; }

    private:
        FloatSliderParameter* m_parameter = nullptr;
    };

    // An AnimGraph that will apply a transform based on a float parameter value
    class BlendTreeTransformNodeAnimGraph
        : public EmptyAnimGraph
    {
    public:
        AZ_CLASS_ALLOCATOR(BlendTreeTransformNodeAnimGraph, AnimGraphAllocator)
        BlendTreeTransformNodeAnimGraph()
        {
            /*
            +--Root State--------------------------------------------------------------+
            |                                                                          |
            |  +-Blend Tree---------------------------------------------------------+  |
            |  |                                                                    |  |
            |  |  +-Parameter Node--+--->+-Transform Node--+-->+-Final Node-----+   |  |
            |  |  +-----------------+    +-----------------+  +-----------------+   |  |
            |  |                                                                    |  |
            |  +--------------------------------------------------------------------+  |
            +--------------------------------------------------------------------------+
            */

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
            parameterNode->SetName("ParameterNodeInReferenceGraph");

            m_transformNode = aznew BlendTreeTransformNode();
            m_transformNode->SetName("BlendTreeTransformNodeInReferenceGraph");
            m_transformNode->AddUnitializedConnection(parameterNode, 0, BlendTreeTransformNode::PORTID_INPUT_TRANSLATE_AMOUNT);
            m_transformNode->SetMinTranslation(AZ::Vector3::CreateZero());
            m_transformNode->SetMaxTranslation(AZ::Vector3::CreateAxisX(10.0f));
            m_transformNode->SetTargetNodeName("rootJoint"); // From the SimpleJointChain actor


            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            finalNode->SetName("BlendTreeFinalNodeInReferenceGraph");
            finalNode->AddUnitializedConnection(m_transformNode, BlendTreeTransformNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            BlendTree* blendTree = aznew BlendTree();
            blendTree->SetName("BlendTreeInReferenceGraph");
            blendTree->AddChildNode(m_transformNode);
            blendTree->AddChildNode(finalNode);
            blendTree->AddChildNode(parameterNode);

            GetRootStateMachine()->AddChildNode(blendTree);
            GetRootStateMachine()->SetEntryState(blendTree);

            m_parameter = static_cast<FloatSliderParameter*>(ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>()));
            AddParameter(m_parameter);
        }

        BlendTreeTransformNode* GetTransformNode() const { return m_transformNode; }
        FloatSliderParameter* GetParameter() const { return m_parameter; }

    private:
        BlendTreeTransformNode* m_transformNode = nullptr;
        FloatSliderParameter* m_parameter = nullptr;
    };


    // Add a reference node without any asset in it
    class AnimGraphReferenceNodeBaseTests : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            m_animGraph = AnimGraphFactory::Create<JustAReferenceNodeGraph>();
            m_rootStateMachine = m_animGraph->GetRootStateMachine();
        }

        BlendTree* GetBlendTree() const
        {
            return static_cast<JustAReferenceNodeGraph*>(m_animGraph.get())->GetBlendTree();
        }

        AnimGraphReferenceNode* GetReferenceNode() const
        {
            return static_cast<JustAReferenceNodeGraph*>(m_animGraph.get())->GetReferenceNode();
        }
    };

    // Basic test that just evaluates the node. Since the node is not doing anything,
    // The pose should not be affected.
    TEST_F(AnimGraphReferenceNodeBaseTests, VerifyRootTransform)
    {
        Evaluate();

        EXPECT_EQ(GetOutputTransform(), Transform::CreateIdentity());
    }

    // Add a reference node with an empty asset
    class AnimGraphReferenceNodeWithAssetTests : public AnimGraphReferenceNodeBaseTests
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphReferenceNodeBaseTests::ConstructGraph();

            auto animGraphAsset = ConstructReferenceGraphAsset();

            GetReferenceNode()->SetAnimGraphAsset(animGraphAsset);
            GetReferenceNode()->OnAssetReady(animGraphAsset);
        }

        virtual AZ::Data::Asset<Integration::AnimGraphAsset> ConstructReferenceGraphAsset()
        {
            AZ::Data::Asset<Integration::AnimGraphAsset> referenceAnimGraphAsset = AnimGraphAssetFactory::Create(AZ::Data::AssetId("{E8FBAEF1-CBC5-43C2-83C8-9F8812857494}"), AnimGraphFactory::Create<EmptyAnimGraph>());
            m_referenceAnimGraph = referenceAnimGraphAsset.Get()->GetAnimGraph();
            m_referenceAnimGraph->InitAfterLoading();
            return referenceAnimGraphAsset;
        }

        AnimGraph* m_referenceAnimGraph = nullptr;
    };

    // Load an empty anim graph into the reference node
    TEST_F(AnimGraphReferenceNodeWithAssetTests, VerifyRootTransform)
    {
        Evaluate();

        EXPECT_EQ(GetOutputTransform(), Transform::CreateIdentity());
    }

    class AnimGraphReferenceNodeWithContentsTests : public AnimGraphReferenceNodeWithAssetTests
    {
    public:
        void ConstructGraph() override
        {
            auto animGraph = AnimGraphFactory::Create<ReferenceNodeWithParameterGraph>();
            m_rootStateMachine = animGraph->GetRootStateMachine();
            m_parameter = animGraph->GetParameter();
            m_animGraph = AZStd::move(animGraph);

            m_referencedAsset = ConstructReferenceGraphAsset();
        }

        AZ::Data::Asset<Integration::AnimGraphAsset> ConstructReferenceGraphAsset() override
        {
            AZ::Data::Asset<Integration::AnimGraphAsset> referenceAnimGraphAsset = AnimGraphAssetFactory::Create(AZ::Data::AssetId("{E8FBAEF1-CBC5-43C2-83C8-9F8812857494}"), AnimGraphFactory::Create<BlendTreeTransformNodeAnimGraph>());
            m_referenceAnimGraph = referenceAnimGraphAsset.Get()->GetAnimGraph();
            m_referenceAnimGraph->InitAfterLoading();
            m_transformNode = static_cast<BlendTreeTransformNodeAnimGraph*>(m_referenceAnimGraph)->GetTransformNode();

            return referenceAnimGraphAsset;
        }

        void TearDown() override
        {
            m_referencedAsset.Release();
            AnimGraphReferenceNodeWithAssetTests::TearDown();
        }

        AZ::Data::Asset<Integration::AnimGraphAsset> m_referencedAsset;
        Parameter* m_parameter = nullptr;
        BlendTreeTransformNode* m_transformNode = nullptr;
    };

    TEST_F(AnimGraphReferenceNodeWithContentsTests, VerifyRootTransform)
    {
        GetReferenceNode()->SetAnimGraphAsset(m_referencedAsset);
        GetReferenceNode()->OnAssetReady(m_referencedAsset);

        GetEMotionFX().Update(0.0f);
        EXPECT_EQ(Transform::CreateIdentity(), GetOutputTransform());

        static_cast<MCore::AttributeFloat*>(m_animGraphInstance->GetParameterValue(static_cast<uint32>(m_animGraph->FindParameterIndex(m_parameter).GetValue())))->SetValue(1.0f);

        GetEMotionFX().Update(0.0f);
        EXPECT_EQ(Transform::CreateIdentity() * AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 0.0f, 0.0f)), GetOutputTransform());
    }

    class AnimGraphWithNestedReferencesTests
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            m_animGraph = AnimGraphFactory::Create<ReferenceNodeWithParameterGraph>();
            m_rootStateMachine = m_animGraph->GetRootStateMachine();

            m_secondLevelAsset = AnimGraphAssetFactory::Create(AZ::Data::AssetId("{5B05769E-2532-4B1E-A37B-E8CCB303E797}"), AnimGraphFactory::Create<ReferenceNodeWithParameterGraph>());
            auto thirdLevel = AnimGraphAssetFactory::Create(AZ::Data::AssetId("{2D605BAF-5C71-44AE-884F-89338AD49F03}"), AnimGraphFactory::Create<ReferenceNodeWithParameterGraph>());
            auto bottomLevel = AnimGraphAssetFactory::Create(AZ::Data::AssetId("{C23E2C8D-72C0-4EDE-BB37-48993A3EE83D}"), AnimGraphFactory::Create<BlendTreeTransformNodeAnimGraph>());

            m_secondLevelAsset->GetAnimGraph()->InitAfterLoading();
            thirdLevel->GetAnimGraph()->InitAfterLoading();
            bottomLevel->GetAnimGraph()->InitAfterLoading();

            auto thirdReferenceNode = static_cast<ReferenceNodeWithParameterGraph*>(thirdLevel.Get()->GetAnimGraph())->GetReferenceNode();
            thirdReferenceNode->SetAnimGraphAsset(bottomLevel);
            thirdReferenceNode->OnAssetReady(bottomLevel);

            auto secondReferenceNode = static_cast<ReferenceNodeWithParameterGraph*>(m_secondLevelAsset.Get()->GetAnimGraph())->GetReferenceNode();
            secondReferenceNode->SetAnimGraphAsset(thirdLevel);
            secondReferenceNode->OnAssetReady(thirdLevel);

            m_topLevelParameter = static_cast<ReferenceNodeWithParameterGraph*>(m_animGraph.get())->GetParameter();
        }

        void TearDown() override
        {
            m_secondLevelAsset.Release();
            AnimGraphFixture::TearDown();
        }

        AZ::Data::Asset<Integration::AnimGraphAsset> m_secondLevelAsset{};

        Parameter* m_topLevelParameter = nullptr;
    };

    TEST_F(AnimGraphWithNestedReferencesTests, VerifyRootTransform)
    {
        // The AnimGraphFixture doesn't call InitAfterLoading until after
        // ConstructGraph() is done, and these bits have to run after
        // InitAfterLoading()
        auto firstReferenceNode = static_cast<ReferenceNodeWithParameterGraph*>(m_animGraph.get())->GetReferenceNode();
        firstReferenceNode->SetAnimGraphAsset(m_secondLevelAsset);
        firstReferenceNode->OnAssetReady(m_secondLevelAsset);

        GetEMotionFX().Update(0.0f);
        EXPECT_EQ(Transform::CreateIdentity(), GetOutputTransform());

        // Changing this one parameter value should change it through all 3
        // layers of reference nodes, down to the referenced Transform node
        static_cast<MCore::AttributeFloat*>(m_animGraphInstance->GetParameterValue(static_cast<uint32>(m_animGraph->FindParameterIndex(m_topLevelParameter).GetValue())))->SetValue(1.0f);

        GetEMotionFX().Update(0.0f);
        EXPECT_EQ(Transform::CreateIdentity() * AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 0.0f, 0.0f)), GetOutputTransform());
    }

    ///////////////////////////////////////////////////////////////////////////

    class AnimGraphReferenceNodeDeferredInitTests
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            /*
            +-Root state machine--------------------------------------------+
            |                                                               |
            |   +------------+       +---------------+       +----------+   |
            | =>|  BindPose  |------>| ReferenceNode |------>| EndState |   |
            |   +------------+       +---------------+       +----------+   |
            |                                                               |
            +---------------------------------------------------------------+

            +-Root state machine (referenceNode)----------------------------+
            |                                                               |
            |   +---------------+       +----------+                        |
            | =>|  RefBindPose  |------>| endState |                        |
            |   +---------------+       +----------+                        |
            |                                                               |
            +---------------------------------------------------------------+
            */
            AnimGraphFixture::ConstructGraph();

            AnimGraphBindPoseNode* entryState = aznew AnimGraphBindPoseNode();
            entryState->SetName("StateA");
            m_rootStateMachine->AddChildNode(entryState);
            m_rootStateMachine->SetEntryState(entryState);

            m_referenceNode = aznew AnimGraphReferenceNode();
            m_referenceNode->SetName("StateB (Reference)");
            m_rootStateMachine->AddChildNode(m_referenceNode);
            AddTransitionWithTimeCondition(entryState, m_referenceNode, 1.0f, 1.0f);

            AnimGraphBindPoseNode* endState = aznew AnimGraphBindPoseNode();
            endState->SetName("StateC");
            m_rootStateMachine->AddChildNode(endState);
            AddTransitionWithTimeCondition(m_referenceNode, endState, 1.0f, 1.0f);

            AnimGraph* referenceAnimGraph = CreateReferenceGraph();
            AZ::Data::Asset<Integration::AnimGraphAsset> animGraphAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::AnimGraphAsset>(AZ::Data::AssetId("{E8FBAEF1-CBC5-43C2-83C8-9F8812857494}"));
            animGraphAsset.GetAs<Integration::AnimGraphAsset>()->SetData(referenceAnimGraph);
            m_referenceNode->SetAnimGraphAsset(animGraphAsset);
            referenceAnimGraph->InitAfterLoading();
            m_referenceNode->SetAnimGraph(m_animGraph.get());
            m_referenceNode->OnAssetReady(animGraphAsset);
        }

        AnimGraph* CreateReferenceGraph()
        {
            AnimGraph* referenceAnimGraph = aznew AnimGraph();

            AnimGraphStateMachine* referenceRootSM = aznew AnimGraphStateMachine();
            referenceAnimGraph->SetRootStateMachine(referenceRootSM);

            AnimGraphBindPoseNode* referenceEntryState = aznew AnimGraphBindPoseNode();
            referenceEntryState->SetName("RefEntryState");
            referenceRootSM->AddChildNode(referenceEntryState);
            referenceRootSM->SetEntryState(referenceEntryState);

            AnimGraphBindPoseNode* referenceEndState = aznew AnimGraphBindPoseNode();
            referenceEndState->SetName("RefEndState");
            referenceRootSM->AddChildNode(referenceEndState);

            AddTransitionWithTimeCondition(referenceEntryState, referenceEndState, 1.0f, 1.0f);

            return referenceAnimGraph;
        }

        void TearDown() override
        {
            m_referenceNode->GetReferencedAnimGraphAsset().Release();
            AnimGraphFixture::TearDown();
        }

    public:
        AnimGraphReferenceNode* m_referenceNode = nullptr;
    };

    TEST_F(AnimGraphReferenceNodeDeferredInitTests, DeferredReferenceGraphTest)
    {
        const size_t numObjects = m_animGraph->GetNumObjects();
        EXPECT_EQ(numObjects, m_animGraphInstance->GetNumUniqueObjectDatas())
            << "There should be a unique data placeholder for each anim graph object.";
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), 0)
            << "Unique datas should not be allocated yet.";

        // Entry state active, conditions are watching.
        GetEMotionFX().Update(0.0f);
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), 3)
            << "Exactly 3 unique datas should be allocated now, the root state machine, the entry state (StateA) as well as the time condition.";

        // Transitioning from entry to reference state.
        GetEMotionFX().Update(1.5f);
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), 6)
           << "As we're transitioning, unique datas from the root SM, State A (entry node), the transition (A->B) + condition, State B and the new condition of B->C as the count-down timer already started as soon as B gets activated.";

        const AnimGraphReferenceNode::UniqueData* referenceNodeUniqueData = static_cast<AnimGraphReferenceNode::UniqueData*>(m_animGraphInstance->GetUniqueObjectData(m_referenceNode->GetObjectIndex()));
        EXPECT_NE(referenceNodeUniqueData, nullptr)
            << "Unique data for reference node should have already been allocated, as we're transitioning into the node.";
        const AnimGraphInstance* referenceAnimGraphInstance = referenceNodeUniqueData->m_referencedAnimGraphInstance;
        EXPECT_NE(referenceAnimGraphInstance, nullptr)
            << "Anim graph instance for reference node should be created already, as we're transitioning into the reference node.";
        EXPECT_EQ(referenceAnimGraphInstance->CalcNumAllocatedUniqueDatas(), 3)
            << "Exactly 3 unique datas should be allocated in the reference instance now, the root state machine, the entry state (RefEntryState) as well as the time condition.";

        // The reference node state machine transitions into the end state.
        GetEMotionFX().Update(1.0f);
        EXPECT_EQ(referenceAnimGraphInstance->CalcNumAllocatedUniqueDatas(), 5)
            << "The transition as well as the end state unique datas are now also allocated.";
        const AnimGraph* refAnimGraph = referenceAnimGraphInstance->GetAnimGraph();
        EXPECT_EQ(referenceAnimGraphInstance->CalcNumAllocatedUniqueDatas(), refAnimGraph->GetNumObjects())
            << "All objects should have their unique datas allocated now.";

        // The root state machine transitioned into the end state.
        GetEMotionFX().Update(1.0f);
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), 8)
            << "The last transition as well as the end state of the root state machine unique datas should now be allocated.";
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), numObjects)
            << "We should have reached all states, transitions and conditions.";
    }

    ///////////////////////////////////////////////////////////////////////////

    using AnimGraphReferenceNodeCircularDependencyDetectionTests = SystemComponentFixture;

    TEST_F(AnimGraphReferenceNodeCircularDependencyDetectionTests, CircularDependencyDetectionTest)
    {
        AZ::Data::Asset<Integration::AnimGraphAsset> assetA = AnimGraphAssetFactory::Create(AZ::Data::AssetId("{1CB9DC29-5063-4F0B-BF31-4610C8E683EA}"), AnimGraphFactory::Create<JustAReferenceNodeGraph>());
        AZ::Data::Asset<Integration::AnimGraphAsset> assetB = AnimGraphAssetFactory::Create(AZ::Data::AssetId("{4EE7A2F6-5982-4DBE-8F66-03BEB456520A}"), AnimGraphFactory::Create<JustAReferenceNodeGraph>());

        AnimGraph* animGraphA = assetA->GetAnimGraph();
        AnimGraph* animGraphB = assetB->GetAnimGraph();

        animGraphA->InitAfterLoading();
        animGraphB->InitAfterLoading();

        AnimGraphReferenceNode* refNodeA = static_cast<AnimGraphReferenceNode*>(animGraphA->GetRootStateMachine()->GetChildNode(0)->GetChildNode(0));
        AnimGraphReferenceNode* refNodeB = static_cast<AnimGraphReferenceNode*>(animGraphB->GetRootStateMachine()->GetChildNode(0)->GetChildNode(0));

        refNodeA->SetAnimGraphAsset(assetB);
        refNodeB->SetAnimGraphAsset(assetA);

        refNodeA->OnAssetReady(assetB);
        refNodeB->OnAssetReady(assetA);

        // Cycle detection for AnimGraphs with no instances only works when
        // we're in editor mode
        GetEMotionFX().SetIsInEditorMode(true);
        AZStd::unordered_set<const AnimGraphNode*> nodes;
        EXPECT_TRUE(animGraphA->GetRootStateMachine()->RecursiveDetectCycles(nodes));
        GetEMotionFX().SetIsInEditorMode(false);

        // Break the cyclic reference to allow memory to be released
        refNodeA->SetAnimGraphAsset({});
    }
} // end namespace EMotionFX
