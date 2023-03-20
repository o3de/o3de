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

    const char* DistributionFilterNode::TITLE = "Distribution Filter";

    DistributionFilterNode::DistributionFilterNode(GraphModel::GraphPtr graph)
        : BaseAreaFilterNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void DistributionFilterNode::RegisterSlots()
    {
        GraphModel::DataTypePtr gradientDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Data,
            INBOUND_GRADIENT_SLOT_ID,
            INBOUND_GRADIENT_SLOT_LABEL.toUtf8().constData(),
            INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ gradientDataType },
            AZStd::any(AZ::EntityId())));
    }
} // namespace LandscapeCanvas
