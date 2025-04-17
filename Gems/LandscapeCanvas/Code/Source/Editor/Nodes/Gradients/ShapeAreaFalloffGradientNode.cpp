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
#include "ShapeAreaFalloffGradientNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void ShapeAreaFalloffGradientNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ShapeAreaFalloffGradientNode, BaseGradientNode>()
                ->Version(0)
                ;
        }
    }

    const char* ShapeAreaFalloffGradientNode::TITLE = "Shape Falloff";

    ShapeAreaFalloffGradientNode::ShapeAreaFalloffGradientNode(GraphModel::GraphPtr graph)
        : BaseGradientNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void ShapeAreaFalloffGradientNode::RegisterSlots()
    {
        BaseGradientNode::RegisterSlots();

        // Shape Falloff Gradient has an additional input slot for an inbound shape
        GraphModel::DataTypePtr boundsDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Data,
            INBOUND_SHAPE_SLOT_ID,
            INBOUND_SHAPE_SLOT_LABEL.toUtf8().constData(),
            INBOUND_SHAPE_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ boundsDataType },
            AZStd::any(AZ::EntityId())));
    }
} // namespace LandscapeCanvas
