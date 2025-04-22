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
    class BaseAreaFilterNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseAreaFilterNode, AZ::SystemAllocator);
        AZ_RTTI(BaseAreaFilterNode, "{53A10ED3-5ECF-421B-B824-84D4C052E92B}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        BaseAreaFilterNode() = default;
        explicit BaseAreaFilterNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override
        {
            return BaseNode::VegetationAreaFilter;
        }
    };
}
