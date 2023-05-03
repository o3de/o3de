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
#include <Editor/Nodes/BaseNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class TerrainSurfaceMaterialsListNode
        : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainSurfaceMaterialsListNode, AZ::SystemAllocator);
        AZ_RTTI(TerrainSurfaceMaterialsListNode, "{41A168E4-6C30-40FA-889A-D2B58724A1D9}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        TerrainSurfaceMaterialsListNode() = default;
        explicit TerrainSurfaceMaterialsListNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override;

        static const char* TITLE;
        const char* GetTitle() const override;

    protected:
        void RegisterSlots() override;
    };
}
