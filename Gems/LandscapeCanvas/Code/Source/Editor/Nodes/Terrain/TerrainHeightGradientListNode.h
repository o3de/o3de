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
#include <Editor/Nodes/BaseNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class TerrainHeightGradientListNode
        : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainHeightGradientListNode, AZ::SystemAllocator);
        AZ_RTTI(TerrainHeightGradientListNode, "{10BE90E1-C508-403B-B1BE-AFB8D8C1BFFE}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        TerrainHeightGradientListNode() = default;
        explicit TerrainHeightGradientListNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override;

        static const char* TITLE;
        const char* GetTitle() const override;

    protected:
        void RegisterSlots() override;
    };
}
