/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include "Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h"

namespace ScriptCanvasEditor
{
    class EBusSenderNodeDescriptorComponent
        : public NodeDescriptorComponent        
    {
    public:
        AZ_COMPONENT(EBusSenderNodeDescriptorComponent, "{6B646A3A-CB7F-49C4-8146-D848F418E0B1}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        EBusSenderNodeDescriptorComponent();
        ~EBusSenderNodeDescriptorComponent() = default;

    protected:
        void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId) override;
    };
}
