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
#include <Editor/Nodes/Areas/BaseAreaNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{

    class AreaBlenderNode : public BaseAreaNode
    {
    public:
        AZ_CLASS_ALLOCATOR(AreaBlenderNode, AZ::SystemAllocator);
        AZ_RTTI(AreaBlenderNode, "{07EFA0EA-F5E1-44A0-8620-D5A75F2D2BED}", BaseAreaNode);

        static void Reflect(AZ::ReflectContext* context);

        AreaBlenderNode() = default;
        explicit AreaBlenderNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }

    protected:
        void RegisterSlots() override;
    };
}
