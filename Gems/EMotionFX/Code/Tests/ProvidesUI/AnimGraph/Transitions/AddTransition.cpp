/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetCommon.h>
#include <Tests/UI/UIFixture.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <Integration/Assets/AnimGraphAsset.h>
#include <QApplication>
#include <QtTest>

namespace QTest
{
    extern int Q_TESTLIB_EXPORT defaultMouseDelay();
} // namespace QTest

namespace EMotionFX
{
    class CanAddTransitionAnimGraph
        : public TwoMotionNodeAnimGraph
    {
    public:
        CanAddTransitionAnimGraph()
        {
            GetMotionNodeA()->SetName("node0");
            GetMotionNodeA()->SetVisualPos(0, 0);
            GetMotionNodeB()->SetName("node1");
            GetMotionNodeB()->SetVisualPos(100, 0);
        }
    };

    TEST_F(UIFixture, CanAddTransition)
    {
        RecordProperty("test_case_id", "C21948784");

        AnimGraphFactory::ReflectTestTypes(GetSerializeContext());

        EMotionFX::Integration::AnimGraphAsset* animGraphAsset = aznew EMotionFX::Integration::AnimGraphAsset();
        animGraphAsset->SetData(AnimGraphFactory::Create<CanAddTransitionAnimGraph>().release());
        auto animGraph = static_cast<TwoMotionNodeAnimGraph*>(animGraphAsset->GetAnimGraph());

        animGraph->InitAfterLoading();
        AZ::Data::Asset<EMotionFX::Integration::AnimGraphAsset> asset(animGraphAsset, AZ::Data::AssetLoadBehavior::Default);
        AZ::Data::AssetBus::Broadcast(&AZ::Data::AssetEvents::OnAssetReady, asset);

        const AnimGraphMotionNode* nodeA = animGraph->GetMotionNodeA();
        const AnimGraphMotionNode* nodeB = animGraph->GetMotionNodeB();

        EMStudio::GetMainWindow()->ApplicationModeChanged("AnimGraph");

        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "AnimGraph plugin not found!";

        EMStudio::AnimGraphModel& model = animGraphPlugin->GetAnimGraphModel();
        EXPECT_EQ(model.rowCount(), 1) << "AnimGraph does not exist in the model";

        animGraphPlugin->SetActiveAnimGraph(animGraphAsset->GetAnimGraph());

        EMStudio::BlendGraphWidget* blendGraphWidget = animGraphPlugin->GetGraphWidget();

        // The NodeGraph filters out non-visible nodes for efficiency. Resize
        // the graph to allow the nodes to be visible
        blendGraphWidget->resize(200, 200);

        // Zoom to show the whole graph. This updates the visibility flags of
        // the nodes
        animGraphPlugin->GetViewWidget()->ZoomSelected();

        const EMStudio::GraphNode* graphNodeForMotionNode0 = blendGraphWidget->GetActiveGraph()->FindGraphNode(nodeA);
        const EMStudio::GraphNode* graphNodeForMotionNode1 = blendGraphWidget->GetActiveGraph()->FindGraphNode(nodeB);
        const QPoint begin(graphNodeForMotionNode0->GetFinalRect().topRight() + QPoint(-2, 2));
        const QPoint end(graphNodeForMotionNode1->GetFinalRect().topLeft() + QPoint(2, 2));
        QTest::mousePress(static_cast<QWidget*>(blendGraphWidget), Qt::LeftButton, Qt::NoModifier, begin);
        {
            // QTest::mouseMove uses QCursor::setPos to generate a MouseMove
            // event to send to the resulting widget. This won't happen if the
            // widget isn't visible. So we need to send the event directly.
            QMouseEvent moveEvent(QMouseEvent::MouseMove, end, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            moveEvent.setTimestamp(QTest::lastMouseTimestamp += QTest::defaultMouseDelay());
            QApplication::instance()->notify(blendGraphWidget, &moveEvent);
        }
        QTest::mouseRelease(static_cast<QWidget*>(blendGraphWidget), Qt::LeftButton, Qt::NoModifier, end);

        // Ensure the transition was added to the root state machine
        ASSERT_EQ(animGraphAsset->GetAnimGraph()->GetRootStateMachine()->GetNumTransitions(), 1);

        // Ensure the transition is in the AnimGraphModel
        AnimGraphStateTransition* transition = animGraphAsset->GetAnimGraph()->GetRootStateMachine()->GetTransition(0);
        const QModelIndexList matches = model.match(
            /* starting index = */ model.index(0, 0, model.index(0, 0)),
            /* role = */ EMStudio::AnimGraphModel::ROLE_POINTER,
            /* value = */ QVariant::fromValue(static_cast<void*>(transition)),
            /* hits =*/ 1,
            Qt::MatchExactly
        );
        EXPECT_EQ(matches.size(), 1);

        QApplication::processEvents();
    }
} // namespace EMotionFX
