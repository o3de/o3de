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
    class GradientBakerNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientBakerNode, AZ::SystemAllocator);
        AZ_RTTI(GradientBakerNode, "{29C0697B-068E-49DF-8D44-36DD98599C30}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        GradientBakerNode() = default;
        explicit GradientBakerNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override;
        const char* GetSubTitle() const override;
        const BaseNodeType GetBaseNodeType() const override;

    protected:
        void RegisterSlots() override;
    };
}
