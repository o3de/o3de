/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Qt
#include <QObject>

// AZ
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <GraphModel/Model/Slot.h>

// Landscape Canvas
#include "PositionModifierNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void PositionModifierNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PositionModifierNode, BaseAreaModifierNode>()
                ->Version(0)
                ;
        }
    }

    const char* PositionModifierNode::TITLE = "Position Modifier";

    PositionModifierNode::PositionModifierNode(GraphModel::GraphPtr graph)
        : BaseAreaModifierNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void PositionModifierNode::RegisterSlots()
    {
        GraphModel::DataTypePtr gradientDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Data,
            BaseAreaModifierNode::INBOUND_GRADIENT_X_SLOT_ID,
            BaseAreaModifierNode::INBOUND_GRADIENT_X_SLOT_LABEL.toUtf8().constData(),
            BaseAreaModifierNode::INBOUND_GRADIENT_X_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ gradientDataType },
            AZStd::any(AZ::EntityId())));

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Data,
            BaseAreaModifierNode::INBOUND_GRADIENT_Y_SLOT_ID,
            BaseAreaModifierNode::INBOUND_GRADIENT_Y_SLOT_LABEL.toUtf8().constData(),
            BaseAreaModifierNode::INBOUND_GRADIENT_Y_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ gradientDataType },
            AZStd::any(AZ::EntityId())));

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Data,
            BaseAreaModifierNode::INBOUND_GRADIENT_Z_SLOT_ID,
            BaseAreaModifierNode::INBOUND_GRADIENT_Z_SLOT_LABEL.toUtf8().constData(),
            BaseAreaModifierNode::INBOUND_GRADIENT_Z_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ gradientDataType },
            AZStd::any(AZ::EntityId())));
    }
} // namespace LandscapeCanvas
