/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorTerrainComponent.h"
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Terrain
{
    void EditorTerrainComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::ReflectSubClass<EditorTerrainComponent, BaseClassType>(context, 2,
            &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType,
            typename BaseClassType::WrappedConfigType,2>);
    }

    void EditorTerrainComponent::Activate()
    {
        GradientSignal::GradientPreviewContextRequestBus::Handler::BusConnect(GetEntityId());
        m_configuration.m_gradientSampler.m_ownerEntityId = GetEntityId();
        BaseClassType::Activate();
    }

    void EditorTerrainComponent::Deactivate()
    {
        GradientSignal::GradientPreviewContextRequestBus::Handler::BusDisconnect();
        BaseClassType::Activate();
    }

    AZ::u32 EditorTerrainComponent::ConfigurationChanged()
    {
        return BaseClassType::ConfigurationChanged();
    }

    AZ::EntityId EditorTerrainComponent::GetPreviewEntity() const
    {
        return GetEntityId();
    }

    AZ::Aabb EditorTerrainComponent::GetPreviewBounds() const
    {
        AZ::Vector3 position = AZ::Vector3(0.0f);

        // if a shape entity was supplied, attempt to use its shape bounds or position
        if (GetEntityId().IsValid())
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                bounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            if (bounds.IsValid())
            {
                return bounds;
            }

            AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
        }

        return AZ::Aabb::CreateCenterHalfExtents(position, AZ::Vector3(1.0f) / 2.0f);
    }

    bool EditorTerrainComponent::GetConstrainToShape() const
    {
        return false;
    }

} // namespace Terrain
