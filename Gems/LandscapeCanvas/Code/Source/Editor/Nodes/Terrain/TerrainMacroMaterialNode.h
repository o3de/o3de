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
    class TerrainMacroMaterialNode
        : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainMacroMaterialNode, AZ::SystemAllocator);
        AZ_RTTI(TerrainMacroMaterialNode, "{E55E39AA-133C-40CE-8FDE-CF674D0E8BB2}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        TerrainMacroMaterialNode() = default;
        explicit TerrainMacroMaterialNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override;

        static const char* TITLE;
        const char* GetTitle() const override;

    protected:
        void RegisterSlots() override;
    };
}
