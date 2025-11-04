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

    class SmoothStepGradientModifierNode : public BaseGradientModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SmoothStepGradientModifierNode, AZ::SystemAllocator);
        AZ_RTTI(SmoothStepGradientModifierNode, "{F0A862F4-DE3A-401C-8138-78B0804C2259}", BaseGradientModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        SmoothStepGradientModifierNode() = default;
        explicit SmoothStepGradientModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
