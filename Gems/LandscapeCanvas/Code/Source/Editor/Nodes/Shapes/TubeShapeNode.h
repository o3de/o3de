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
    class TubeShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(TubeShapeNode, AZ::SystemAllocator);
        AZ_RTTI(TubeShapeNode, "{2A0193AD-25AC-49CA-B90E-8C61A5C1CCC5}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        TubeShapeNode() = default;
        explicit TubeShapeNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
