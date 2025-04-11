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
    class TerrainPhysicsHeightfieldColliderNode
        : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainPhysicsHeightfieldColliderNode, AZ::SystemAllocator);
        AZ_RTTI(TerrainPhysicsHeightfieldColliderNode, "{8F7DB486-972B-427C-9D1D-CF798D569847}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        TerrainPhysicsHeightfieldColliderNode() = default;
        explicit TerrainPhysicsHeightfieldColliderNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override;

        static const char* TITLE;
        const char* GetTitle() const override;

        AZ::ComponentDescriptor::DependencyArrayType GetOptionalRequiredServices() const override;
    };
}
