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

    class LevelsGradientModifierNode : public BaseGradientModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(LevelsGradientModifierNode, AZ::SystemAllocator);
        AZ_RTTI(LevelsGradientModifierNode, "{59013DA0-EC62-4BB8-A64D-417948601B59}", BaseGradientModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        LevelsGradientModifierNode() = default;
        explicit LevelsGradientModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
