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

    class InvertGradientModifierNode : public BaseGradientModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(InvertGradientModifierNode, AZ::SystemAllocator);
        AZ_RTTI(InvertGradientModifierNode, "{B98466A1-1980-43B6-909F-671AF3987E3B}", BaseGradientModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        InvertGradientModifierNode() = default;
        explicit InvertGradientModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
