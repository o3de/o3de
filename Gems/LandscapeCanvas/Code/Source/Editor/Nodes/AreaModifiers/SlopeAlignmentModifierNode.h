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
    class SlopeAlignmentModifierNode : public BaseAreaModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SlopeAlignmentModifierNode, AZ::SystemAllocator);
        AZ_RTTI(SlopeAlignmentModifierNode, "{3F5771DF-C2A0-4B52-9A91-96253A640A7C}", BaseAreaModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        SlopeAlignmentModifierNode() = default;
        explicit SlopeAlignmentModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override
        {
            return TITLE;
        }

    protected:
        void RegisterSlots() override;
    };
}
