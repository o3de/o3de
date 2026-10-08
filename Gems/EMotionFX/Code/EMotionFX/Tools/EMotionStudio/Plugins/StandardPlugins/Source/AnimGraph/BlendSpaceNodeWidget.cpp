/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BlendSpaceNodeWidget.h"
#include "AnimGraphModel.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <Editor/AnimGraphEditorBus.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendSpace1DNode.h>
#include <EMotionFX/Source/BlendSpace2DNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MotionSetSelectionWindow.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <QMenu>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QToolButton>
#include <MCore/Source/Algorithms.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/MathUtils.h>

namespace EMStudio
{

// Current sample point
const float     BlendSpaceNodeWidget::s_currentSamplePointWidth         = 6.0f;
const QColor    BlendSpaceNodeWidget::s_currentSamplePointColor         = QColor(0xCF, 0x02, 0x1B);

// Motion points
const float     BlendSpaceNodeWidget::s_motionPointWidthMin      = 5.0f;
const float     BlendSpaceNodeWidget::s_motionPointWidthMax      = 10.0f;
const float     BlendSpaceNodeWidget::s_motionPointAlphaMin      = 1.0f;
const float     BlendSpaceNodeWidget::s_motionPointAlphaMax      = 1.0f;
const QColor    BlendSpaceNodeWidget::s_motionPointColor         = QColor(245, 166, 35);

const char*     BlendSpaceNodeWidget::s_warningOffsetForIcon     = "     ";

namespace
{
    // The selected actor, else any actor loaded in the Animation Editor, else any actor at all (read-only preview).
    EMotionFX::ActorInstance* FindPreviewActorInstance()
    {
        if (EMotionFX::ActorInstance* selected = CommandSystem::GetCommandManager()->GetCurrentSelection().GetFirstActorInstance())
        {
            return selected;
        }
        const EMotionFX::ActorManager& actorManager = EMotionFX::GetActorManager();
        if (EMotionFX::ActorInstance* editorActor = actorManager.GetFirstEditorActorInstance())
        {
            return editorActor;
        }
        return actorManager.GetNumActorInstances() > 0 ? actorManager.GetActorInstance(0) : nullptr;
    }

    bool MotionSetHasAnyMotion(const EMotionFX::MotionSet& motionSet, const EMotionFX::BlendSpaceNode& node)
    {
        const AZStd::vector<EMotionFX::BlendSpaceNode::BlendSpaceMotion>& motions = node.GetMotions();
        return AZStd::any_of(motions.begin(), motions.end(), [&motionSet](const EMotionFX::BlendSpaceNode::BlendSpaceMotion& motion)
            { return motionSet.RecursiveFindMotionEntryById(motion.GetMotionId()) != nullptr; });
    }

