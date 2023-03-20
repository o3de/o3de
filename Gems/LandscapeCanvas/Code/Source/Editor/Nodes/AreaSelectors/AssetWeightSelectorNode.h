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
    class AssetWeightSelectorNode
        : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetWeightSelectorNode, AZ::SystemAllocator);
        AZ_RTTI(AssetWeightSelectorNode, "{083CA722-638B-4E14-836B-2614451C2A91}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        AssetWeightSelectorNode() = default;
        explicit AssetWeightSelectorNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override;

        static const char* TITLE;
        const char* GetTitle() const override;

    protected:
        void RegisterSlots() override;
    };
}
