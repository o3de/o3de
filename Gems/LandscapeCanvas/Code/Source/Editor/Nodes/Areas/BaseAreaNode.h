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
    class BaseAreaNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseAreaNode, AZ::SystemAllocator);
        AZ_RTTI(BaseAreaNode, "{16CC0816-6A5F-4244-B66A-2D34B6D4E508}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        BaseAreaNode() = default;
        explicit BaseAreaNode(GraphModel::GraphPtr graph);

        const char* GetSubTitle() const override
        {
            return LandscapeCanvas::VEGETATION_AREA_TITLE;
        }

        GraphModel::NodeType GetNodeType() const override
        {
            return GraphModel::NodeType::WrapperNode;
        }

        const BaseNodeType GetBaseNodeType() const override
        {
            return BaseNode::VegetationArea;
        }

        /// Retrieve a pointer to the Reference Shape Component on this Entity
        /// if it exists (to be used for getting/setting the Placement Bounds)
        AZ::Component* GetReferenceShapeComponent() const;

    protected:
        void RegisterSlots() override;
    };
}
