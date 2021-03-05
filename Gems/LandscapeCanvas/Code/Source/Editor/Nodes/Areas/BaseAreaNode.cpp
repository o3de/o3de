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
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// Graph Model
#include <GraphModel/Integration/Helpers.h>
#include <GraphModel/Model/Slot.h>

// Vegetation
#include <Vegetation/Editor/EditorVegetationComponentTypeIds.h>

// Landscape Canvas
#include "BaseAreaNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void BaseAreaNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BaseAreaNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BaseAreaNode>("BaseAreaNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "VegetationAreaNodeTitlePalette")
                    ;
            }
        }
    }

    BaseAreaNode::BaseAreaNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
    }

    AZ::Component* BaseAreaNode::GetReferenceShapeComponent() const
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, GetVegetationEntityId());
        if (!entity)
        {
            return nullptr;
        }

        AZ::Component* component = entity->FindComponent(Vegetation::EditorReferenceShapeComponentTypeId);
        if (component)
        {
            return component;
        }

        return nullptr;
    }

    void BaseAreaNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        GraphModel::DataTypePtr invalidEntityDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::InvalidEntity);
        GraphModel::DataTypePtr boundsDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);
        GraphModel::DataTypePtr areaDataType = GraphContext::GetInstance()->GetDataType(LandscapeCanvasDataTypeEnum::Area);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            PLACEMENT_BOUNDS_SLOT_ID,
            PLACEMENT_BOUNDS_SLOT_LABEL.toUtf8().constData(),
            { boundsDataType, invalidEntityDataType },
            boundsDataType->GetDefaultValue(),
            PLACEMENT_BOUNDS_INPUT_SLOT_DESCRIPTION.toUtf8().constData()));

        RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
            OUTBOUND_AREA_SLOT_ID,
            OUTBOUND_AREA_SLOT_LABEL.toUtf8().constData(),
            areaDataType,
            OUTBOUND_AREA_OUTPUT_SLOT_DESCRIPTION.toUtf8().constData()));
    }
} // namespace LandscapeCanvas
