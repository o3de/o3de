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

    const QString AreaBlenderNode::TITLE = QObject::tr("Vegetation Layer Blender");

    AreaBlenderNode::AreaBlenderNode(GraphModel::GraphPtr graph)
        : BaseAreaNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void AreaBlenderNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        GraphModel::DataTypePtr invalidEntityDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::InvalidEntity);
        GraphModel::DataTypePtr areaDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Area);

        GraphModel::ExtendableSlotConfiguration slotConfig;
        slotConfig.m_addButtonLabel = QObject::tr("Add Area").toUtf8().constData();
        slotConfig.m_addButtonTooltip = QObject::tr("Add a layer area to the blender").toUtf8().constData();
        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            INBOUND_AREA_SLOT_ID,
            INBOUND_AREA_SLOT_LABEL.toUtf8().constData(),
            { areaDataType, invalidEntityDataType },
            areaDataType->GetDefaultValue(),
            INBOUND_AREA_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            &slotConfig));

        RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
            OUTBOUND_AREA_SLOT_ID,
            OUTBOUND_AREA_SLOT_LABEL.toUtf8().constData(),
            areaDataType,
            OUTBOUND_AREA_OUTPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
