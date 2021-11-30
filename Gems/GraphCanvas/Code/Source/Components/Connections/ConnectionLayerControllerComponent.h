/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/Components/LayerControllerComponent.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>

namespace GraphCanvas
{
    class ConnectionLayerControllerComponent
        : public LayerControllerComponent
        , public ConnectionNotificationBus::Handler
        , public LayerControllerNotificationBus::MultiHandler
    {
    public:
        static void Reflect(AZ::ReflectContext* context);
        AZ_COMPONENT(ConnectionLayerControllerComponent, "{9D71AFFE-539A-467B-8012-470100E0DA98}", LayerControllerComponent);

        ConnectionLayerControllerComponent();

        void Activate() override;

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId) override;
        ////

        // ConnecitonNotificationBus
        void OnMoveBegin() override;
        void OnMoveFinalized(bool isValidConnection) override;

        void OnSourceSlotIdChanged(const AZ::EntityId&, const AZ::EntityId&) override;
        void OnTargetSlotIdChanged(const AZ::EntityId&, const AZ::EntityId&) override;
        ////

        // LayerControllerNotificationBus
        void OnOffsetsChanged(int selectionOffset, int groupOffset) override;
        ////

    private:

        void UpdateEndpoints();
        
        LayerControllerRequests* m_sourceLayerController;
        LayerControllerRequests* m_targetLayerController;
    };
}
