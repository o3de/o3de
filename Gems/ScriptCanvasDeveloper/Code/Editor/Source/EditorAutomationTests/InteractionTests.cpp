/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qpushbutton.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>

#include <EditorAutomationTests/InteractionTests.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/EditorViewStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/ElementInteractionStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/GraphStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/UtilityStates.h>

namespace ScriptCanvasDeveloper
{
    ///////////////////////
    // AltClickDeleteTest
    ///////////////////////

    AltClickDeleteTest::AltClickDeleteTest()
        : EditorAutomationTest("Alt Click Deletion Test")
    {
        AddState(new CreateRuntimeGraphState());

        AutomationStateModelId onGraphStartTargetPointId = "OnGraphStartScenePoint";
        AutomationStateModelId onGraphStartId = "OnGraphStartId";

        AddState(new FindViewCenterState(onGraphStartTargetPointId));
        AddState(new CreateNodeFromContextMenuState("On Graph Start", CreateNodeFromContextMenuState::CreationType::ScenePosition, onGraphStartTargetPointId, onGraphStartId));

        AutomationStateModelId onGraphStartEndpoint = "OnGraphStart::ExecutionEndpoint";

        AddState(new FindEndpointOfTypeState(onGraphStartId, onGraphStartEndpoint, GraphCanvas::CT_Output, GraphCanvas::SlotTypes::ExecutionSlot));

        AutomationStateModelId buildStringNodeId = "BuildStringId";
        AddState(new CreateNodeFromProposalState("Build String", onGraphStartEndpoint, "", buildStringNodeId));

        AutomationStateModelId buildStringEndpoint = "BuildString::ExecutionEndpoint";

        AddState(new FindEndpointOfTypeState(buildStringNodeId, buildStringEndpoint, GraphCanvas::CT_Output, GraphCanvas::SlotTypes::ExecutionSlot));

        AutomationStateModelId printNodeId = "PrintNodeId";
        AddState(new CreateNodeFromProposalState("Print", buildStringEndpoint, "", printNodeId));

        AddState(new AltClickSceneElementState(buildStringNodeId));

        AutomationStateModelId altClickConnectionTarget = "ConnectionTarget";
        AddState(new GetLastConnection(onGraphStartEndpoint, altClickConnectionTarget));

        AddState(new AltClickSceneElementState(altClickConnectionTarget));

        AddState(new ForceCloseActiveGraphState());
    }

    //////////////////////////////
    // CutCopyPasteDuplicateTest
    //////////////////////////////

    CutCopyPasteDuplicateTest::CheckpointState::CheckpointState(AZStd::string checkPoint)
        : CustomActionState(checkPoint.c_str())
    {
    }

    CutCopyPasteDuplicateTest::CutCopyPasteDuplicateTest(QString nodeName)
        : EditorAutomationTest(QString("Cut/Copy/Paste/Duplicate %1 Test").arg(nodeName))
        , m_originalNodeId("OriginalNodeId")
    {
        AddState(new CreateRuntimeGraphState());

        AutomationStateModelId viewCenter = "ViewCenter";
        AddState(new FindViewCenterState(viewCenter));

        m_createNodeState = new CreateNodeFromContextMenuState(nodeName, CreateNodeFromContextMenuState::CreationType::ScenePosition, viewCenter, m_originalNodeId);
        AddState(m_createNodeState);

        AddState(new SelectSceneElementState(m_originalNodeId));

        AddState(new TriggerHotKey(QChar('x'), "CutOriginal"));
        AddState(new TriggerHotKey(QChar('v'), "PasteOriginal"));

        m_cutPasteCheckpoint = new CheckpointState("Confirm Cut/Paste");
        AddState(m_cutPasteCheckpoint);

        AddState(new TriggerHotKey(QChar('c'), "CopyOriginal"));
        AddState(new TriggerHotKey(QChar('v'), "PasteCopied"));

        m_copyPasteCheckpoint = new CheckpointState("Confirm Copy/Paste");
        AddState(m_copyPasteCheckpoint);

        AddState(new TriggerHotKey(QChar('c'), "CopyCopied"));
        AddState(new TriggerHotKey(QChar('v'), "PasteCopiedCopied"));

        m_copyPasteCopyCheckpoint = new CheckpointState("Confirm Copy/Paste for Copy");
        AddState(m_copyPasteCopyCheckpoint);

        AddState(new TriggerHotKey(QChar('d'), "Duplicate"));

        m_duplicateCheckpoint = new CheckpointState("Confirm Duplication");
        AddState(m_duplicateCheckpoint);

        AddState(new ForceCloseActiveGraphState());
    }

