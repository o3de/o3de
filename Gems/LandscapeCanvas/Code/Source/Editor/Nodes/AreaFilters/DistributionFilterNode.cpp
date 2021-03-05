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
#include "DistributionFilterNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void DistributionFilterNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DistributionFilterNode, BaseAreaFilterNode>()
                ->Version(0)
                ;
        }
    }

    const QString DistributionFilterNode::TITLE = QObject::tr("Distribution Filter");

    DistributionFilterNode::DistributionFilterNode(GraphModel::GraphPtr graph)
        : BaseAreaFilterNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void DistributionFilterNode::RegisterSlots()
    {
        GraphModel::DataTypePtr invalidEntityDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::InvalidEntity);
        GraphModel::DataTypePtr gradientDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            INBOUND_GRADIENT_SLOT_ID,
            INBOUND_GRADIENT_SLOT_LABEL.toUtf8().constData(),
            { gradientDataType, invalidEntityDataType },
            AZStd::any(AZ::EntityId()),
            INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
