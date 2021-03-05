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