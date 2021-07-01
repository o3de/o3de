/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <AzCore/Component/EntityBus.h>

#include "Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h"

#include "ScriptCanvas/Core/NodeBus.h"
#include "ScriptCanvas/Core/Endpoint.h"

namespace ScriptCanvasEditor
{
    class EntityRefNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public AZ::EntityBus::Handler        
    {
    public:
        AZ_COMPONENT(EntityRefNodeDescriptorComponent, "{887AE9AC-C793-4FE5-BAE2-AF6A7F70A374}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        EntityRefNodeDescriptorComponent();
        ~EntityRefNodeDescriptorComponent() = default;

        // AZ::EntityBus
        void OnEntityNameChanged(const AZStd::string& name) override;
        ////
        
        // ScriptCanvas::NodeNotificationsBus
        void OnSlotInputChanged(const ScriptCanvas::SlotId& slotId) override;
        ////

    private:

        void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId) override;

        AZ::EntityId GetReferencedEntityId() const;
        void UpdateNodeTitle();
        
        ScriptCanvas::Endpoint m_endpoint;
    };
}
