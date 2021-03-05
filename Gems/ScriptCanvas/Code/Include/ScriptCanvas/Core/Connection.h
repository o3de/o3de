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

#include <AzCore/Component/Component.h>
#include <AzCore/Outcome/Outcome.h>

#include "Core.h"
#include "ConnectionBus.h"
#include "GraphBus.h"

namespace ScriptCanvas
{
    class Slot;

    class Connection
        : public AZ::Component
        , protected ConnectionRequestBus::Handler
        , protected GraphNotificationBus::Handler
    {
    public:

        AZ_COMPONENT(Connection, "{64CA5016-E803-4AC4-9A36-BDA2C890C6EB}", AZ::Component);

        Connection() = default;

        Connection(const ID& fromNode, const SlotId& fromSlot, const ID& toNode, const SlotId& toSlot);
        Connection(const Endpoint& fromConnection, const Endpoint& toConnection);

        virtual ~Connection();

        static void Reflect(AZ::ReflectContext* reflection);

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static AZ::Outcome<void, AZStd::string> ValidateEndpoints(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint);
        static AZ::Outcome<void, AZStd::string> ValidateConnection(const Slot& sourceSlot, const Slot& targetSlot);

        bool ContainsEndpoint(const Endpoint& endpoint);

        // ConnectionRequestBus
        const SlotId& GetSourceSlot() const override;
        const SlotId& GetTargetSlot() const override;

        const ID& GetTargetNode() const override;
        const ID& GetSourceNode() const override;

        const Endpoint& GetTargetEndpoint() const override;
        const Endpoint& GetSourceEndpoint() const override;

        // GraphNotificationBus
        void OnNodeRemoved(const ID& nodeId) override;

    protected:
        //-------------------------------------------------------------------------
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasConnectionService", 0x028dc06e));
        }

        Endpoint m_sourceEndpoint;
        Endpoint m_targetEndpoint;
    };

}