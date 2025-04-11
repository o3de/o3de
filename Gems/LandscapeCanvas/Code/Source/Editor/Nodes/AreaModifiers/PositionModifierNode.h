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
#include <Editor/Nodes/AreaModifiers/BaseAreaModifierNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class PositionModifierNode : public BaseAreaModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(PositionModifierNode, AZ::SystemAllocator);
        AZ_RTTI(PositionModifierNode, "{3613E5F4-BBFF-4FC5-90B5-902B3FFE7F8D}", BaseAreaModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        PositionModifierNode() = default;
        explicit PositionModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override
        {
            return TITLE;
        }

    protected:
        void RegisterSlots() override;
    };
}
