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
    class PolygonPrismShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(PolygonPrismShapeNode, AZ::SystemAllocator);
        AZ_RTTI(PolygonPrismShapeNode, "{2DF92351-E990-4DCF-920E-4EFF7C061964}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        PolygonPrismShapeNode() = default;
        explicit PolygonPrismShapeNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
