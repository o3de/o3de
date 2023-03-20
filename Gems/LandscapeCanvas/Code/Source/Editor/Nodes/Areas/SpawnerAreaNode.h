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

    class SpawnerAreaNode : public BaseAreaNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SpawnerAreaNode, AZ::SystemAllocator);
        AZ_RTTI(SpawnerAreaNode, "{664DF87C-5420-44EA-8CFF-BC8D63F52112}", BaseAreaNode);

        static void Reflect(AZ::ReflectContext* context);

        SpawnerAreaNode() = default;
        explicit SpawnerAreaNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
