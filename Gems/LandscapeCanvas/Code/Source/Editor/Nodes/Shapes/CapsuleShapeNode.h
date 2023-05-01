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
    class CapsuleShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CapsuleShapeNode, AZ::SystemAllocator);
        AZ_RTTI(CapsuleShapeNode, "{537C20E6-D319-4CEE-BAA5-75C4B52E5BE0}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        CapsuleShapeNode() = default;
        explicit CapsuleShapeNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
