/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        AZ_CLASS_ALLOCATOR(BaseGradientModifierNode, AZ::SystemAllocator, 0);
        AZ_RTTI(BaseGradientModifierNode, "{A918BFDF-4871-4100-BDB0-AE575E5287A2}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        BaseGradientModifierNode() = default;
        explicit BaseGradientModifierNode(GraphModel::GraphPtr graph);

        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_MODIFIER_TITLE.toUtf8().constData(); }

        const BaseNodeType GetBaseNodeType() const override { return BaseNode::GradientModifier; }

    protected:
        virtual void RegisterSlots() override;
    };
}
