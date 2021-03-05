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
    class BaseAreaFilterNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseAreaFilterNode, AZ::SystemAllocator, 0);
        AZ_RTTI(BaseAreaFilterNode, "{53A10ED3-5ECF-421B-B824-84D4C052E92B}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        BaseAreaFilterNode() = default;
        explicit BaseAreaFilterNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override
        {
            return BaseNode::VegetationAreaFilter;
        }

        const bool ShouldShowEntityName() const override
        {
            // Don't show the entity name for Area Filters since they will
            // be wrapped on a Vegetation Area it would be redundant
            return false;
        }
    };
}
