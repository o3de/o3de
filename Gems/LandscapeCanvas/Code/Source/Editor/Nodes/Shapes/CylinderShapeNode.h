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
    class CylinderShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CylinderShapeNode, AZ::SystemAllocator);
        AZ_RTTI(CylinderShapeNode, "{5F0DD9F7-BCB8-4428-A3BD-8FEC3141E2D0}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        CylinderShapeNode() = default;
        explicit CylinderShapeNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
