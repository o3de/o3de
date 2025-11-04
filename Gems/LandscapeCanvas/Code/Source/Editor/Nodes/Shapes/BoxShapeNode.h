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
    class BoxShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BoxShapeNode, AZ::SystemAllocator);
        AZ_RTTI(BoxShapeNode, "{8901D121-7F43-4EDE-8858-AFAC9D2E55F0}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        BoxShapeNode() = default;
        explicit BoxShapeNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }

    };
}
