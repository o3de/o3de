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

    class SlopeGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SlopeGradientNode, AZ::SystemAllocator);
        AZ_RTTI(SlopeGradientNode, "{180A74BE-2244-4DF1-B656-68C66CBD68E7}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        SlopeGradientNode() = default;
        explicit SlopeGradientNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_TITLE; }
    };
}
