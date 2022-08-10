/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QString>

#include <Editor/Core/Core.h>
#include <Editor/Nodes/Gradients/BaseGradientNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class GradientPainterNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientPainterNode, AZ::SystemAllocator, 0);
        AZ_RTTI(GradientPainterNode, "{01752BC7-2B2B-4C00-B059-0A7A494EFB6F}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        GradientPainterNode() = default;
        explicit GradientPainterNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override;
        const char* GetSubTitle() const override;
        const BaseNodeType GetBaseNodeType() const override;

    protected:
        void RegisterSlots() override;
    };
}
