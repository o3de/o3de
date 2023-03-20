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
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <GraphModel/Integration/Helpers.h>
#include <GraphModel/Model/Slot.h>

// Landscape Canvas
#include "BaseAreaNode.h"
#include <Editor/Core/GraphContext.h>

#include <LmbrCentral/Shape/ReferenceShapeComponentBus.h>

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

        AZ::Component* component = entity->FindComponent(LmbrCentral::EditorReferenceShapeComponentTypeId);
        if (component)
        {
            return component;
        }

        return nullptr;
    }

    void BaseAreaNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        GraphModel::DataTypePtr boundsDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);
        GraphModel::DataTypePtr areaDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Area);

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Data,
            PLACEMENT_BOUNDS_SLOT_ID,
            PLACEMENT_BOUNDS_SLOT_LABEL.toUtf8().constData(),
            PLACEMENT_BOUNDS_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ boundsDataType },
            AZStd::any(AZ::EntityId())));

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Output,
            GraphModel::SlotType::Data,
            OUTBOUND_AREA_SLOT_ID,
            OUTBOUND_AREA_SLOT_LABEL.toUtf8().constData(),
            OUTBOUND_AREA_OUTPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ areaDataType }));
    }
} // namespace LandscapeCanvas
