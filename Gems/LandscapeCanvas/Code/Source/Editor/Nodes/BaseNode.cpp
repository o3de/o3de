/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>

// Qt
#include <QString>

// Graph Model
#include <GraphModel/Model/Slot.h>

// Landscape Canvas
#include "BaseNode.h"
#include <Editor/Core/Core.h>
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void BaseNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BaseNode, GraphModel::Node>()
                ->Version(0)
                ->Field("m_vegetationEntityId", &BaseNode::m_vegetationEntityId)
                ->Field("m_componentId", &BaseNode::m_componentId)
                ;
        }
    }

    BaseNode::BaseNode(GraphModel::GraphPtr graph)
        : Node(graph)
    {
    }

    void BaseNode::PostLoadSetup(GraphModel::GraphPtr graph, GraphModel::NodeId id)
    {
        Node::PostLoadSetup(graph, id);

        // Make sure to refresh the data in the entity name slot after any load or reload in case the name changed.
        RefreshEntityName();
    }

    void BaseNode::PostLoadSetup()
    {
        Node::PostLoadSetup();

        // Make sure to refresh the data in the entity name slot after any load or reload in case the name changed.
        RefreshEntityName();
    }

    void BaseNode::CreateEntityNameSlot()
    {
        // Property field to show the name of the corresponding Vegetation Entity
        GraphModel::DataTypePtr stringDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::String);

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Property,
            ENTITY_NAME_SLOT_ID,
            ENTITY_NAME_SLOT_LABEL.toUtf8().constData(),
            ENTITY_NAME_SLOT_DESCRIPTION.toUtf8().constData(),
            GraphModel::DataTypeList{ stringDataType },
            stringDataType->GetDefaultValue()));
    }

    void BaseNode::SetVegetationEntityId(const AZ::EntityId& entityId)
    {
        m_vegetationEntityId = entityId;
        RefreshEntityName();
    }

    void BaseNode::RefreshEntityName()
    {
        // Update the Entity Name (if we have the property slot)
        GraphModel::SlotPtr slot = GetSlot(ENTITY_NAME_SLOT_ID);
        if (slot)
        {
            AZStd::string name;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                name, m_vegetationEntityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetName);
            slot->SetValue(AZStd::any(name));
        }
    }

    void BaseNode::SetComponentId(const AZ::ComponentId& componentId)
    {
        m_componentId = componentId;
    }

    AZ::ComponentDescriptor::DependencyArrayType BaseNode::GetOptionalRequiredServices() const
    {
        return {};
    }

    AZ::Component* BaseNode::GetComponent() const
    {
        AZ::Entity* gradientEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(gradientEntity, &AZ::ComponentApplicationRequests::FindEntity, GetVegetationEntityId());
        if (!gradientEntity)
        {
            return nullptr;
        }

        AZ::Component* component = gradientEntity->FindComponent(GetComponentId());
        if (component)
        {
            return component;
        }

        return nullptr;
    }

    bool BaseNode::IsAreaExtender() const
    {
        BaseNodeType baseNodeType = GetBaseNodeType();
        return baseNodeType == LandscapeCanvas::BaseNode::VegetationAreaModifier
            || baseNodeType == LandscapeCanvas::BaseNode::VegetationAreaFilter
            || baseNodeType == LandscapeCanvas::BaseNode::VegetationAreaSelector
            || baseNodeType == LandscapeCanvas::BaseNode::TerrainExtender
            || baseNodeType == LandscapeCanvas::BaseNode::TerrainSurfaceExtender
            ;
    }
} // namespace LandscapeCanvas
