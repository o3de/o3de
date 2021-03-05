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
        GraphModel::DataTypePtr invalidEntityDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::InvalidEntity);
        GraphModel::DataTypePtr gradientDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            INBOUND_GRADIENT_SLOT_ID,
            INBOUND_GRADIENT_SLOT_LABEL.toUtf8().constData(),
            { gradientDataType, invalidEntityDataType },
            gradientDataType->GetDefaultValue(),
            INBOUND_GRADIENT_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
