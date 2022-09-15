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

    class ConstantGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(ConstantGradientNode, AZ::SystemAllocator, 0);
        AZ_RTTI(ConstantGradientNode, "{3D45B989-815F-461E-943D-F5139C1C6F06}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        ConstantGradientNode() = default;
        explicit ConstantGradientNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_TITLE; }
    };
}
