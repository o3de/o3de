/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QObject>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <GraphModel/Integration/Helpers.h>
#include <GraphModel/Model/Slot.h>

#include "ReferenceShapeNode.h"
#include <Editor/Core/Core.h>
#include <Editor/Core/GraphContext.h>
#include <Editor/Nodes/Shapes/BaseShapeNode.h>

namespace LandscapeCanvas
{
    void ReferenceShapeNode::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ReferenceShapeNode, BaseNode>()
                ->Version(0)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ReferenceShapeNode>("ReferenceShapeNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "ShapeNodeTitlePalette")
                    ;
            }
        }
    }

    const QString ReferenceShapeNode::TITLE = QObject::tr("Shape Reference");

    ReferenceShapeNode::ReferenceShapeNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* ReferenceShapeNode::GetTitle() const
    {
        return TITLE.toUtf8().constData();
    }

    const char* ReferenceShapeNode::GetSubTitle() const
    {
        return BaseShapeNode::SHAPE_CATEGORY_TITLE.toUtf8().constData();
    }

    const BaseNode::BaseNodeType ReferenceShapeNode::GetBaseNodeType() const
    {
        return BaseNode::Shape;
    }

    void ReferenceShapeNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        GraphModel::DataTypePtr boundsDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            INBOUND_SHAPE_SLOT_ID,
            INBOUND_SHAPE_SLOT_LABEL.toUtf8().constData(),
            { boundsDataType },
            AZStd::any(AZ::EntityId()),
            INBOUND_SHAPE_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));

        RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
            BaseShapeNode::BOUNDS_SLOT_ID,
            BaseShapeNode::BOUNDS_SLOT_LABEL.toUtf8().constData(),
            boundsDataType,
            BaseShapeNode::BOUNDS_OUTPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
