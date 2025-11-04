/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Landscape Canvas
#include <Editor/Nodes/Shapes/BaseShapeNode.h>

namespace LandscapeCanvas
{
    class AxisAlignedBoxShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(AxisAlignedBoxShapeNode, AZ::SystemAllocator);
        AZ_RTTI(AxisAlignedBoxShapeNode, "{C33B49D1-F3AF-432D-9DD5-239C053A5A55}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        AxisAlignedBoxShapeNode() = default;
        explicit AxisAlignedBoxShapeNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }

    };
}
