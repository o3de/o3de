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
    class TerrainSurfaceGradientListNode
        : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainSurfaceGradientListNode, AZ::SystemAllocator);
        AZ_RTTI(TerrainSurfaceGradientListNode, "{9414099F-A3BB-432E-86B8-3FB2C44D2529}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        TerrainSurfaceGradientListNode() = default;
        explicit TerrainSurfaceGradientListNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override;

        static const char* TITLE;
        const char* GetTitle() const override;

    protected:
        void RegisterSlots() override;
    };
}
