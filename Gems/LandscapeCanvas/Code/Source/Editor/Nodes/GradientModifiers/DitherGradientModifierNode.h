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
#include <Editor/Nodes/GradientModifiers/BaseGradientModifierNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{

    class DitherGradientModifierNode : public BaseGradientModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(DitherGradientModifierNode, AZ::SystemAllocator);
        AZ_RTTI(DitherGradientModifierNode, "{14B9E63F-5ECA-49AA-B50F-7238B1442E3D}", BaseGradientModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        DitherGradientModifierNode() = default;
        explicit DitherGradientModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
