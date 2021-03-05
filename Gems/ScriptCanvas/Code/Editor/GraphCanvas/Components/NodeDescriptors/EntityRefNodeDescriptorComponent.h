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
        void OnInputChanged(const ScriptCanvas::SlotId& slotId) override;
        ////

    private:

        void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId) override;

        AZ::EntityId GetReferencedEntityId() const;
        void UpdateNodeTitle();
        
        ScriptCanvas::Endpoint m_endpoint;
    };
}