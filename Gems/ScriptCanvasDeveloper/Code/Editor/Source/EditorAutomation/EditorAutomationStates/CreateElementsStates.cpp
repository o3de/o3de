/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QToolButton>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/Automation/AutomationIds.h>
#include <GraphCanvas/Editor/Automation/AutomationUtils.h>

#include <GraphCanvas/Widgets/NodePalette/Model/NodePaletteSortFilterProxyModel.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteTreeView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/CreateElementsStates.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/CreateElementsActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/WidgetActions.h>

namespace ScriptCanvasDeveloper
{
    ///////////////////////////////
    // CreateNodeFromPaletteState
    ///////////////////////////////

    CreateNodeFromPaletteState::CreateNodeFromPaletteState(GraphCanvas::NodePaletteWidget* paletteWidget, const QString& nodeName, CreationType creationType, AutomationStateModelId creationDataId, AutomationStateModelId outputId)
        : NamedAutomationState("CreateNodeFromPaletteState")
        , m_nodePaletteWidget(paletteWidget)
        , m_nodeName(nodeName)
        , m_creationType(creationType)
        , m_creationDataId(creationDataId)
        , m_outputId(outputId)
        , m_delayAction(AZStd::chrono::milliseconds(500))
    {
        AZStd::string nameString = AZStd::string::format("CreateNodeFromPaletteState::%s", nodeName.toUtf8().data());

        SetStateName(nameString);
    }

    void CreateNodeFromPaletteState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const GraphCanvas::GraphId* graphId = GetStateModel()->GetStateDataAs<GraphCanvas::GraphId>(StateModelIds::GraphCanvasId);

