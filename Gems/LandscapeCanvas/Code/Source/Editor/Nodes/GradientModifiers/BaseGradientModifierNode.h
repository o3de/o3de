/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Landscape Canvas
#include <Editor/Core/Core.h>
#include <Editor/Nodes/BaseNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class BaseGradientModifierNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseGradientModifierNode, AZ::SystemAllocator);
        AZ_RTTI(BaseGradientModifierNode, "{A918BFDF-4871-4100-BDB0-AE575E5287A2}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        BaseGradientModifierNode() = default;
        explicit BaseGradientModifierNode(GraphModel::GraphPtr graph);

        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_MODIFIER_TITLE; }

        const BaseNodeType GetBaseNodeType() const override { return BaseNode::GradientModifier; }

    protected:
        virtual void RegisterSlots() override;
    };
}