    void CutCopyPasteDuplicateTest::OnNodeAdded(const AZ::EntityId& nodeId, bool)
    {
        m_createdSet.insert(nodeId);
    }

    void CutCopyPasteDuplicateTest::OnNodeRemoved(const AZ::EntityId& nodeId)
    {
        if (nodeId == m_removalTarget)
        {
            m_removalTarget.SetInvalid();
        }
    }

    void CutCopyPasteDuplicateTest::OnStateComplete(int stateId)
    {
        if (stateId == CreateRuntimeGraphStateId::StateID())
        {
            const GraphCanvas::GraphId* graphId = GetStateDataAs<GraphCanvas::GraphId>(StateModelIds::GraphCanvasId);

            if (graphId)
            {
                GraphCanvas::SceneNotificationBus::Handler::BusConnect((*graphId));
            }
        }
        else if (stateId == m_createNodeState->GetStateId())
        {
            const GraphCanvas::NodeId* nodeId = GetStateDataAs<GraphCanvas::GraphId>(m_originalNodeId);

            if (nodeId)
            {
                m_removalTarget = (*nodeId);
                m_createdSet.clear();
            }
        }
        else if (stateId == m_cutPasteCheckpoint->GetStateId())
        {
            if (m_removalTarget.IsValid())
            {
                AddError("Cut failed to remove original element from the scene.");
            }
            else if (m_createdSet.empty())
            {
                AddError("Paste failed to add element to the scene.");
            }
            else
            {
                ProcessCreationSet();
            }
        }
        else if (stateId == m_copyPasteCheckpoint->GetStateId()
            || stateId == m_copyPasteCopyCheckpoint->GetStateId())
        {
            if (m_createdSet.empty())
            {
                AddError("Paste failed to add element to the scene.");
            }
            else
            {
                ProcessCreationSet();
            }
        }
        else if (stateId == m_duplicateCheckpoint->GetStateId())
        {
            if (m_createdSet.empty())
            {
                AddError("Duplicate failed to add element to the scene.");
            }
            else
            {
                ProcessCreationSet();
            }
        }
        else if (stateId == ForceCloseActiveGraphStateId::StateID())
        {
            GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
        }
    }

    void CutCopyPasteDuplicateTest::ProcessCreationSet()
    {
        AZ::EntityId testId;

        for (GraphCanvas::NodeId nodeId : m_createdSet)
        {
            if (GraphCanvas::GraphUtils::IsNodeWrapped(nodeId))
            {
                continue;
            }

            testId = nodeId;
            break;
        }

        m_createdSet.clear();

        bool isSelected = false;
        GraphCanvas::SceneMemberUIRequestBus::EventResult(isSelected, testId, &GraphCanvas::SceneMemberUIRequests::IsSelected);

        if (!isSelected)
        {
            AddError("Pasted node is not selected by default.");
        }
    }