        if (graphId != nullptr && m_nodePaletteWidget != nullptr)
        {
            switch (m_creationType)
            {
            case CreationType::ScenePosition:
            {
                const AZ::Vector2* dropPoint = GetStateModel()->GetStateDataAs<AZ::Vector2>(m_creationDataId);

                if (dropPoint)
                {
                    QPointF qPoint = QPoint(static_cast<int>(dropPoint->GetX()), static_cast<int>(dropPoint->GetY()));
                    m_createNodeAction = aznew CreateNodeFromPaletteAction(m_nodePaletteWidget, (*graphId), m_nodeName, qPoint);
                }
                break;
            }
            case CreationType::Splice:
            {
                const GraphCanvas::ConnectionId* connectionId = GetStateModel()->GetStateDataAs<GraphCanvas::ConnectionId>(m_creationDataId);

                if (connectionId)
                {
                    m_createNodeAction = aznew CreateNodeFromPaletteAction(m_nodePaletteWidget, (*graphId), m_nodeName, (*connectionId));
                }
                break;
            }
            default:
                break;
            }

            if (m_createNodeAction)
            {
                actionRunner.AddAction(m_createNodeAction);
                actionRunner.AddAction(&m_delayAction);
            }
            else
            {
                ReportError("Unknown creation type provided");
            }
        }
        else
        {
            if (graphId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid GraphCanvas::GraphId", StateModelIds::GraphCanvasId));
            }

            if (m_nodePaletteWidget == nullptr)
            {
                ReportError(AZStd::string::format("NodePaletteWidget not provided"));
            }
        }
    }

    void CreateNodeFromPaletteState::OnStateActionsComplete()
    {
        if (m_createNodeAction)
        {
            if (!m_outputId.empty())
            {
                GraphCanvas::NodeId nodeId = m_createNodeAction->GetCreatedNodeId();
                GetStateModel()->SetStateData(m_outputId, nodeId);
            }

            delete m_createNodeAction;
            m_createNodeAction = nullptr;
        }
    }

    ///////////////////////////////////////
    // CreateCategoryFromNodePaletteState
    ///////////////////////////////////////

    CreateCategoryFromNodePaletteState::CreateCategoryFromNodePaletteState(GraphCanvas::NodePaletteWidget* paletteWidget, AutomationStateModelId categoryId, AutomationStateModelId scenePoint, AutomationStateModelId outputId)
        : NamedAutomationState("CreateCategoryFromNodePaletteState")
        , m_paletteWidget(paletteWidget)
        , m_categoryId(categoryId)
        , m_scenePoint(scenePoint)
        , m_outputId(outputId)
    {
        SetStateName(AZStd::string::format("CreateCategoryFromNodePaletteState::%s", m_categoryId.c_str()));
    }

    void CreateCategoryFromNodePaletteState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const GraphCanvas::GraphId* graphId = GetStateModel()->GetStateDataAs<GraphCanvas::GraphId>(StateModelIds::GraphCanvasId);
        const AZ::Vector2* scenePoint = GetStateModel()->GetStateDataAs<AZ::Vector2>(m_scenePoint);
        const AZStd::string* category = GetStateModel()->GetStateDataAs<AZStd::string>(m_categoryId);

        if (graphId && scenePoint && category)
        {
            m_creationAction = aznew CreateCategoryFromNodePaletteAction(m_paletteWidget, (*graphId), category->c_str(), GraphCanvas::ConversionUtils::AZToQPoint((*scenePoint)));
            actionRunner.AddAction(m_creationAction);
        }
        else
        {
            if (graphId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid GraphCanvas::GraphId", StateModelIds::GraphCanvasId));
            }

            if (scenePoint == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid Vector2", m_scenePoint.c_str()));
            }

            if (category == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid string", m_categoryId.c_str()));
            }
        }
    }

    void CreateCategoryFromNodePaletteState::OnStateActionsComplete()
    {
        if (m_creationAction)
        {
            if (!m_outputId.empty())
            {
                auto createdNodeIds = m_creationAction->GetCreatedNodes();
                GetStateModel()->SetStateData(m_outputId, createdNodeIds);
            }

            delete m_creationAction;
            m_creationAction = nullptr;
        }
    }

    ///////////////////////////////////
    // CreateNodeFromContextMenuState
    ///////////////////////////////////

    CreateNodeFromContextMenuState::CreateNodeFromContextMenuState(const QString& nodeName, CreationType creationType, AutomationStateModelId creationDataId, AutomationStateModelId outputId)
        : NamedAutomationState("CreateNodeFromContextMenuState")
        , m_nodeName(nodeName)
        , m_creationType(creationType)
        , m_creationDataId(creationDataId)
        , m_outputId(outputId)
        , m_delayAction(AZStd::chrono::milliseconds(500))
    {
        AZStd::string nameString = AZStd::string::format("CreateNodeFromContextMenuState::%s", nodeName.toUtf8().data());

        SetStateName(nameString);
    }

    void CreateNodeFromContextMenuState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const GraphCanvas::GraphId* graphId = GetStateModel()->GetStateDataAs<GraphCanvas::GraphId>(StateModelIds::GraphCanvasId);

        if (graphId != nullptr)
        {
            switch (m_creationType)
            {
            case CreationType::ScenePosition:
            {
                const AZ::Vector2* dropPoint = GetStateModel()->GetStateDataAs<AZ::Vector2>(m_creationDataId);

                if (dropPoint)
                {
                    QPointF qPoint = QPoint(static_cast<int>(dropPoint->GetX()), static_cast<int>(dropPoint->GetY()));
                    m_createNodeAction = aznew CreateNodeFromContextMenuAction((*graphId), m_nodeName, qPoint);
                }
                break;
            }
            case CreationType::Splice:
            {
                const GraphCanvas::ConnectionId* connectionId = GetStateModel()->GetStateDataAs<GraphCanvas::ConnectionId>(m_creationDataId);

                if (connectionId)
                {
                    m_createNodeAction = aznew CreateNodeFromContextMenuAction((*graphId), m_nodeName, (*connectionId));
                }
                break;
            }
            default:
                break;
            }
        }

        if (m_createNodeAction)
        {
            actionRunner.AddAction(m_createNodeAction);
            actionRunner.AddAction(&m_delayAction);
        }
        else
        {
            ReportError(AZStd::string::format("Failed to configure CreateNodeFromContextMenuState::%s", m_nodeName.toUtf8().data()));
        }
    }

    void CreateNodeFromContextMenuState::OnStateActionsComplete()
    {
        if (m_createNodeAction)
        {
            if (!m_outputId.empty())
            {
                GraphCanvas::NodeId nodeId = m_createNodeAction->GetCreatedNodeId();
                GetStateModel()->SetStateData(m_outputId, nodeId);
            }

            delete m_createNodeAction;
            m_createNodeAction = nullptr;
        }
    }

    ////////////////////////////////
    // CreateNodeFromProposalState
    ////////////////////////////////

    CreateNodeFromProposalState::CreateNodeFromProposalState(const QString& nodeName, AutomationStateModelId endpointId, AutomationStateModelId scenePointId, AutomationStateModelId nodeOutputId, AutomationStateModelId connectionOutputId)
        : NamedAutomationState("CreateNodeFromProposalState")
        , m_nodeName(nodeName)
        , m_endpointId(endpointId)
        , m_scenePointId(scenePointId)
        , m_nodeOutputId(nodeOutputId)
        , m_connectionOutputId(connectionOutputId)
        , m_delayAction(AZStd::chrono::milliseconds(500))
    {
        AZStd::string stateId = AZStd::string::format("CreateNodeFromProposalState::%s", nodeName.toUtf8().data());
        SetStateName(stateId);
    }

    void CreateNodeFromProposalState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const GraphCanvas::GraphId* graphId = GetStateModel()->GetStateDataAs<GraphCanvas::GraphId>(StateModelIds::GraphCanvasId);
        const GraphCanvas::Endpoint* endpoint = GetStateModel()->GetStateDataAs<GraphCanvas::Endpoint>(m_endpointId);

        if (graphId && endpoint)
        {
            if (!m_scenePointId.empty())
            {
                const AZ::Vector2* scenePoint = GetStateModel()->GetStateDataAs<AZ::Vector2>(m_scenePointId);

                if (scenePoint)
                {
                    m_createNodeAction = aznew CreateNodeFromProposalAction((*graphId), (*endpoint), m_nodeName, GraphCanvas::ConversionUtils::AZToQPoint((*scenePoint)));
                }
                else
                {
                    ReportError(AZStd::string::format("%s is an invalid Vector2", m_scenePointId.c_str()));
                }
            }
            else
            {
                m_createNodeAction = aznew CreateNodeFromProposalAction((*graphId), (*endpoint), m_nodeName);
            }
        }
        else
        {
            if (graphId == nullptr)
            {
                ReportError(AZStd::string::format("%s is an invalid GraphCanvas::GraphId", StateModelIds::GraphCanvasId));
            }

            if (endpoint == nullptr)
            {
                ReportError(AZStd::string::format("%s is an invalid GraphCanvas::Endpoint", m_endpointId.c_str()));
            }
        }

        if (m_createNodeAction)
        {
            actionRunner.AddAction(m_createNodeAction);
            actionRunner.AddAction(&m_delayAction);
        }
    }

    void CreateNodeFromProposalState::OnStateActionsComplete()
    {
        if (m_createNodeAction)
        {
            if (!m_nodeOutputId.empty())
            {
                GraphCanvas::NodeId nodeId = m_createNodeAction->GetCreatedNodeId();
                GetStateModel()->SetStateData(m_nodeOutputId, nodeId);
            }

            if (!m_connectionOutputId.empty())
            {
                GraphCanvas::ConnectionId connectionId = m_createNodeAction->GetConnectionId();
                GetStateModel()->SetStateData(m_connectionOutputId, connectionId);
            }

            delete m_createNodeAction;
            m_createNodeAction = nullptr;
        }
    }

    /////////////////////
    // CreateGroupState
    /////////////////////

    CreateGroupState::CreateGroupState(GraphCanvas::EditorId editorId, CreateGroupAction::CreationType creationType, AutomationStateModelId outputId)
        : NamedAutomationState("CreateGroupState")
        , m_editorId(editorId)
        , m_creationType(creationType)
        , m_outputId(outputId)
        , m_delayAction(AZStd::chrono::milliseconds(500))
    {
        AZStd::string stateName = "CreateGroupState::%s";

        if (!m_outputId.empty())
        {
            stateName.append("::");
            stateName.append(m_outputId);
        }

        switch (m_creationType)
        {
        case CreateGroupAction::CreationType::Hotkey:
            stateName = AZStd::string::format(stateName.c_str(), "HotKey");
            break;
        case CreateGroupAction::CreationType::Toolbar:
            stateName = AZStd::string::format(stateName.c_str(), "Toolbar");
            break;
        }

        SetStateName(stateName);
    }

    void CreateGroupState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const GraphCanvas::GraphId* graphId = GetStateModel()->GetStateDataAs<GraphCanvas::GraphId>(StateModelIds::GraphCanvasId);

        m_createGroupAction = aznew CreateGroupAction(m_editorId, (*graphId), m_creationType);

        actionRunner.AddAction(m_createGroupAction);
        actionRunner.AddAction(&m_delayAction);
    }

    void CreateGroupState::OnStateActionsComplete()
    {
        if (!m_outputId.empty())
        {
            AZ::EntityId groupId = m_createGroupAction->GetCreatedGroupId();
            GetStateModel()->SetStateData(m_outputId, groupId);
        }

        delete m_createGroupAction;
        m_createGroupAction = nullptr;
    }
}
