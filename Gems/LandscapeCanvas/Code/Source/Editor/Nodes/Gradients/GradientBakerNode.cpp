/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientBakerNode.h"

#include <QObject>

#include <AzCore/Serialization/SerializeContext.h>

#include <GraphModel/Model/Slot.h>

#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void GradientBakerNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GradientBakerNode, BaseGradientNode>()
                ->Version(0)
                ;
        }
    }

    const QString GradientBakerNode::TITLE = QObject::tr("Gradient Baker");

    GradientBakerNode::GradientBakerNode(GraphModel::GraphPtr graph)
        : BaseGradientNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* GradientBakerNode::GetTitle() const
    {
        return TITLE.toUtf8().constData();
    }

    const char* GradientBakerNode::GetSubTitle() const
    {
        return GRADIENT_GENERATOR_TITLE.toUtf8().constData();
    }

    const BaseNode::BaseNodeType GradientBakerNode::GetBaseNodeType() const
    {
        return BaseNode::GradientGenerator;
    }

    void GradientBakerNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        GraphModel::DataTypePtr boundsDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);
        GraphModel::DataTypePtr gradientDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);
        GraphModel::DataTypePtr pathDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Path);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            INPUT_BOUNDS_SLOT_ID,
            INPUT_BOUNDS_SLOT_LABEL.toUtf8().constData(),
            { boundsDataType },
            AZStd::any(AZ::EntityId()),
            INPUT_BOUNDS_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            INBOUND_GRADIENT_SLOT_ID,
            INBOUND_GRADIENT_SLOT_LABEL.toUtf8().constData(),
            { gradientDataType },
            AZStd::any(AZ::EntityId()),
            INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));

        RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
            OUTPUT_IMAGE_SLOT_ID,
            OUTPUT_IMAGE_SLOT_LABEL.toUtf8().constData(),
            pathDataType,
            OUTPUT_IMAGE_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
