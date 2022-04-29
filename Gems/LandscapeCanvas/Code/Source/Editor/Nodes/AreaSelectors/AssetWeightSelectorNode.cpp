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
#include "AssetWeightSelectorNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void AssetWeightSelectorNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AssetWeightSelectorNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AssetWeightSelectorNode>("AssetWeightSelectorNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "VegetationAreaNodeTitlePalette")
                    ;
            }
        }
    }

    const QString AssetWeightSelectorNode::TITLE = QObject::tr("Vegetation Asset Weight Selector");

    AssetWeightSelectorNode::AssetWeightSelectorNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const BaseNode::BaseNodeType AssetWeightSelectorNode::GetBaseNodeType() const
    {
        return BaseNode::VegetationAreaSelector;
    }

    const bool AssetWeightSelectorNode::ShouldShowEntityName() const
    {
        // Don't show the entity name for Area Selectors since they will
        // be wrapped on a Vegetation Area it would be redundant
        return false;
    }

    const char* AssetWeightSelectorNode::GetTitle() const
    {
        return TITLE.toUtf8().constData();
    }

    void AssetWeightSelectorNode::RegisterSlots()
    {
        GraphModel::DataTypePtr invalidEntityDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::InvalidEntity);
        GraphModel::DataTypePtr gradientDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            INBOUND_GRADIENT_SLOT_ID,
            INBOUND_GRADIENT_SLOT_LABEL.toUtf8().constData(),
            { gradientDataType, invalidEntityDataType },
            gradientDataType->GetDefaultValue(),
            INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
