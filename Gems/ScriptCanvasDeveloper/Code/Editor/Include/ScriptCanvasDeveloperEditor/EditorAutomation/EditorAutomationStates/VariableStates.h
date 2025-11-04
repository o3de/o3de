/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QAbstractItemView>
#include <QLineEdit>
#include <QRect>
#include <QPoint>
#include <QString>
#include <QWidget>

#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>

#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Variable/VariableBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/VariableActions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationState that will create a variable and give it a name(assuming one is provided), using the specified creation method.
    */
    class CreateVariableState
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateVariableState, AZ::SystemAllocator)
        CreateVariableState(AutomationStateModelId dataTypeId, AutomationStateModelId nameId, bool errorOnNameMismatch = true, CreateVariableAction::CreationType creationType = CreateVariableAction::CreationType::AutoComplete, AutomationStateModelId outputId = "");
        ~CreateVariableState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:
        
        AutomationStateModelId m_dataTypeId;
        AutomationStateModelId m_nameId;
        AutomationStateModelId m_outputId;

        CreateVariableAction::CreationType m_creationType;

        bool m_errorOnNameMismatch = true;

        CreateVariableAction* m_createVariableAction = nullptr;
    };

    /**
        EditorAutomationState that will create a variable node by holding down a modifier and dragging off of the GraphVariablePalette
    */
    class CreateVariableNodeFromGraphPaletteState
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateVariableNodeFromGraphPaletteState, AZ::SystemAllocator)
        CreateVariableNodeFromGraphPaletteState(AutomationStateModelId variableNameId, AutomationStateModelId scenePoint, Qt::KeyboardModifier modifier, AutomationStateModelId outputId);
        ~CreateVariableNodeFromGraphPaletteState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:
        AutomationStateModelId m_variableNameId;
        AutomationStateModelId m_scenePoint;
        AutomationStateModelId m_outputId;

        Qt::KeyboardModifier   m_modifier;

        CreateVariableNodeFromGraphPalette* m_createVariable = nullptr;
    };
}
