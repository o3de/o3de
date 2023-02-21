/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(BaseGradientNode, AZ::SystemAllocator);
        AZ_RTTI(BaseGradientNode, "{9B58A983-243F-43A6-ABC0-6D6B8D7BCB4C}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        BaseGradientNode() = default;
        explicit BaseGradientNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override { return BaseNode::Gradient; }

    protected:
        void RegisterSlots() override;
    };
}
