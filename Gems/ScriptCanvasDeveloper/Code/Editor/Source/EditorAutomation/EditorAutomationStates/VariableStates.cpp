/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QApplication>
#include <QPushButton>
#include <QTableView>
#include <QToolButton>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <GraphCanvas/Widgets/NodePalette/Model/NodePaletteSortFilterProxyModel.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteTreeView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/VariableStates.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/WidgetActions.h>

namespace ScriptCanvasDeveloper
{
    ////////////////////////
    // CreateVariableState
    ////////////////////////

    CreateVariableState::CreateVariableState(AutomationStateModelId dataTypeId, AutomationStateModelId nameId, bool errorOnNameMisMatch, CreateVariableAction::CreationType creationType, AutomationStateModelId outputId)
        : NamedAutomationState("CreateVariableState")
        , m_dataTypeId(dataTypeId)
        , m_nameId(nameId)
        , m_outputId(outputId)
        , m_creationType(creationType)
        , m_errorOnNameMismatch(errorOnNameMisMatch)
    {
        SetStateName(AZStd::string::format("CreateVariableState::%s", dataTypeId.c_str()));
    }

    void CreateVariableState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const ScriptCanvas::Data::Type* dataType = GetStateModel()->GetStateDataAs<ScriptCanvas::Data::Type>(m_dataTypeId);

        if (dataType)
        {
            if (!m_nameId.empty())
            {
                const AZStd::string* variableName = GetStateModel()->GetStateDataAs<AZStd::string>(m_nameId);

                if (variableName)
                {
                    QString name = QString(variableName->c_str());
                    m_createVariableAction = aznew CreateVariableAction((*dataType), name, m_creationType);
                }
                else
                {
                    ReportError(AZStd::string::format("%s is not a string value", m_nameId.c_str()));
                }
            }
            else
            {
                m_createVariableAction = aznew CreateVariableAction((*dataType), m_creationType);
            }
        }
        else
        {
            ReportError(AZStd::string::format("%s is not a valid ScriptCanvas::Data::DataType", m_dataTypeId.c_str()));
        }

        if (m_createVariableAction)
        {
            m_createVariableAction->SetErrorOnNameMisMatch(m_errorOnNameMismatch);
            actionRunner.AddAction(m_createVariableAction);
        }
    }

    void CreateVariableState::OnStateActionsComplete()
    {
        if (m_createVariableAction)
        {
            if (!m_outputId.empty())
            {
                ScriptCanvas::VariableId variableId = m_createVariableAction->GetVariableId();
                GetStateModel()->SetStateData(m_outputId, variableId);
            }

            delete m_createVariableAction;
            m_createVariableAction = nullptr;
        }
    }

    ////////////////////////////////////////////
    // CreateVariableNodeFromGraphPaletteState
    ////////////////////////////////////////////

    CreateVariableNodeFromGraphPaletteState::CreateVariableNodeFromGraphPaletteState(AutomationStateModelId variableNameId, AutomationStateModelId scenePoint, Qt::KeyboardModifier modifier, AutomationStateModelId outputId)
        : NamedAutomationState("CreateVariableNodeFromGraphPaletteState")
        , m_variableNameId(variableNameId)
        , m_scenePoint(scenePoint)
        , m_outputId(outputId)
        , m_modifier(modifier)
    {        
        AZStd::string keyModifier;

        if (modifier == Qt::KeyboardModifier::AltModifier)
        {
            keyModifier = "Alt";
        }
        else if (modifier == Qt::KeyboardModifier::ShiftModifier)
        {
            keyModifier = "Shift";
        }
        else
        {
            keyModifier = "???";
        }

        SetStateName(AZStd::string::format("CreateVariableNodeFromGraphPaletteState::%s::%s", m_variableNameId.c_str(), keyModifier.c_str()));
    }

    void CreateVariableNodeFromGraphPaletteState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const GraphCanvas::GraphId* graphId = GetStateModel()->GetStateDataAs<GraphCanvas::GraphId>(StateModelIds::GraphCanvasId);
        const AZStd::string* variableName = GetStateModel()->GetStateDataAs<AZStd::string>(m_variableNameId);
        const AZ::Vector2*   scenePoint = GetStateModel()->GetStateDataAs<AZ::Vector2>(m_scenePoint);

        if (graphId && variableName && scenePoint)
        {
            m_createVariable = aznew CreateVariableNodeFromGraphPalette((*variableName), (*graphId), GraphCanvas::ConversionUtils::AZToQPoint((*scenePoint)).toPoint(), m_modifier);
            actionRunner.AddAction(m_createVariable);
        }
        else
        {
            if (graphId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a GraphCanvas::GraphId", StateModelIds::GraphCanvasId));
            }

            if (variableName == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid string", m_variableNameId.c_str()));
            }

            if (scenePoint == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid Vector2", m_scenePoint.c_str()));
            }
        }
    }

    void CreateVariableNodeFromGraphPaletteState::OnStateActionsComplete()
    {
        if (m_createVariable)
        {
            if (!m_outputId.empty())
            {
                AZ::EntityId nodeId = m_createVariable->GetCreatedNodeId();
                GetStateModel()->SetStateData(m_outputId, nodeId);
            }

            delete m_createVariable;
            m_createVariable = nullptr;
        }
    }
}
