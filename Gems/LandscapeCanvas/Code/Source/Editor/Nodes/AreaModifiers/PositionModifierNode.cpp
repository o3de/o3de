/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// Qt
#include <QObject>

// AZ
#include <AzCore/Serialization/SerializeContext.h>

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

    const QString PositionModifierNode::TITLE = QObject::tr("Position Modifier");

    PositionModifierNode::PositionModifierNode(GraphModel::GraphPtr graph)
        : BaseAreaModifierNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void PositionModifierNode::RegisterSlots()
    {
        GraphModel::DataTypePtr invalidEntityDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::InvalidEntity);
        GraphModel::DataTypePtr gradientDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            BaseAreaModifierNode::INBOUND_GRADIENT_X_SLOT_ID,
            BaseAreaModifierNode::INBOUND_GRADIENT_X_SLOT_LABEL.toUtf8().constData(),
            { gradientDataType, invalidEntityDataType },
            AZStd::any(AZ::EntityId()),
            BaseAreaModifierNode::INBOUND_GRADIENT_X_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            BaseAreaModifierNode::INBOUND_GRADIENT_Y_SLOT_ID,
            BaseAreaModifierNode::INBOUND_GRADIENT_Y_SLOT_LABEL.toUtf8().constData(),
            { gradientDataType, invalidEntityDataType },
            AZStd::any(AZ::EntityId()),
            BaseAreaModifierNode::INBOUND_GRADIENT_Y_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            BaseAreaModifierNode::INBOUND_GRADIENT_Z_SLOT_ID,
            BaseAreaModifierNode::INBOUND_GRADIENT_Z_SLOT_LABEL.toUtf8().constData(),
            { gradientDataType, invalidEntityDataType },
            AZStd::any(AZ::EntityId()),
            BaseAreaModifierNode::INBOUND_GRADIENT_Z_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
