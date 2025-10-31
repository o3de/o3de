/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(BaseShapeNode, AZ::SystemAllocator);
        AZ_RTTI(BaseShapeNode, "{1A9B84EC-22FA-4139-9D86-B158688612E2}", BaseNode);

        // Category sub-title
        static const char* SHAPE_CATEGORY_TITLE;

        // Connection slot ID (not translated, only used internally)
        static const char* BOUNDS_SLOT_ID;

        // Connection slot label
        static const QString BOUNDS_SLOT_LABEL;

        // Connection slot description
        static const QString BOUNDS_OUTPUT_SLOT_DESCRIPTION;

        static void Reflect(AZ::ReflectContext* context);

        BaseShapeNode() = default;
        explicit BaseShapeNode(GraphModel::GraphPtr graph);

        const char* GetSubTitle() const override { return SHAPE_CATEGORY_TITLE; }

        const BaseNodeType GetBaseNodeType() const override { return BaseNode::Shape; }

    protected:
        void RegisterSlots() override;
    };
}
