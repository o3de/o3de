/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationModelIds.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/CreateElementsActions.h>

namespace ScriptCanvasDeveloper
{
    /**
        EditorAutomationState that will create the specified node from the NodePalette either at the specified point, or via splicing
    */
    class CreateNodeFromPaletteState
        : public NamedAutomationState
    {
    public:
        enum class CreationType
        {
            ScenePosition,
            Splice
        };

        CreateNodeFromPaletteState(GraphCanvas::NodePaletteWidget* paletteWidget, const QString& nodeName, CreationType creationType, AutomationStateModelId creationDataId, AutomationStateModelId outputId = "");
        ~CreateNodeFromPaletteState() override = default;

    protected:

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        GraphCanvas::NodePaletteWidget* m_nodePaletteWidget = nullptr;
        QString m_nodeName;

        CreationType           m_creationType;
        AutomationStateModelId m_creationDataId;

        AutomationStateModelId m_outputId;

        CreateNodeFromPaletteAction* m_createNodeAction = nullptr;
        DelayAction                  m_delayAction;
    };

    /**
        EditorAutomationState that will create all of the nodes under the specified category
    */
    class CreateCategoryFromNodePaletteState
        : public NamedAutomationState
    {
    public:

        CreateCategoryFromNodePaletteState(GraphCanvas::NodePaletteWidget* paletteWidget, AutomationStateModelId categoryId, AutomationStateModelId scenePoint, AutomationStateModelId outputId = "");
        ~CreateCategoryFromNodePaletteState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        AutomationStateModelId m_categoryId;
        AutomationStateModelId m_scenePoint;
        AutomationStateModelId m_outputId;

        GraphCanvas::NodePaletteWidget* m_paletteWidget = nullptr;

        CreateCategoryFromNodePaletteAction* m_creationAction = nullptr;
    };

    /**
        EditorAutomationState that will create the specified node at either the specified point using the context menu, or by
        splicing it onto a ConnectionId
    */
    class CreateNodeFromContextMenuState
        : public NamedAutomationState
    {
    public:
        enum class CreationType
        {
            ScenePosition,
            Splice
        };

        CreateNodeFromContextMenuState(const QString& nodeName, CreationType creationType, AutomationStateModelId creationDataId, AutomationStateModelId outputId = "");
        ~CreateNodeFromContextMenuState() override = default;

    protected:

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        QString m_nodeName;

        CreationType           m_creationType;
        AutomationStateModelId m_creationDataId;

        AutomationStateModelId m_outputId;

        CreateNodeFromContextMenuAction* m_createNodeAction = nullptr;
        DelayAction                  m_delayAction;
    };

    /**
        EditorAutomationState that will create the specified node at the specified point by dragging a connection from the specified endpoint.
        Will place the new node at a 'reasonable' distance from the specified endpoint, or at the specified point.
    */
    class CreateNodeFromProposalState
        : public NamedAutomationState
    {
    public:
        CreateNodeFromProposalState(const QString& nodeName, AutomationStateModelId endpointId, AutomationStateModelId scenePointId = "", AutomationStateModelId nodeOutputId = "", AutomationStateModelId connectionOutputId = "");
        ~CreateNodeFromProposalState() override = default;

    protected:

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        QString m_nodeName;

        AutomationStateModelId m_endpointId;

        AutomationStateModelId m_scenePointId;
        AutomationStateModelId m_nodeOutputId;
        AutomationStateModelId m_connectionOutputId;

        CreateNodeFromProposalAction*   m_createNodeAction = nullptr;
        DelayAction                     m_delayAction;
    }; 

    /**
        EditorAutomationState that will create a Group using the specified creation method.
    */
    class CreateGroupState 
        : public NamedAutomationState
    {
    public:
        CreateGroupState(GraphCanvas::EditorId editorId, CreateGroupAction::CreationType creationType = CreateGroupAction::CreationType::Hotkey, AutomationStateModelId outputId = "");
        ~CreateGroupState() override = default;

    protected:

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        GraphCanvas::EditorId m_editorId;
        CreateGroupAction::CreationType m_creationType;

        AutomationStateModelId m_outputId;

        CreateGroupAction* m_createGroupAction;
        DelayAction        m_delayAction;
    };
}
