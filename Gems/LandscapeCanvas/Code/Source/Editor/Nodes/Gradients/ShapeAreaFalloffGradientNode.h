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

    class ShapeAreaFalloffGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(ShapeAreaFalloffGradientNode, AZ::SystemAllocator);
        AZ_RTTI(ShapeAreaFalloffGradientNode, "{8871F483-5087-4776-A4F8-35388B3D9CE0}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        ShapeAreaFalloffGradientNode() = default;
        explicit ShapeAreaFalloffGradientNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_TITLE; }

    protected:
        void RegisterSlots() override;
    };
}
