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
    class TerrainLayerSpawnerNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainLayerSpawnerNode, AZ::SystemAllocator);
        AZ_RTTI(TerrainLayerSpawnerNode, "{C901635B-4EC8-40A1-8D67-4138C7567C3E}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        TerrainLayerSpawnerNode() = default;
        explicit TerrainLayerSpawnerNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override;
        const char* GetSubTitle() const override;
        GraphModel::NodeType GetNodeType() const override;
        const BaseNodeType GetBaseNodeType() const override;

    protected:
        void RegisterSlots() override;
    };
}
