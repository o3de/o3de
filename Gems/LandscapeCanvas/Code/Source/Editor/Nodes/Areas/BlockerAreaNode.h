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

    class BlockerAreaNode : public BaseAreaNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BlockerAreaNode, AZ::SystemAllocator);
        AZ_RTTI(BlockerAreaNode, "{EF5E78F2-00A6-472E-9411-8CD31953B9CB}", BaseAreaNode);

        static void Reflect(AZ::ReflectContext* context);

        BlockerAreaNode() = default;
        explicit BlockerAreaNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
