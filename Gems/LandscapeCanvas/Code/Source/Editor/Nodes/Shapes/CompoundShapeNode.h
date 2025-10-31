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
    class CompoundShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CompoundShapeNode, AZ::SystemAllocator);
        AZ_RTTI(CompoundShapeNode, "{96A15E81-7271-4580-85F4-0D417807E86A}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        CompoundShapeNode() = default;
        explicit CompoundShapeNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
