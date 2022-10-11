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

// Graph Model
#include <GraphModel/Integration/Helpers.h>
#include <GraphModel/Model/Slot.h>

// Landscape Canvas
#include <Editor/Core/GraphContext.h>
#include <Editor/Nodes/Terrain/TerrainSurfaceGradientListNode.h>

namespace LandscapeCanvas
{
    void TerrainSurfaceGradientListNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TerrainSurfaceGradientListNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TerrainSurfaceGradientListNode>("TerrainSurfaceGradientListNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "TerrainNodeTitlePalette")
                    ;
            }
        }
    }

    const char* TerrainSurfaceGradientListNode::TITLE = "Terrain Surface Gradient List";

    TerrainSurfaceGradientListNode::TerrainSurfaceGradientListNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const BaseNode::BaseNodeType TerrainSurfaceGradientListNode::GetBaseNodeType() const
    {
        return BaseNode::TerrainSurfaceExtender;
    }

    const char* TerrainSurfaceGradientListNode::GetTitle() const
    {
        return TITLE;
    }

    void TerrainSurfaceGradientListNode::RegisterSlots()
    {
        GraphModel::DataTypePtr gradientDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);

        GraphModel::ExtendableSlotConfiguration slotConfig;
        slotConfig.m_addButtonLabel = "Add Gradient";
        slotConfig.m_addButtonTooltip = "Add a gradient surface provider";
        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            INBOUND_GRADIENT_SLOT_ID,
            INBOUND_GRADIENT_SLOT_LABEL.toUtf8().constData(),
            { gradientDataType },
            AZStd::any(AZ::EntityId()),
            INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            &slotConfig));
    }
} // namespace LandscapeCanvas
