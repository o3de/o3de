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

// Landscape Canvas
#include <Editor/Core/Core.h>
#include <Editor/Nodes/BaseNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class BaseGradientNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseGradientNode, AZ::SystemAllocator, 0);
        AZ_RTTI(BaseGradientNode, "{9B58A983-243F-43A6-ABC0-6D6B8D7BCB4C}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        BaseGradientNode() = default;
        explicit BaseGradientNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override { return BaseNode::Gradient; }

    protected:
        void RegisterSlots() override;
    };
}
