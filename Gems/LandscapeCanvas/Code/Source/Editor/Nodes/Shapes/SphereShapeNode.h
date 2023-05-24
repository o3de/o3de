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
    class SphereShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SphereShapeNode, AZ::SystemAllocator);
        AZ_RTTI(SphereShapeNode, "{9ED3D9AF-E82D-4291-874A-B0758879CF39}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        SphereShapeNode() = default;
        explicit SphereShapeNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
