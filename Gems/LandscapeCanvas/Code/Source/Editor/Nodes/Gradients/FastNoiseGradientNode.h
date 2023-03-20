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
#include <Editor/Core/Core.h>
#include <Editor/Nodes/Gradients/BaseGradientNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class FastNoiseGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(FastNoiseGradientNode, AZ::SystemAllocator);
        AZ_RTTI(FastNoiseGradientNode, "{38A4CDEA-082B-4769-922B-713BF77CEA28}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        FastNoiseGradientNode() = default;
        explicit FastNoiseGradientNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_GENERATOR_TITLE; }

        const BaseNodeType GetBaseNodeType() const override { return BaseNode::GradientGenerator; }
    };
}
