/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientPainterNode.h"

#include <QObject>

#include <AzCore/Serialization/SerializeContext.h>

#include <GraphModel/Model/Slot.h>

#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void GradientPainterNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GradientPainterNode, BaseGradientNode>()
                ->Version(0)
                ;
        }
    }

    const QString GradientPainterNode::TITLE = QObject::tr("Gradient Painter");

    GradientPainterNode::GradientPainterNode(GraphModel::GraphPtr graph)
        : BaseGradientNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* GradientPainterNode::GetTitle() const
    {
        return TITLE.toUtf8().constData();
    }

    const char* GradientPainterNode::GetSubTitle() const
    {
        return GRADIENT_GENERATOR_TITLE.toUtf8().constData();
    }

    const BaseNode::BaseNodeType GradientPainterNode::GetBaseNodeType() const
    {
        return BaseNode::GradientGenerator;
    }

    void GradientPainterNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        GraphModel::DataTypePtr pathDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Path);

        RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
            OUTPUT_IMAGE_SLOT_ID,
            OUTPUT_IMAGE_SLOT_LABEL.toUtf8().constData(),
            pathDataType,
            OUTPUT_IMAGE_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
