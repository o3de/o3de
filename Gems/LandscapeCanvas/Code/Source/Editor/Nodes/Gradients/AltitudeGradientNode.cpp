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
#include "AltitudeGradientNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void AltitudeGradientNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AltitudeGradientNode, BaseGradientNode>()
                ->Version(0)
                ;
        }
    }

    const QString AltitudeGradientNode::TITLE = QObject::tr("Altitude");

    AltitudeGradientNode::AltitudeGradientNode(GraphModel::GraphPtr graph)
        : BaseGradientNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void AltitudeGradientNode::RegisterSlots()
    {
        BaseGradientNode::RegisterSlots();

        // Altitude Gradient has an additional input slot for an inbound shape
        GraphModel::DataTypePtr invalidEntityDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::InvalidEntity);
        GraphModel::DataTypePtr boundsDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            PIN_TO_SHAPE_SLOT_ID,
            PIN_TO_SHAPE_SLOT_LABEL.toUtf8().constData(),
            { boundsDataType, invalidEntityDataType },
            AZStd::any(AZ::EntityId()),
            PIN_TO_SHAPE_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
