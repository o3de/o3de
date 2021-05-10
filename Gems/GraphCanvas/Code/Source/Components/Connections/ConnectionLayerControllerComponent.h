/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        void OnOffsetsChanged(int selectionOffset, int groupOffset);
        ////

    private:

        void UpdateEndpoints();
        
        LayerControllerRequests* m_sourceLayerController;
        LayerControllerRequests* m_targetLayerController;
    };
}