    // The Anim Graph panel's motion set when it holds the blend space's motions, else the first editor motion set that does.
    EMotionFX::MotionSet* FindPreviewMotionSet(const EMotionFX::BlendSpaceNode& node)
    {
        EMotionFX::MotionSet* selected = nullptr;
        EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(selected, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
        if (selected && MotionSetHasAnyMotion(*selected, node))
        {
            return selected;
        }
        const EMotionFX::MotionManager& motionManager = EMotionFX::GetMotionManager();
        for (size_t i = 0; i < motionManager.GetNumMotionSets(); ++i)
        {
            EMotionFX::MotionSet* candidate = motionManager.GetMotionSet(i);
            if (!candidate->GetIsOwnedByRuntime() && MotionSetHasAnyMotion(*candidate, node))
            {
                return candidate;
            }
        }
        return selected;
    }
} // namespace

class BlendSpaceNodeWidget::StopPreviewCommandCallback
    : public MCore::Command::Callback
{
public:
    explicit StopPreviewCommandCallback(BlendSpaceNodeWidget& widget)
        : MCore::Command::Callback(true, true)
        , m_widget(widget)
    {
    }

    bool Execute(MCore::Command*, const MCore::CommandLine&) override
    {
        m_widget.StopPreviewPlayback();
        return true;
    }

    bool Undo(MCore::Command*, const MCore::CommandLine&) override
    {
        m_widget.StopPreviewPlayback();
        return true;
    }

private:
    BlendSpaceNodeWidget& m_widget;
};

BlendSpaceNodeWidget::BlendSpaceNodeWidget()
{
    EMotionFX::GetEventManager().AddEventHandler(this);
    EMotionFX::ActorInstanceNotificationBus::Handler::BusConnect();
    // Restore the real graph before commands capture their undo state or replace actor playback.
    m_stopPreviewCallback = new StopPreviewCommandCallback(*this);
    for (const char* command : { "ActivateAnimGraph", "PlayMotion", "Select", "Unselect", "ClearSelection" })
    {
        CommandSystem::GetCommandManager()->RegisterCommandCallback(command, m_stopPreviewCallback);
    }
}


BlendSpaceNodeWidget::~BlendSpaceNodeWidget()
{
    m_dragNode = nullptr;
    QObject::disconnect(m_modelConnection);
    CommandSystem::GetCommandManager()->RemoveCommandCallback(m_stopPreviewCallback, true);
    ResetPreview();
    EMotionFX::ActorInstanceNotificationBus::Handler::BusDisconnect();
    EMotionFX::GetEventManager().RemoveEventHandler(this);
}

void BlendSpaceNodeWidget::InitializePreviewControls(QWidget* widget)
{
    m_previewWidget = widget;
    m_previewButton = new QToolButton(widget);
    m_previewButton->setObjectName("BlendSpacePreviewAnimation");
    m_previewButton->setText("Preview animation");
    m_previewButton->setCheckable(true);
    m_previewButton->setEnabled(false);
    m_previewButton->setToolTip("Play this blend space on the selected actor. Stop to restore the actor's previous anim graph.");
    m_previewButton->adjustSize();
    m_previewButton->move(10, 10);
    QObject::connect(m_previewButton, &QToolButton::toggled, widget,
        [this](bool enabled) { SetPreviewPlayback(enabled); });
}

void BlendSpaceNodeWidget::StopPreviewPlayback()
{
    if (m_previewPlaying && m_previewInstance)
    {
        EMotionFX::ActorInstance* actorInstance = m_previewInstance->GetActorInstance();
        // Another editor action may have replaced the preview. Preserve its new assignment.
        if (actorInstance->GetAnimGraphInstance() == m_previewInstance)
        {
            actorInstance->SetAnimGraphInstance(m_previousInstance);
        }
    }
    m_previousInstance = nullptr;
    m_previewPlaying = false;
    if (m_previewButton)
    {
        const QSignalBlocker blocker(m_previewButton);
        m_previewButton->setChecked(false);
        m_previewButton->setText("Preview animation");
        m_previewButton->adjustSize();
    }
}

void BlendSpaceNodeWidget::ResetPreview()
{
    StopPreviewPlayback();
    EMotionFX::AnimGraphInstance* instance = m_previewInstance;
    m_previewInstance = nullptr;
    m_previewNode = nullptr;
    m_previewSource = nullptr;
    m_dataNode = nullptr;
    if (instance)
    {
        instance->Destroy();
    }
    EMotionFX::AnimGraph* graph = m_previewGraph;
    m_previewGraph = nullptr;
    delete graph;
}

void BlendSpaceNodeWidget::SetPreviewPosition(const AZ::Vector2& position)
{
    m_previewPosition = position;
    if (auto* node1D = azdynamic_cast<EMotionFX::BlendSpace1DNode*>(m_previewNode))
    {
        node1D->SetCurrentPosition(position.GetX());
    }
    else if (auto* node2D = azdynamic_cast<EMotionFX::BlendSpace2DNode*>(m_previewNode))
    {
        node2D->SetCurrentPosition(position);
    }
}

EMotionFX::AnimGraphObjectData* BlendSpaceNodeWidget::GetBlendSpaceData(
    EMotionFX::BlendSpaceNode* node, const QPersistentModelIndex& modelIndex)
{
    if (!node)
    {
        return nullptr;
    }
    m_previewModelIndex = modelIndex;
    if (m_previewModel != modelIndex.model())
    {
        QObject::disconnect(m_modelConnection);
        m_previewModel = modelIndex.model();
        if (m_previewModel)
        {
            m_modelConnection = QObject::connect(m_previewModel, &QAbstractItemModel::dataChanged, m_previewWidget,
                [this] { m_previewDirty = true; });
        }
    }

    auto* liveInstance = modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
    if (liveInstance && liveInstance->GetAnimGraph() != node->GetAnimGraph())
    {
        liveInstance = nullptr;
    }
    EMotionFX::ActorInstance* actorInstance = liveInstance ? liveInstance->GetActorInstance() : FindPreviewActorInstance();
    EMotionFX::MotionSet* motionSet = liveInstance ? liveInstance->GetMotionSet() : nullptr;
    if (!motionSet)
    {
        motionSet = FindPreviewMotionSet(*node);
    }
    // Playback swaps the actor's anim graph, so it never takes over an actor owned by a game entity.
    const bool canPlay = actorInstance && motionSet && !actorInstance->GetIsOwnedByRuntime();
    m_previewButton->setEnabled(canPlay);

    if (m_previewPlaying && m_previewInstance && actorInstance
        && actorInstance->GetAnimGraphInstance() != m_previewInstance)
    {
        StopPreviewPlayback();
    }
    const bool wantPlayback = canPlay && m_previewButton->isChecked();
    if (liveInstance && liveInstance->GetIsOutputReady(node->GetObjectIndex()) && !wantPlayback)
    {
        ResetPreview();
        m_previewSource = node;
        m_dataNode = node;
        m_dataMotionSet = liveInstance->GetMotionSet();
        return liveInstance->FindOrCreateUniqueObjectData(node);
    }
    if (!actorInstance || !motionSet)
    {
        ResetPreview();
        m_previewStatus = !actorInstance ? "Load an actor in the Animation Editor to preview this blend space."
            : "Create or select a motion set containing this blend space's motions to preview it.";
        return nullptr;
    }

    if (m_previewSource != node || m_previewDirty || !m_previewInstance
        || m_previewInstance->GetActorInstance() != actorInstance || m_previewInstance->GetMotionSet() != motionSet)
    {
        ResetPreview();
        m_previewDirty = false;
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        m_previewNode = serializeContext ? serializeContext->CloneObject(node) : nullptr;
        if (!m_previewNode)
        {
            AZ_WarningOnce("EMotionFX", false, "Blend space preview: could not clone node '%s'.", node->GetName());
            m_previewStatus = "Could not build the blend space preview. See the log for details.";
            return nullptr;
        }
        // The preview contains only the blend space; its graph inputs and state machine are independent of the edited graph.
        m_previewNode->RemoveAllConnections();
        for (size_t i = 0; i < m_previewNode->GetInputPorts().size(); ++i)
        {
            m_previewNode->GetInputPort(i).m_connection = nullptr;
        }
        m_previewGraph = aznew EMotionFX::AnimGraph();
        m_previewGraph->SetIsOwnedByRuntime(true);
        m_previewGraph->SetRetargetingEnabled(node->GetAnimGraph()->GetRetargetingEnabled());
        auto* root = aznew EMotionFX::AnimGraphStateMachine();
        auto* blendTree = aznew EMotionFX::BlendTree();
        auto* finalNode = aznew EMotionFX::BlendTreeFinalNode();
        auto* inPlace = aznew EMotionFX::BlendTreeFloatConstantNode();
        inPlace->SetValue(1.0f);
        m_previewGraph->SetRootStateMachine(root);
        root->AddChildNode(blendTree);
        root->SetEntryState(blendTree);
        blendTree->AddChildNode(m_previewNode);
        blendTree->AddChildNode(finalNode);
        blendTree->AddChildNode(inPlace);
        finalNode->AddConnection(m_previewNode, 0, EMotionFX::BlendTreeFinalNode::INPUTPORT_POSE);
        if (auto* previewNode1D = azdynamic_cast<EMotionFX::BlendSpace1DNode*>(m_previewNode))
        {
            previewNode1D->SetEventFilterMode(EMotionFX::BlendSpaceNode::BSEVENTMODE_NONE);
            previewNode1D->AddConnection(inPlace, 0, EMotionFX::BlendSpace1DNode::INPUTPORT_INPLACE);
        }
        else if (auto* previewNode2D = azdynamic_cast<EMotionFX::BlendSpace2DNode*>(m_previewNode))
        {
            previewNode2D->SetEventFilterMode(EMotionFX::BlendSpaceNode::BSEVENTMODE_NONE);
            previewNode2D->AddConnection(inPlace, 0, EMotionFX::BlendSpace2DNode::INPUTPORT_INPLACE);
        }
        if (!m_previewGraph->InitAfterLoading())
        {
            AZ_WarningOnce("EMotionFX", false, "Blend space preview: could not initialize the preview graph for '%s'.", node->GetName());
            ResetPreview();
            m_previewStatus = "Could not build the blend space preview. See the log for details.";
            return nullptr;
        }
        m_previewSource = node;
        m_previewNode->SetInteractiveMode(true);
        SetPreviewPosition(m_previewPosition);
        m_previewInstance = EMotionFX::AnimGraphInstance::Create(m_previewGraph, actorInstance, motionSet);
        m_previewInstance->SetIsOwnedByRuntime(true);
        m_previewInstance->SetVisualizationEnabled(false);
        if (wantPlayback)
        {
            m_previousInstance = actorInstance->GetAnimGraphInstance();
            actorInstance->SetAnimGraphInstance(m_previewInstance);
            m_previewPlaying = true;
            const QSignalBlocker blocker(m_previewButton);
            m_previewButton->setChecked(true);
            m_previewButton->setText("Stop preview");
            m_previewButton->adjustSize();
        }
    }

    m_dataNode = m_previewNode;
    m_dataMotionSet = motionSet;
    auto* data = m_previewInstance->FindOrCreateUniqueObjectData(m_previewNode);
    if (!m_previewPlaying)
    {
        if (auto* previewNode1D = azdynamic_cast<EMotionFX::BlendSpace1DNode*>(m_previewNode))
        {
            previewNode1D->UpdatePreviewPosition(*static_cast<EMotionFX::BlendSpace1DNode::UniqueData*>(data), m_previewPosition.GetX());
        }
        else if (auto* previewNode2D = azdynamic_cast<EMotionFX::BlendSpace2DNode*>(m_previewNode))
        {
            previewNode2D->UpdatePreviewPosition(*static_cast<EMotionFX::BlendSpace2DNode::UniqueData*>(data), m_previewPosition);
        }
    }
    return data;
}

void BlendSpaceNodeWidget::SetPreviewPlayback(bool enabled)
{
    if (!enabled)
    {
        StopPreviewPlayback();
    }
    else if (m_previewSource)
    {
        GetBlendSpaceData(m_previewSource, m_previewModelIndex);
        if (m_previewInstance && !m_previewPlaying)
        {
            EMotionFX::ActorInstance* actorInstance = m_previewInstance->GetActorInstance();
            m_previousInstance = actorInstance->GetAnimGraphInstance();
            actorInstance->SetAnimGraphInstance(m_previewInstance);
            m_previewPlaying = true;
            m_previewButton->setText("Stop preview");
            m_previewButton->adjustSize();
        }
    }
    if (!m_previewPlaying)
    {
        StopPreviewPlayback();
    }
    m_previewWidget->update();
}

const AZStd::vector<EMotionFX::EventTypes> BlendSpaceNodeWidget::GetHandledEventTypes() const
{
    return { EMotionFX::EVENT_TYPE_ON_DELETE_ANIM_GRAPH_INSTANCE, EMotionFX::EVENT_TYPE_ON_DELETE_ANIM_GRAPH,
        EMotionFX::EVENT_TYPE_ON_DELETE_MOTION_SET, EMotionFX::EVENT_TYPE_ON_DELETE_MOTION, EMotionFX::EVENT_TYPE_ON_REMOVE_NODE };
}

void BlendSpaceNodeWidget::OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance)
{
    if (m_previewInstance && m_previewInstance->GetActorInstance() == actorInstance)
    {
        ResetPreview();
    }
}

void BlendSpaceNodeWidget::OnDeleteAnimGraphInstance(EMotionFX::AnimGraphInstance* instance)
{
    if (m_previousInstance == instance)
    {
        m_previousInstance = nullptr;
    }
    if (m_previewInstance == instance)
    {
        m_previewInstance = nullptr;
        m_previousInstance = nullptr;
        StopPreviewPlayback();
    }
}

void BlendSpaceNodeWidget::OnDeleteAnimGraph(EMotionFX::AnimGraph* graph)
{
    if (m_previewGraph == graph)
    {
        m_previewGraph = nullptr;
        m_previewNode = nullptr;
        m_previewSource = nullptr;
    }
    else if (m_previewSource && m_previewSource->GetAnimGraph() == graph)
    {
        ResetPreview();
    }
}

void BlendSpaceNodeWidget::OnDeleteMotionSet(EMotionFX::MotionSet* motionSet)
{
    if (!m_previewInstance)
    {
        return;
    }
    for (auto* usedSet = m_previewInstance->GetMotionSet(); usedSet; usedSet = usedSet->GetParentSet())
    {
        if (usedSet == motionSet)
        {
            ResetPreview();
            return;
        }
    }
}

void BlendSpaceNodeWidget::OnDeleteMotion(EMotionFX::Motion* motion)
{
    if (!m_previewInstance || !m_previewNode)
    {
        return;
    }
    auto* data = m_previewInstance->GetUniqueObjectData(m_previewNode->GetObjectIndex());
    if (!data)
    {
        return;
    }
    // Release only previews that reference this motion, before its motion data is destroyed.
    const auto usesMotion = [motion](const auto& motionInfos)
    {
        return AZStd::any_of(motionInfos.begin(), motionInfos.end(), [motion](const auto& info)
            { return info.m_motionInstance->GetMotion() == motion; });
    };
    const bool affected = azrtti_istypeof<EMotionFX::BlendSpace1DNode>(m_previewNode)
        ? usesMotion(static_cast<EMotionFX::BlendSpace1DNode::UniqueData*>(data)->m_motionInfos)
        : usesMotion(static_cast<EMotionFX::BlendSpace2DNode::UniqueData*>(data)->m_motionInfos);
    if (affected)
    {
        ResetPreview();
    }
}

void BlendSpaceNodeWidget::OnRemoveNode(EMotionFX::AnimGraph*, EMotionFX::AnimGraphNode* node)
{
    if (m_dragNode && (m_dragNode == node || node->RecursiveIsChildNode(m_dragNode)))
    {
        m_dragNode = nullptr;
    }
    if (m_previewSource && (m_previewSource == node || node->RecursiveIsChildNode(m_previewSource)))
    {
        ResetPreview();
    }
}

AZ::u32 BlendSpaceNodeWidget::GetMotionIndexForPoint(AZ::u32 pointIndex) const
{
    if (!m_dataNode)
    {
        return MCORE_INVALIDINDEX32;
    }
    // The unique data only holds the motions found in the motion set, in the node's order.
    const BlendSpaceMotions& motions = m_dataNode->GetMotions();
    AZ::u32 validIndex = 0;
    for (AZ::u32 i = 0; i < motions.size(); ++i)
    {
        if (motions[i].TestFlag(EMotionFX::BlendSpaceNode::BlendSpaceMotion::TypeFlags::InvalidMotion))
        {
            continue;
        }
        if (validIndex == pointIndex)
        {
            return i;
        }
        ++validIndex;
    }
    return MCORE_INVALIDINDEX32;
}

bool BlendSpaceNodeWidget::BeginMotionDrag(EMotionFX::BlendSpaceNode* node, AZ::u32 pointIndex)
{
    const AZ::u32 motionIndex = GetMotionIndexForPoint(pointIndex);
    if (!node || motionIndex >= node->GetMotions().size())
    {
        return false;
    }
    m_dragNode = node;
    m_dragMotionIndex = motionIndex;
    m_dragStartMotions = node->GetMotions();
    m_dragMoved = false;
    m_selectedPointIndex = pointIndex;
    return true;
}

void BlendSpaceNodeWidget::UpdateMotionDrag(const AZ::Vector2& coordinates)
{
    if (!m_dragNode)
    {
        return;
    }
    // Dragging overrides the evaluator, like typing a coordinate in the Attributes window.
    BlendSpaceMotions motions = m_dragStartMotions;
    EMotionFX::BlendSpaceNode::BlendSpaceMotion& motion = motions[m_dragMotionIndex];
    motion.SetXCoordinate(coordinates.GetX());
    motion.MarkXCoordinateSetByUser(true);
    if (azrtti_istypeof<EMotionFX::BlendSpace2DNode>(m_dragNode))
    {
        motion.SetYCoordinate(coordinates.GetY());
        motion.MarkYCoordinateSetByUser(true);
    }
    m_dragMoved = true;
    ApplyMotionsLive(m_dragNode, motions);
}

void BlendSpaceNodeWidget::EndMotionDrag()
{
    EMotionFX::BlendSpaceNode* node = m_dragNode;
    m_dragNode = nullptr;
    if (node && m_dragMoved)
    {
        const BlendSpaceMotions after = node->GetMotions();
        CommitMotions(node, m_dragStartMotions, after);
    }
    m_dragStartMotions.clear();
}

void BlendSpaceNodeWidget::CancelMotionDrag()
{
    EMotionFX::BlendSpaceNode* node = m_dragNode;
    m_dragNode = nullptr;
    if (node && m_dragMoved)
    {
        ApplyMotionsLive(node, m_dragStartMotions);
    }
    m_dragStartMotions.clear();
}

void BlendSpaceNodeWidget::ShowGridContextMenu(
    EMotionFX::BlendSpaceNode* node, const QPoint& globalPos, AZ::u32 pointIndex, const AZ::Vector2& coordinates)
{
    if (!node)
    {
        return;
    }
    const bool is2D = azrtti_istypeof<EMotionFX::BlendSpace2DNode>(node);
    const AZ::u32 motionIndex = pointIndex != MCORE_INVALIDINDEX32 ? GetMotionIndexForPoint(pointIndex) : MCORE_INVALIDINDEX32;

    QMenu menu(m_previewWidget);
    QAction* addAction = nullptr;
    QAction* removeAction = nullptr;
    if (motionIndex < node->GetMotions().size())
    {
        const AZStd::string& motionId = node->GetMotions()[motionIndex].GetMotionId();
        removeAction = menu.addAction(QString("Remove '%1'").arg(motionId.c_str()));
    }
    else
    {
        const QString position = is2D
            ? QString("(%1, %2)").arg(coordinates.GetX(), 0, 'g', 4).arg(coordinates.GetY(), 0, 'g', 4)
            : QString("(%1)").arg(coordinates.GetX(), 0, 'g', 4);
        addAction = menu.addAction(QString("Add motion at %1...").arg(position));
        addAction->setEnabled(m_dataMotionSet != nullptr);
    }

    QAction* chosen = menu.exec(globalPos);
    if (chosen && chosen == removeAction)
    {
        RemoveMotionAtPoint(node, pointIndex);
    }
    else if (chosen && chosen == addAction)
    {
        AddMotionAt(node, coordinates);
    }
}

void BlendSpaceNodeWidget::RemoveMotionAtPoint(EMotionFX::BlendSpaceNode* node, AZ::u32 pointIndex)
{
    const AZ::u32 motionIndex = GetMotionIndexForPoint(pointIndex);
    if (!node || motionIndex >= node->GetMotions().size())
    {
        return;
    }
    const BlendSpaceMotions before = node->GetMotions();
    BlendSpaceMotions after = before;
    after.erase(after.begin() + motionIndex);
    m_selectedPointIndex = MCORE_INVALIDINDEX32;
    CommitMotions(node, before, after);
}

void BlendSpaceNodeWidget::AddMotionAt(EMotionFX::BlendSpaceNode* node, const AZ::Vector2& coordinates)
{
    EMotionFX::MotionSet* motionSet = m_dataMotionSet;
    if (!motionSet)
    {
        return;
    }

    MotionSetSelectionWindow motionPickWindow(m_previewWidget);
    motionPickWindow.GetHierarchyWidget()->SetSelectionMode(true);
    motionPickWindow.Update(motionSet);
    motionPickWindow.setModal(true);
    if (motionPickWindow.exec() == QDialog::Rejected)
    {
        return;
    }
    const AZStd::vector<AZStd::string> selectedMotionIds = motionPickWindow.GetHierarchyWidget()->GetSelectedMotionIds(motionSet);
    if (selectedMotionIds.empty())
    {
        return;
    }

    const AZStd::string& motionId = selectedMotionIds.front();
    const BlendSpaceMotions before = node->GetMotions();
    const bool alreadyAdded = AZStd::any_of(before.begin(), before.end(),
        [&motionId](const EMotionFX::BlendSpaceNode::BlendSpaceMotion& motion) { return motion.GetMotionId() == motionId; });
    if (alreadyAdded)
    {
        QMessageBox::information(m_previewWidget, "Motion already in blend space",
            QString("'%1' is already in this blend space. Drag its point to move it.").arg(motionId.c_str()));
        return;
    }

    EMotionFX::BlendSpaceNode::BlendSpaceMotion motion(motionId);
    motion.SetXCoordinate(coordinates.GetX());
    motion.MarkXCoordinateSetByUser(true);
    if (azrtti_istypeof<EMotionFX::BlendSpace2DNode>(node))
    {
        motion.SetYCoordinate(coordinates.GetY());
        motion.MarkYCoordinateSetByUser(true);
    }
    BlendSpaceMotions after = before;
    after.push_back(motion);
    CommitMotions(node, before, after);
}

void BlendSpaceNodeWidget::ApplyMotionsLive(EMotionFX::BlendSpaceNode* node, const BlendSpaceMotions& motions)
{
    node->SetMotions(motions);

    // Rebuild this node's motion data in every running instance of the edited graph.
    EMotionFX::AnimGraph* animGraph = node->GetAnimGraph();
    for (size_t i = 0; animGraph && i < animGraph->GetNumAnimGraphInstances(); ++i)
    {
        if (EMotionFX::AnimGraphObjectData* data = animGraph->GetAnimGraphInstance(i)->GetUniqueObjectData(node->GetObjectIndex()))
        {
            data->Invalidate();
        }
    }

    if (m_previewNode && m_previewInstance && m_previewSource == node)
    {
        m_previewNode->SetMotions(motions);
        if (EMotionFX::AnimGraphObjectData* data = m_previewInstance->GetUniqueObjectData(m_previewNode->GetObjectIndex()))
        {
            data->Invalidate();
        }
    }
}

void BlendSpaceNodeWidget::CommitMotions(EMotionFX::BlendSpaceNode* node, const BlendSpaceMotions& before, const BlendSpaceMotions& after)
{
    // Serialize through the node so the command gets the member in its own format, then restore the old list for the command to record.
    node->SetMotions(after);
    const AZ::Outcome<AZStd::string> serializedMotions = MCore::ReflectionSerializer::SerializeMember(node, "motions");
    ApplyMotionsLive(node, before);
    if (!serializedMotions.IsSuccess())
    {
        AZ_Error("EMotionFX", false, "Blend space: could not serialize the motions of '%s'.", node->GetName());
        return;
    }

    const AZStd::string command = AZStd::string::format("AnimGraphAdjustNode -animGraphID %u -name \"%s\" -attributesString {-motions {%s}}",
        node->GetAnimGraph()->GetID(), node->GetName(), serializedMotions.GetValue().c_str());
    AZStd::string result;
    if (!CommandSystem::GetCommandManager()->ExecuteCommand(command, result))
    {
        AZ_Error("EMotionFX", false, "Blend space: could not update the motions of '%s': %s", node->GetName(), result.c_str());
    }
    const BlendSpaceMotions current = node->GetMotions();
    ApplyMotionsLive(node, current);
}


void BlendSpaceNodeWidget::RenderCurrentSamplePoint(QPainter& painter, const QPointF& samplePoint)
{
    RenderCircle(painter, samplePoint, s_currentSamplePointColor, s_currentSamplePointWidth);
}


void BlendSpaceNodeWidget::RenderSampledMotionPoint(QPainter& painter, const QPointF& point, float weight)
{
    const float alpha = AZ::Lerp(s_motionPointAlphaMin, s_motionPointAlphaMax, weight);
    const float size = AZ::Lerp(s_motionPointWidthMin, s_motionPointWidthMax, weight);
    
    QColor color = s_motionPointColor;
    color.setAlphaF(alpha);

    // Render the slightly transparent circle background.
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(point, size, size);

    // Render the fully opaque border
    painter.setPen(s_motionPointColor);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(point, size, size);
}


void BlendSpaceNodeWidget::RenderCircle(QPainter& painter, const QPointF& point, const QColor& color, float size)
{
    painter.setPen(color);
    painter.setBrush(color);

    painter.drawEllipse(point, size, size);
}


void BlendSpaceNodeWidget::RenderTextBox(QPainter& painter, const QPointF& point, const QString& text)
{
    QFont font;
    font.setPointSizeF(8.0f);

    painter.setFont(font);

    QFontMetrics fontMetrics(font);

    const int boxWidth = fontMetrics.horizontalAdvance(text);
    const int boxHeight = fontMetrics.height();

    const int halfBoxWidth = static_cast<int>(static_cast<float>(boxWidth) * 0.5f + 0.5f);
    const int halfBoxHeight = static_cast<int>(static_cast<float>(boxHeight) * 0.5f + 0.5f);

    QRect rect;
    rect.setTop(aznumeric_cast<int>(point.y() - halfBoxHeight));
    rect.setLeft(aznumeric_cast<int>(point.x() - halfBoxWidth));
    rect.setBottom(aznumeric_cast<int>(point.y() + halfBoxHeight));
    rect.setRight(aznumeric_cast<int>(point.x() + halfBoxWidth));
    
    painter.setPen(Qt::NoPen);
    QColor semiTransparentColor = s_currentSamplePointColor;
    semiTransparentColor.setAlphaF(0.25f);
    painter.setBrush(semiTransparentColor);
    painter.drawRect(rect);

    painter.setPen(s_currentSamplePointColor);
    semiTransparentColor.setAlphaF(0.5f);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect);

    painter.setPen(s_currentSamplePointColor);
    painter.setBrush(s_currentSamplePointColor);
    painter.drawText(rect, Qt::AlignCenter, text);
}

} // namespace EMStudio
