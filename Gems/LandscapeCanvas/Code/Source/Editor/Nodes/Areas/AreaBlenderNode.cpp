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
#include "AreaBlenderNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void AreaBlenderNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AreaBlenderNode, BaseAreaNode>()
                ->Version(0)
                ;
        }
    }

    const char* AreaBlenderNode::TITLE = "Vegetation Layer Blender";

    AreaBlenderNode::AreaBlenderNode(GraphModel::GraphPtr graph)
        : BaseAreaNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void AreaBlenderNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        GraphModel::DataTypePtr areaDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Area);

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Data,
            INBOUND_AREA_SLOT_ID,
            INBOUND_AREA_SLOT_LABEL.toUtf8().constData(),
            INBOUND_AREA_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ areaDataType },
            AZStd::any(AZ::EntityId()),
            1,
            100,
            QObject::tr("Add Area").toUtf8().constData(),
            QObject::tr("Add a layer area to the blender").toUtf8().constData()));

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Output,
            GraphModel::SlotType::Data,
            OUTBOUND_AREA_SLOT_ID,
            OUTBOUND_AREA_SLOT_LABEL.toUtf8().constData(),
            OUTBOUND_AREA_OUTPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ areaDataType }));
    }
} // namespace LandscapeCanvas
