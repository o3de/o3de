/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QApplication>
#include <QtTest>
#include <QAbstractItemModelTester>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <Source/Integration/Assets/AnimGraphAsset.h>

#include <Tests/TestAssetCode/AnimGraphAssetFactory.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/ProvidesUI/AnimGraph/SimpleAnimGraphUIFixture.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/Mocks/EventHandler.h>

namespace EMotionFX
{
    TEST_F(SimpleAnimGraphUIFixture, ResetAnimGraph)
    {
        // This test checks that we can reset a graph without any problem.
        EMStudio::AnimGraphModel& animGraphModel = m_animGraphPlugin->GetAnimGraphModel();

        AZStd::string commandResult;
        MCore::CommandGroup group;
        CommandSystem::ClearAnimGraphsCommand(&group);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(group, commandResult)) << commandResult.c_str();

        EXPECT_EQ(GetAnimGraphManager().GetNumAnimGraphs(), 0);
        EXPECT_EQ(animGraphModel.GetFocusedAnimGraph(), nullptr);
        m_animGraph = nullptr;
    }

    TEST_F(SimpleAnimGraphUIFixture, FocusRemainValidAfterDeleteFocus)
    {
        // This test checks that a focused item can be deleted, and afterward the focus will get set correctly.
        EMStudio::AnimGraphModel& animGraphModel = m_animGraphPlugin->GetAnimGraphModel();
        AnimGraphNode* motionNode = m_animGraph->RecursiveFindNodeByName("testMotion");
        EXPECT_TRUE(motionNode);
        AnimGraphNode* blendTreeNode = m_animGraph->RecursiveFindNodeByName("testBlendTree");
        EXPECT_TRUE(blendTreeNode);

        // Focus on the motion node.
        const QModelIndex motionNodeModelIndex = animGraphModel.FindFirstModelIndex(motionNode);
        animGraphModel.Focus(motionNodeModelIndex);
        EXPECT_EQ(motionNodeModelIndex, animGraphModel.GetFocus());

        // Delete the motion Node.
        AZStd::string removeCommand = AZStd::string::format("AnimGraphRemoveNode -animGraphID %d -name testMotion", m_animGraphId);
        AZStd::string commandResult;
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(removeCommand, commandResult)) << commandResult.c_str();

        // The focus should change.
        const QModelIndex focusIndex = animGraphModel.GetFocus();
        EXPECT_TRUE(focusIndex.isValid()) << "AnimGraphModel should have a valid index after removing the focus node.";
        EXPECT_EQ(focusIndex, animGraphModel.FindFirstModelIndex(m_animGraph->GetRootStateMachine())) << "the root statemachine node should become the new focus";
    }

    TEST_F(SimpleAnimGraphUIFixture, ParametersWindowFocusChange)
    {
        // This test checks that parameters window behave expected after model changes.
        EMStudio::AnimGraphModel& animGraphModel = m_animGraphPlugin->GetAnimGraphModel();
        AnimGraphNode* motionNode = m_animGraph->RecursiveFindNodeByName("testMotion");
        EXPECT_TRUE(motionNode);
        AnimGraphNode* blendTreeNode = m_animGraph->RecursiveFindNodeByName("testBlendTree");
        EXPECT_TRUE(blendTreeNode);

        // Focus on the motion node.
        const QModelIndex motionNodeModelIndex = animGraphModel.FindFirstModelIndex(motionNode);
        animGraphModel.Focus(motionNodeModelIndex);

        // Check the parameters window
        const EMStudio::ParameterWindow* parameterWindow = m_animGraphPlugin->GetParameterWindow();
        EXPECT_EQ(parameterWindow->GetTopLevelItemCount(), 3) << "Should be 3 parameters added in the parameters window.";

        // Force the model to look at an invalid index. This should reset the parameters window.
        animGraphModel.Focus(QModelIndex());
        EXPECT_EQ(parameterWindow->GetTopLevelItemCount(), 0) << "Should be 0 parameters in the parameters window after reset.";

        // Force the model to look back at the motion node.
        animGraphModel.Focus(motionNodeModelIndex);
        EXPECT_EQ(parameterWindow->GetTopLevelItemCount(), 3) << "Should be 3 parameters added in the parameters window.";

        // Delete the motion node.
        AZStd::string removeCommand = AZStd::string::format("AnimGraphRemoveNode -animGraphID %d -name testMotion", m_animGraphId);
        AZStd::string commandResult;
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(removeCommand, commandResult)) << commandResult.c_str();

        // The parameter windows shouldn't be affected
        EXPECT_EQ(parameterWindow->GetTopLevelItemCount(), 3) << "Should be 3 parameters added in the parameters window.";
    }

    // Turns Qt messages into Google Test assertions
    class AssertNoQtLogWarnings
    {
        static void messageHandlerTest(QtMsgType type, [[maybe_unused]] const QMessageLogContext& context, const QString& msg)
        {
            QByteArray localMsg = msg.toLocal8Bit();
            switch (type)
            {
            case QtDebugMsg:
                //printf("Debug: %s: %s\n", context.category, localMsg.constData());
                break;
            case QtInfoMsg:
                //printf("Info: %s: %s\n", context.category, localMsg.constData());
                break;
            case QtWarningMsg:
            case QtCriticalMsg:
            case QtFatalMsg:
                FAIL() << msg.toStdString();
                break;
            }
        }

    public:
        AssertNoQtLogWarnings()
            : m_oldHandler(qInstallMessageHandler(messageHandlerTest))
        {
            QLoggingCategory::setFilterRules(QStringLiteral("qt.modeltest=true"));
        }

        ~AssertNoQtLogWarnings()
        {
            // Install default message handler
            qInstallMessageHandler(m_oldHandler);
        }

    private:
        QtMessageHandler m_oldHandler;
    };

    // Tests that use this fixture validate the AnimGraphModel by using the
    // QAbstractItemModelTester. It will trigger Qt warnings if any action made
    // by the model violates the API contract that QAbstractItemModels must
    // adhere to. Those warnings are then turned into Google Test failures
    // using the Qt warning redirector class above.
    class AnimGraphModelFixture
        : public UIFixture
    {
    public:
        void SetUp() override
        {
            UIFixture::SetUp();

            auto* animGraphPlugin = EMStudio::GetPluginManager()->FindActivePlugin<EMStudio::AnimGraphPlugin>();

            m_model = &animGraphPlugin->GetAnimGraphModel();
            m_modelTester = AZStd::make_unique<QAbstractItemModelTester>(m_model, QAbstractItemModelTester::FailureReportingMode::Warning);

            m_model->connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved, [this](const QModelIndex& parent, int startRow, int endRow)
            {
                for (int i = startRow; i <= endRow; ++i)
                {
                    QModelIndex aboutToBeRemoved = m_model->index(i, 0, parent);
                    EXPECT_TRUE(aboutToBeRemoved.isValid());
                    EXPECT_FALSE(m_model->data(aboutToBeRemoved).isNull());
                    //qDebug(QLoggingCategory("qt.modeltest")) << "     Data that will be removed:" << m_model->data(aboutToBeRemoved);
                }
            });

            EmptyAnimGraph::Reflect(GetSerializeContext());
            TwoMotionNodeAnimGraph::Reflect(GetSerializeContext());
            OneBlendTreeNodeAnimGraph::Reflect(GetSerializeContext());
        }

        void CallOnAnimGraphAssetChanged(AnimGraphReferenceNode* referenceNode)
        {
            // AnimGraphReferenceNode::OnAnimGraphAssetChanged is registered as the ChangeNotify method when the
            // reference node's m_animGraphAsset member is changed by the reflected property editor. In a test, the RPE
            // is not used to change the value, so the ChangeNotify callback must be invoked directly. That method is
            // also private, so a direct call is not possible. Look up the ChangeNotify handler for that class member,
            // and invoke it.
            const auto& referenceNodeClassElements = GetSerializeContext()->FindClassData(azrtti_typeid<AnimGraphReferenceNode>())->m_editData->m_elements;
            const auto animGraphElement = AZStd::find_if(referenceNodeClassElements.begin(), referenceNodeClassElements.end(), [](const AZ::Edit::ElementData& elementData)
            {
                return AZ::StringFunc::Equal(elementData.m_name, "Anim graph");
            });
            ASSERT_NE(animGraphElement, referenceNodeClassElements.end());
            animGraphElement->FindAttribute(AZ::Edit::Attributes::ChangeNotify);
            bool somethingCalled = false;
            for (const auto& [attributeId, attribute] : animGraphElement->m_attributes)
            {
                if (attributeId != AZ::Edit::Attributes::ChangeNotify)
                {
                    continue;
                }

                somethingCalled |= AZ::AttributeInvoker(referenceNode, attribute).Invoke<void>();
            }
            EXPECT_TRUE(somethingCalled) << "No call made to OnAnimGraphAssetChanged";
        }

        EMStudio::AnimGraphModel* GetModel() const { return m_model; }

    private:
        EMStudio::AnimGraphModel* m_model;
        AZStd::unique_ptr<QAbstractItemModelTester> m_modelTester;
        AssertNoQtLogWarnings m_warningRedirector;
    };

    TEST_F(AnimGraphModelFixture, CanAddASingleNodeToTheAnimGraphModel)
    {
        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("CreateAnimGraph -animGraphId 0", result)) << result.c_str();
        }

        auto* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(0);

        CommandSystem::CreateAnimGraphNode(nullptr, animGraph, azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), "Motion", animGraph->GetRootStateMachine(), 0, 0);
    }

    TEST_F(AnimGraphModelFixture, CanAddAndRemoveASingleNodeToTheAnimGraphModel)
    {
        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("CreateAnimGraph -animGraphId 0", result)) << result.c_str();
        }

        auto* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(0);

        CommandSystem::CreateAnimGraphNode(nullptr, animGraph, azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), "Motion", animGraph->GetRootStateMachine(), 0, 0);

        CommandSystem::DeleteNodes(animGraph, {"Motion0"});
    }

    TEST_F(AnimGraphModelFixture, CanAddAndRemoveNestedNodesToTheAnimGraphModel)
    {
        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("CreateAnimGraph -animGraphId 0", result)) << result.c_str();
        }

        auto* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(0);

        CommandSystem::CreateAnimGraphNode(nullptr, animGraph, azrtti_typeid<EMotionFX::BlendTree>(), "BlendTree", animGraph->GetRootStateMachine(), 0, 0);
        AnimGraphNode* blendTree = animGraph->RecursiveFindNodeByName("BlendTree0");

        CommandSystem::CreateAnimGraphNode(nullptr, animGraph, azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), "Motion", blendTree, 0, 0);

        CommandSystem::DeleteNodes(animGraph, {"BlendTree0"});
    }

    TEST_F(AnimGraphModelFixture, CanRemoveNodeFromInsideReferencedGraphThatAppearsInTwoPlacesInTheModel)
    {
        using testing::Eq;

        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("CreateAnimGraph -animGraphId 0", result)) << result.c_str();
        }

        auto* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(0);

        CommandSystem::CreateAnimGraphNode(nullptr, animGraph, azrtti_typeid<EMotionFX::AnimGraphReferenceNode>(), "Reference", animGraph->GetRootStateMachine(), 0, 0);
        auto* referenceNode = static_cast<AnimGraphReferenceNode*>(animGraph->RecursiveFindNodeByName("Reference0"));

        AnimGraph* referenceAnimGraph = nullptr;
        {
            AZ::Data::Asset<Integration::AnimGraphAsset> referenceAnimGraphAsset =
                AnimGraphAssetFactory::Create(AZ::Data::AssetId("{EC53A3C1-DDAF-46AA-B091-041449FC7FEE}"), AnimGraphFactory::Create<OneBlendTreeParameterNodeAnimGraph>());
            referenceAnimGraph = referenceAnimGraphAsset->GetAnimGraph();
            referenceAnimGraph->SetFileName("ReferencedAnimGraph.animgraph");
            referenceAnimGraph->InitAfterLoading();

            referenceAnimGraphAsset->SetStatus(AZ::Data::AssetData::AssetStatus::Queued);
            referenceNode->SetAnimGraphAsset(referenceAnimGraphAsset);
            CallOnAnimGraphAssetChanged(referenceNode);
            referenceAnimGraphAsset->SetStatus(AZ::Data::AssetData::AssetStatus::Ready);

            // Let the AnimGraphModel know that the anim graph asset has been loaded
            AZ::Data::AssetBus::Broadcast(&AZ::Data::AssetBus::Events::OnAssetReady, referenceAnimGraphAsset);
        }

        {
            auto* parameterNode = static_cast<BlendTreeParameterNode*>(referenceAnimGraph->GetRootStateMachine()->GetChildNode(0)->GetChildNode(0));
            EXPECT_TRUE(parameterNode);

            const QModelIndexList modelIndexes = GetModel()->FindModelIndexes(parameterNode);
            QList<QPersistentModelIndex> modelIndexesForParameterNode;
            AZStd::copy(modelIndexes.begin(), modelIndexes.end(), AZStd::back_inserter(modelIndexesForParameterNode));
            EXPECT_THAT(modelIndexesForParameterNode.size(), Eq(2));

            const auto modelIndexIsValid = testing::Truly([](const QPersistentModelIndex& i) { return i.isValid(); });
            const auto eachModelIndexIsValid = testing::Each(modelIndexIsValid);
            const auto eachModelIndexIsInvalid = testing::Each(testing::Not(modelIndexIsValid));

            EXPECT_THAT(modelIndexesForParameterNode, eachModelIndexIsValid);
            CommandSystem::DeleteNodes(referenceAnimGraph, {parameterNode->GetNameString()});
            EXPECT_THAT(modelIndexesForParameterNode, eachModelIndexIsInvalid);
        }

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    // This test simulates an asset reload. It ensures that the model stays
    // stable while the new reference graph is loaded.
    // To reload an asset, a separate asset is created with its own AssetData
    // pointer, but the same AssetId.
    // Normally, the only holder of an Asset reference is the ReferenceNode
    // itself. The Asset variables are scoped so that the ReferenceNode is the
    // only holder of the Asset. When the asset is reloaded, the ReferenceNode
    // assigns over its old Asset. Since it was the last holder, the asset is
    // released, and the underlying AnimGraph is destroyed.
    TEST_F(AnimGraphModelFixture, CanReloadAReferenceNodesReferencedGraph)
    {
        using testing::Eq;

        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("CreateAnimGraph -animGraphId 0", result)) << result.c_str();
        }

        auto* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(0);

        CommandSystem::CreateAnimGraphNode(nullptr, animGraph, azrtti_typeid<EMotionFX::AnimGraphReferenceNode>(), "Reference", animGraph->GetRootStateMachine(), 0, 0);
        auto* referenceNode = static_cast<AnimGraphReferenceNode*>(animGraph->RecursiveFindNodeByName("Reference0"));

        const AZ::Data::AssetId assetId("{B359FEA1-7628-4981-91E2-63F58413EEF5}");

        AnimGraph* referenceAnimGraph = nullptr;
        {
            AZ::Data::Asset<Integration::AnimGraphAsset> referenceAnimGraphAsset =
                AnimGraphAssetFactory::Create(assetId, AnimGraphFactory::Create<OneBlendTreeParameterNodeAnimGraph>());
            referenceAnimGraph = referenceAnimGraphAsset->GetAnimGraph();
            referenceAnimGraph->SetFileName("ReferencedAnimGraph.animgraph");
            referenceAnimGraph->InitAfterLoading();
            referenceAnimGraph->SetIsOwnedByRuntime(true);
            referenceAnimGraphAsset->SetStatus(AZ::Data::AssetData::AssetStatus::Queued);
            referenceNode->SetAnimGraphAsset(referenceAnimGraphAsset);
            CallOnAnimGraphAssetChanged(referenceNode);
            referenceAnimGraphAsset->SetStatus(AZ::Data::AssetData::AssetStatus::Ready);

            // In normal operation, asset loading results in this sequence of events:
            //
            // AnimGraphAssetHandler::OnInitAsset
            //     sets owned by runtime = true
            // AnimGraphModel::OnAssetReady
            //     not added to top-level because is owned by runtime = true
            // AnimGraphReferenceNode::OnAssetReady
            //     sets owned by runtime = false
            //     emits OnReferenceAnimGraphChanged
            //         AnimGraphModel::OnReferenceAnimGraphChanged
            //             adds nodes of the graph to the right places in the model

            // Let the AnimGraphModel know that the anim graph asset has been loaded
            AZ::Data::AssetBus::Broadcast(&AZ::Data::AssetBus::Events::OnAssetReady, referenceAnimGraphAsset);
        }


        auto* parameterNode = static_cast<BlendTreeParameterNode*>(referenceAnimGraph->GetRootStateMachine()->GetChildNode(0)->GetChildNode(0));
        EXPECT_TRUE(parameterNode);

        QModelIndexList modelIndexesForParameterNode = GetModel()->FindModelIndexes(parameterNode);
        EXPECT_THAT(modelIndexesForParameterNode.size(), Eq(1));
        QPersistentModelIndex index = modelIndexesForParameterNode[0];
        EXPECT_TRUE(index.isValid());

        {
            auto* handler = static_cast<Integration::AnimGraphAssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<Integration::AnimGraphAsset>()));
            AZ::Data::Asset<Integration::AnimGraphAsset> newAsset{handler->CreateAsset(assetId, AZ::AzTypeInfo<Integration::AnimGraphAsset>::Uuid()), AZ::Data::AssetLoadBehavior::Default};
            newAsset->SetData(AnimGraphFactory::Create<OneBlendTreeParameterNodeAnimGraph>().release());
            newAsset->GetAnimGraph()->SetFileName("ReferencedAnimGraph.animgraph");
            newAsset->GetAnimGraph()->InitAfterLoading();
            newAsset->GetAnimGraph()->SetIsOwnedByRuntime(true);
            newAsset->SetStatus(AZ::Data::AssetData::AssetStatus::Ready);

            // In normal operation, asset reloading results in this sequence of events:
            //
            // AnimGraphAssetHandler::OnInitAsset
            //     sets owned by runtime = true
            // AnimGraphModel::OnAssetReloaded
            //     not added to top-level because is owned by runtime = true
            // AnimGraphReferenceNode::OnAssetReloaded
            //     sets owned by runtime = false
            //     emits OnReferenceAnimGraphAboutToBeChanged
            //         AnimGraphModel::OnReferenceAnimGraphAboutToBeChanged
            //             removes child nodes of the existing reference node
            //     releases reference to old asset, potentially deleting the old anim graph
            //     emits OnReferenceAnimGraphChanged
            //         AnimGraphModel::OnReferenceAnimGraphChanged
            //             adds nodes of the graph to the right places in the model
            AZ::Data::AssetBus::Broadcast(&AZ::Data::AssetBus::Events::OnAssetReloaded, newAsset);
        }
        EXPECT_FALSE(index.isValid());

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    TEST_F(AnimGraphModelFixture, CanReloadAnActivatedReferenceNodesReferencedGraph)
    {
        using testing::Eq;
        using testing::Not;

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 1);

        auto motionSet = AZStd::make_unique<EMotionFX::MotionSet>();

        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("CreateAnimGraph -animGraphId 0", result)) << result.c_str();
        }

        auto* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(0);
        auto* actorInstance = EMotionFX::ActorInstance::Create(actorAsset->GetActor());
        auto* animGraphInstance = EMotionFX::AnimGraphInstance::Create(animGraph, actorInstance, motionSet.get());
        actorInstance->SetAnimGraphInstance(animGraphInstance);

        GetEMotionFX().Update(0.0f);

        CommandSystem::CreateAnimGraphNode(nullptr, animGraph, azrtti_typeid<EMotionFX::AnimGraphReferenceNode>(), "Reference", animGraph->GetRootStateMachine(), 0, 0);
        auto* referenceNode = static_cast<AnimGraphReferenceNode*>(animGraph->RecursiveFindNodeByName("Reference0"));

        const AZ::Data::AssetId assetId("{B359FEA1-7628-4981-91E2-63F58413EEF5}");

        AnimGraph* referenceAnimGraph = nullptr;
        {
            // Using blocks here ensure that we don't keep an extra reference to the Asset
            AZ::Data::Asset<Integration::AnimGraphAsset> referenceAnimGraphAsset =
                AnimGraphAssetFactory::Create(assetId, AnimGraphFactory::Create<OneBlendTreeParameterNodeAnimGraph>());
            referenceAnimGraph = referenceAnimGraphAsset->GetAnimGraph();
            referenceAnimGraph->SetFileName("ReferencedAnimGraph.animgraph");
            referenceAnimGraph->SetIsOwnedByRuntime(true);
            referenceAnimGraphAsset->SetStatus(AZ::Data::AssetData::AssetStatus::Queued);
            referenceNode->SetAnimGraphAsset(referenceAnimGraphAsset);
            CallOnAnimGraphAssetChanged(referenceNode);
            referenceAnimGraphAsset->SetStatus(AZ::Data::AssetData::AssetStatus::Ready);

            AZ::Data::AssetBus::Broadcast(&AZ::Data::AssetBus::Events::OnAssetReady, referenceAnimGraphAsset);
        }

        const auto* refNodeUniqueData = static_cast<AnimGraphReferenceNode::UniqueData*>(referenceNode->FindOrCreateUniqueNodeData(animGraphInstance));
        ASSERT_TRUE(refNodeUniqueData);
        const auto* referenceAnimGraphInstance = refNodeUniqueData->m_referencedAnimGraphInstance;
        EXPECT_TRUE(referenceAnimGraphInstance);

        {
            auto* handler = static_cast<Integration::AnimGraphAssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<Integration::AnimGraphAsset>()));
            AZ::Data::Asset<Integration::AnimGraphAsset> newAsset{handler->CreateAsset(assetId, AZ::AzTypeInfo<Integration::AnimGraphAsset>::Uuid()), AZ::Data::AssetLoadBehavior::Default};
            newAsset->SetData(AnimGraphFactory::Create<OneBlendTreeParameterNodeAnimGraph>().release());
            newAsset->GetAnimGraph()->SetFileName("ReferencedAnimGraph.animgraph");
            newAsset->GetAnimGraph()->SetIsOwnedByRuntime(true);

            EMotionFX::MockEventHandler eventHandler;
            EXPECT_CALL(eventHandler, GetHandledEventTypes())
                .WillRepeatedly(testing::Return(AZStd::vector<EventTypes> { EVENT_TYPE_ON_CREATE_ANIM_GRAPH_INSTANCE, EVENT_TYPE_ON_DELETE_ANIM_GRAPH_INSTANCE, }));
            {
                testing::InSequence deleteThenCreateCalledInSequence;
                EXPECT_CALL(eventHandler, OnDeleteAnimGraphInstance(refNodeUniqueData->m_referencedAnimGraphInstance))
                    .Times(1);
                EXPECT_CALL(eventHandler, OnCreateAnimGraphInstance(testing::_))
                    .Times(1);
            }
            GetEventManager().AddEventHandler(&eventHandler);

            AZ::Data::AssetBus::Broadcast(&AZ::Data::AssetBus::Events::OnAssetReloaded, newAsset);

            GetEventManager().RemoveEventHandler(&eventHandler);
        }

        GetEMotionFX().Update(0.1f);

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
} // namespace EMotionFX
