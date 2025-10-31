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

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ConnectionActions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationState that will couple the two specified nodes together. The ordering of the coupling is decided by the specified connection type from the perspective of the 'nodeToPickUp'
    */
    class CoupleNodesState
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(CoupleNodesState, AZ::SystemAllocator)
        CoupleNodesState(AutomationStateModelId firstNode, GraphCanvas::ConnectionType connectionType, AutomationStateModelId secondNode, AutomationStateModelId outputId = "");
        ~CoupleNodesState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        AutomationStateModelId m_pickUpNode;
        AutomationStateModelId m_targetNode;

        GraphCanvas::ConnectionType m_connectionType;

        CoupleNodesAction* m_coupleNodesAction = nullptr;

        AutomationStateModelId m_outputId;
    };

    /**
        EditorAutomationState that will attempt to create a connection between the two specified endpoints
    */
    class ConnectEndpointsState
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(ConnectEndpointsState, AZ::SystemAllocator)
        ConnectEndpointsState(AutomationStateModelId sourceEndpoint, AutomationStateModelId targetEndpoint, AutomationStateModelId outputId = "");
        ~ConnectEndpointsState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        AutomationStateModelId m_sourceEndpoint;
        AutomationStateModelId m_targetEndpoint;

        ConnectEndpointsAction* m_connectEndpointsAction = nullptr;

        AutomationStateModelId m_outputId;
    };
}
