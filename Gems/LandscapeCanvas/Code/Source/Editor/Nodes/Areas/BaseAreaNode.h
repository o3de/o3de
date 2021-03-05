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
    class BaseAreaNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseAreaNode, AZ::SystemAllocator, 0);
        AZ_RTTI(BaseAreaNode, "{16CC0816-6A5F-4244-B66A-2D34B6D4E508}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        BaseAreaNode() = default;
        explicit BaseAreaNode(GraphModel::GraphPtr graph);

        const char* GetSubTitle() const override
        {
            return LandscapeCanvas::VEGETATION_AREA_TITLE.toUtf8().constData();
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
