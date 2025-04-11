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
    class ScaleModifierNode : public BaseAreaModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(ScaleModifierNode, AZ::SystemAllocator);
        AZ_RTTI(ScaleModifierNode, "{470E2762-A7DF-4500-B4BE-B705ED7EDEDC}", BaseAreaModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        ScaleModifierNode() = default;
        explicit ScaleModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override
        {
            return TITLE;
        }

    protected:
        void RegisterSlots() override;
    };
}
