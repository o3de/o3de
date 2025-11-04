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
    class RotationModifierNode : public BaseAreaModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(RotationModifierNode, AZ::SystemAllocator);
        AZ_RTTI(RotationModifierNode, "{14B577ED-4135-4711-B9F3-016E106EA66B}", BaseAreaModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        RotationModifierNode() = default;
        explicit RotationModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override
        {
            return TITLE;
        }

    protected:
        void RegisterSlots() override;
    };
}
