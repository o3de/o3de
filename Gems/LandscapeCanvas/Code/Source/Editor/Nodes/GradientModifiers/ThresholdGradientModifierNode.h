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

    class ThresholdGradientModifierNode : public BaseGradientModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(ThresholdGradientModifierNode, AZ::SystemAllocator);
        AZ_RTTI(ThresholdGradientModifierNode, "{3AFD613A-E5E4-4359-9E20-335F202AAB99}", BaseGradientModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        ThresholdGradientModifierNode() = default;
        explicit ThresholdGradientModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
