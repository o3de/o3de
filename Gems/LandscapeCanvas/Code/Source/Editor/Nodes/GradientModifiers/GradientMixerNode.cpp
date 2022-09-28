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
#include "GradientMixerNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void GradientMixerNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GradientMixerNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GradientMixerNode>("GradientMixerNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "GradientModifierNodeTitlePalette")
                    ;
            }
        }
    }

    const char* GradientMixerNode::TITLE = "Gradient Mixer";

    GradientMixerNode::GradientMixerNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* GradientMixerNode::GetTitle() const
    {
        return TITLE;
    }

    const char* GradientMixerNode::GetSubTitle() const
    {
        return GRADIENT_MODIFIER_TITLE;
    }

    const BaseNode::BaseNodeType GradientMixerNode::GetBaseNodeType() const
    {
        return BaseNode::GradientModifier;
    }

    void GradientMixerNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        GraphModel::DataTypePtr boundsDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);
        GraphModel::DataTypePtr gradientDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            PREVIEW_BOUNDS_SLOT_ID,
            PREVIEW_BOUNDS_SLOT_LABEL.toUtf8().constData(),
            { boundsDataType },
            AZStd::any(AZ::EntityId()),
            PREVIEW_BOUNDS_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));

        GraphModel::ExtendableSlotConfiguration slotConfig;
        slotConfig.m_addButtonLabel = "Add Gradient";
        slotConfig.m_addButtonTooltip = "Add a gradient layer to the mixer";
        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            INBOUND_GRADIENT_SLOT_ID,
            INBOUND_GRADIENT_SLOT_LABEL.toUtf8().constData(),
            { gradientDataType },
            AZStd::any(AZ::EntityId()),
            INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            &slotConfig));

        RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
            OUTBOUND_GRADIENT_SLOT_ID,
            OUTBOUND_GRADIENT_SLOT_LABEL.toUtf8().constData(),
            gradientDataType,
            OUTBOUND_GRADIENT_OUTPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
