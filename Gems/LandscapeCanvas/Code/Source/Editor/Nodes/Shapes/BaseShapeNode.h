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

// Qt
#include <QString>

// Landscape Canvas
#include <Editor/Nodes/BaseNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class BaseShapeNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseShapeNode, AZ::SystemAllocator, 0);
        AZ_RTTI(BaseShapeNode, "{1A9B84EC-22FA-4139-9D86-B158688612E2}", BaseNode);

        // Category sub-title
        static const QString SHAPE_CATEGORY_TITLE;

        // Connection slot ID (not translated, only used internally)
        static const char* BOUNDS_SLOT_ID;

        // Connection slot label
        static const QString BOUNDS_SLOT_LABEL;

        // Connection slot description
        static const QString BOUNDS_OUTPUT_SLOT_DESCRIPTION;

        static void Reflect(AZ::ReflectContext* context);

        BaseShapeNode() = default;
        explicit BaseShapeNode(GraphModel::GraphPtr graph);

        const char* GetSubTitle() const override { return SHAPE_CATEGORY_TITLE.toUtf8().constData(); }

        const BaseNodeType GetBaseNodeType() const override { return BaseNode::Shape; }

    protected:
        void RegisterSlots() override;
    };
}
