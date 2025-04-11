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

    class ImageGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(ImageGradientNode, AZ::SystemAllocator);
        AZ_RTTI(ImageGradientNode, "{EA6E28AC-19C3-45B5-8D3E-01778B57AA85}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        ImageGradientNode() = default;
        explicit ImageGradientNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_TITLE; }

    protected:
        void RegisterSlots() override;
    };
}
