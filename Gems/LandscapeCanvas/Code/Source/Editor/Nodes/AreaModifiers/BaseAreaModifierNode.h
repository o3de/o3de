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
    class BaseAreaModifierNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseAreaModifierNode, AZ::SystemAllocator);
        AZ_RTTI(BaseAreaModifierNode, "{9FCA4158-1974-4CE3-93B9-10F1D6A25D9F}", BaseNode);

        // Connection slot IDs (not translated, only used internally)
        static const char* INBOUND_GRADIENT_X_SLOT_ID;
        static const char* INBOUND_GRADIENT_Y_SLOT_ID;
        static const char* INBOUND_GRADIENT_Z_SLOT_ID;

        // Connection slot labels
        static const QString INBOUND_GRADIENT_X_SLOT_LABEL;
        static const QString INBOUND_GRADIENT_Y_SLOT_LABEL;
        static const QString INBOUND_GRADIENT_Z_SLOT_LABEL;

        // Connection slot descriptions
        static const QString INBOUND_GRADIENT_X_INPUT_SLOT_DESCRIPTION;
        static const QString INBOUND_GRADIENT_Y_INPUT_SLOT_DESCRIPTION;
        static const QString INBOUND_GRADIENT_Z_INPUT_SLOT_DESCRIPTION;

        static void Reflect(AZ::ReflectContext* context);

        BaseAreaModifierNode() = default;
        explicit BaseAreaModifierNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override
        {
            return BaseNode::VegetationAreaModifier;
        }
    };
}
