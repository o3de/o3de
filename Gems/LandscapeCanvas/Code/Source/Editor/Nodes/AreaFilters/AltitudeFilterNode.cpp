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

// Graph Model
#include <GraphModel/Model/Slot.h>

// Landscape Canvas
#include "AltitudeFilterNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void AltitudeFilterNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AltitudeFilterNode, BaseAreaFilterNode>()
                ->Version(0)
                ;
        }
    }

    const QString AltitudeFilterNode::TITLE = QObject::tr("Altitude Filter");

    AltitudeFilterNode::AltitudeFilterNode(GraphModel::GraphPtr graph)
        : BaseAreaFilterNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void AltitudeFilterNode::RegisterSlots()
    {
        GraphModel::DataTypePtr boundsDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            PIN_TO_SHAPE_SLOT_ID,
            PIN_TO_SHAPE_SLOT_LABEL.toUtf8().constData(),
            { boundsDataType },
            AZStd::any(AZ::EntityId()),
            PIN_TO_SHAPE_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
