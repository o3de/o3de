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
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <GraphModel/Integration/Helpers.h>
#include <GraphModel/Model/Slot.h>

// Landscape Canvas
#include <Editor/Core/GraphContext.h>
#include <Editor/Nodes/Terrain/TerrainHeightGradientListNode.h>

namespace LandscapeCanvas
{
    void TerrainHeightGradientListNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TerrainHeightGradientListNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TerrainHeightGradientListNode>("TerrainHeightGradientListNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "TerrainNodeTitlePalette")
                    ;
            }
        }
    }

    const char* TerrainHeightGradientListNode::TITLE = "Terrain Height Gradient List";

    TerrainHeightGradientListNode::TerrainHeightGradientListNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const BaseNode::BaseNodeType TerrainHeightGradientListNode::GetBaseNodeType() const
    {
        return BaseNode::TerrainExtender;
    }

    const char* TerrainHeightGradientListNode::GetTitle() const
    {
        return TITLE;
    }

    void TerrainHeightGradientListNode::RegisterSlots()
    {
        GraphModel::DataTypePtr gradientDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Data,
            INBOUND_GRADIENT_SLOT_ID,
            INBOUND_GRADIENT_SLOT_LABEL.toUtf8().constData(),
            INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ gradientDataType },
            AZStd::any(AZ::EntityId()),
            1,
            100,
            "Add Gradient",
            "Add a gradient height provider"));
    }
} // namespace LandscapeCanvas
