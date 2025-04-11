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

    class PerlinNoiseGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(PerlinNoiseGradientNode, AZ::SystemAllocator);
        AZ_RTTI(PerlinNoiseGradientNode, "{11900419-DDC1-401C-97CC-8986600EE725}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        PerlinNoiseGradientNode() = default;
        explicit PerlinNoiseGradientNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_GENERATOR_TITLE; }

        const BaseNodeType GetBaseNodeType() const override { return BaseNode::GradientGenerator; }
    };
}