    /*
    bool CutCopyPasteDuplicateTest::TransitionToState(int state)
    {
        if (state == CutOriginalNode)
        {
            delete m_selectElement;
            m_selectElement = aznew SelectSceneElementAction(m_target);

            m_removalTarget = m_target;

            m_actionRunner.AddAction(m_selectElement);
            m_actionRunner.AddAction(&m_pressCtrlAction);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_typeX);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_releaseCtrlAction);
            m_actionRunner.AddAction(&m_processEvents);
        }
        else if (state == PasteOriginalNode)
        {
            m_actionRunner.AddAction(&m_pressCtrlAction);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_typeV);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_releaseCtrlAction);
            m_actionRunner.AddAction(&m_processEvents);
        }
        else if (state == CopyPasteOriginalNodeState)
        {
            m_actionRunner.AddAction(&m_pressCtrlAction);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_typeC);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_typeV);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_releaseCtrlAction);
            m_actionRunner.AddAction(&m_processEvents);
        }
        else if (state == CopyPasteDuplicatedNodeState)
        {
            m_actionRunner.AddAction(&m_pressCtrlAction);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_typeC);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_typeV);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_releaseCtrlAction);
            m_actionRunner.AddAction(&m_processEvents);
        }
        else if (state == DuplicateNodeState)
        {
            m_actionRunner.AddAction(&m_pressCtrlAction);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_typeD);
            m_actionRunner.AddAction(&m_processEvents);
            m_actionRunner.AddAction(&m_releaseCtrlAction);
            m_actionRunner.AddAction(&m_processEvents);
        }
        else if (state == ForceCloseGraphState)
        {
            m_actionRunner.AddAction(&m_forceCloseGraphAction);
        }

        return m_actionRunner.HasActions();
    }

    void CutCopyPasteDuplicateTest::OnTestComplete()
    {
        delete m_createFromContextMenu;
        m_createFromContextMenu = nullptr;

        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
    }

    void CutCopyPasteDuplicateTest::OnStateComplete(int state)
    {
        if (state == CreateGraphState)
        {
            m_graphId = m_createNewGraphAction.GetGraphId();
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphId);
        }
        else if (state == CreateNodeState)
        {
            m_target = m_createFromContextMenu->GetCreatedNodeId();
            m_createdSet.clear();
        }
        else if (state == CutOriginalNode)
        {
            if (m_removalTarget.IsValid())
            {
                AddError(QString("Failed to cut %1").arg(m_nodeName).toUtf8().data());
            }
        }
        else if (state == PasteOriginalNode)
        {
            if (m_createdSet.empty())
            {
                AddError(QString("Failed to cut and paste original %1").arg(m_nodeName).toUtf8().data());
            }
            else
            {
                for (GraphCanvas::NodeId nodeId : m_createdSet)
                {
                    if (GraphCanvas::GraphUtils::IsNodeWrapped(nodeId))
                    {
                        continue;
                    }

                    m_target = nodeId;
                    break;
                }

                m_createdSet.clear();

                bool isSelected = false;
                GraphCanvas::SceneMemberUIRequestBus::EventResult(isSelected, m_target, &GraphCanvas::SceneMemberUIRequests::IsSelected);

                if (!isSelected)
                {
                    AddError("Pasted node is not selected by default.");
                }
            }
        }
        else if (state == CopyPasteOriginalNodeState)
        {
            if (m_createdSet.empty())
            {
                AddError(QString("Failed to copy and paste original %1").arg(m_nodeName).toUtf8().data());
            }
            else
            {
                for (GraphCanvas::NodeId nodeId : m_createdSet)
                {
                    if (GraphCanvas::GraphUtils::IsNodeWrapped(nodeId))
                    {
                        continue;
                    }

                    m_target = nodeId;
                    break;
                }

                m_createdSet.clear();

                bool isSelected = false;
                GraphCanvas::SceneMemberUIRequestBus::EventResult(isSelected, m_target, &GraphCanvas::SceneMemberUIRequests::IsSelected);

                if (!isSelected)
                {
                    AddError("Pasted node is not selected by default.");
                }
            }
        }
        else if (state == CopyPasteDuplicatedNodeState)
        {
            if (m_createdSet.empty())
            {
                AddError(QString("Failed to copy and paste duplicated %1").arg(m_nodeName).toUtf8().data());
            }
            else
            {
                for (GraphCanvas::NodeId nodeId : m_createdSet)
                {
                    if (GraphCanvas::GraphUtils::IsNodeWrapped(nodeId))
                    {
                        continue;
                    }

                    m_target = nodeId;
                    break;
                }

                m_createdSet.clear();

                bool isSelected = false;
                GraphCanvas::SceneMemberUIRequestBus::EventResult(isSelected, m_target, &GraphCanvas::SceneMemberUIRequests::IsSelected);

                if (!isSelected)
                {
                    AddError("Pasted node is not selected by default.");
                }
            }
        }
        else if (state == DuplicateNodeState)
        {
            if (m_createdSet.empty())
            {
                AddError(QString("Failed to duplicate %1").arg(m_nodeName).toUtf8().data());
            }
            else
            {
                m_createdSet.clear();
            }
        }
    }

    bool CutCopyPasteDuplicateTest::CleanupAfterErrorState()
    {
        if (m_createNewGraphAction.GetGraphId().IsValid())
        {
            m_actionRunner.AddAction(&m_forceCloseGraphAction);
        }

        return m_actionRunner.HasActions();
    }
    */
}
