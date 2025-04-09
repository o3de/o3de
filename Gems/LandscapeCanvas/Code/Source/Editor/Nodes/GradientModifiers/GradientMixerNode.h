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
    class GradientMixerNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientMixerNode, AZ::SystemAllocator);
        AZ_RTTI(GradientMixerNode, "{D5AEAA23-12EB-4E55-B396-BEE13724FBDC}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        GradientMixerNode() = default;
        explicit GradientMixerNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override;
        const char* GetSubTitle() const override;
        const BaseNodeType GetBaseNodeType() const override;

    protected:
        virtual void RegisterSlots() override;
    };
}
