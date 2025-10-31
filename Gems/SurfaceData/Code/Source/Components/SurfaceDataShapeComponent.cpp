/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SurfaceData/Components/SurfaceDataShapeComponent.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <SurfaceDataProfiler.h>

namespace SurfaceData
{
    void SurfaceDataShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataShapeConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ProviderTags", &SurfaceDataShapeConfig::m_providerTags)
                ->Field("ModifierTags", &SurfaceDataShapeConfig::m_modifierTags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceDataShapeConfig>(
                    "Shape Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceDataShapeConfig::m_providerTags, "Generated Tags", "Surface tags to add to created points")
                    ->DataElement(0, &SurfaceDataShapeConfig::m_modifierTags, "Extended Tags", "Surface tags to add to contained points")
                    ;
            }
        }
    }

    void SurfaceDataShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SurfaceDataProviderService"));
        services.push_back(AZ_CRC_CE("SurfaceDataModifierService"));
    }

    void SurfaceDataShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SurfaceDataProviderService"));
        services.push_back(AZ_CRC_CE("SurfaceDataModifierService"));
    }

    void SurfaceDataShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("ShapeService"));
    }

    void SurfaceDataShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceDataShapeConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataShapeComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceDataShapeComponent::m_configuration)
                ;
        }
    }

    SurfaceDataShapeComponent::SurfaceDataShapeComponent(const SurfaceDataShapeConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceDataShapeComponent::Activate()
    {
        m_providerHandle = InvalidSurfaceDataRegistryHandle;
        m_modifierHandle = InvalidSurfaceDataRegistryHandle;
        m_refresh = false;

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());

        // Update the cached shape data and bounds, then register the surface data provider / modifier
        m_newPointWeights.AssignSurfaceTagWeights(m_configuration.m_providerTags, 1.0f);
        UpdateShapeData();
    }

    void SurfaceDataShapeComponent::Deactivate()
    {
        if (m_providerHandle != InvalidSurfaceDataRegistryHandle)
        {
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataProvider(m_providerHandle);
            m_providerHandle = InvalidSurfaceDataRegistryHandle;
        }
        if (m_modifierHandle != InvalidSurfaceDataRegistryHandle)
        {
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataModifier(m_modifierHandle);
            m_modifierHandle = InvalidSurfaceDataRegistryHandle;

        }

        m_refresh = false;
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        SurfaceDataModifierRequestBus::Handler::BusDisconnect();

        // Clear the cached shape data
        {
            AZStd::unique_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);
            m_shapeBounds = AZ::Aabb::CreateNull();
            m_shapeBoundsIsValid = false;
        }
    }

    bool SurfaceDataShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceDataShapeConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceDataShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceDataShapeConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void SurfaceDataShapeComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const
    {
        GetSurfacePointsFromList(AZStd::span<const AZ::Vector3>(&inPosition, 1), surfacePointList);
    }

    void SurfaceDataShapeComponent::GetSurfacePointsFromList(
        AZStd::span<const AZ::Vector3> inPositions, SurfacePointList& surfacePointList) const
    {
        SURFACE_DATA_PROFILE_FUNCTION_VERBOSE

        AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);

        if (!m_shapeBoundsIsValid)
        {
            return;
        }

        LmbrCentral::ShapeComponentRequestsBus::Event(
            GetEntityId(),
            [this, inPositions, &surfacePointList](LmbrCentral::ShapeComponentRequestsBus::Events* shape)
            {
                const AZ::Vector3 rayDirection = -AZ::Vector3::CreateAxisZ();

                // Shapes don't currently have a way to query normals at a point intersection, so we'll just return a Z-up normal
                // until they get support for it.
                const AZ::Vector3 surfacePointNormal = AZ::Vector3::CreateAxisZ();

                for (auto& inPosition : inPositions)
                {
                    if (SurfaceData::AabbContains2D(m_shapeBounds, inPosition))
                    {
                        const AZ::Vector3 rayOrigin = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_shapeBounds.GetMax().GetZ());
                        float intersectionDistance = 0.0f;
                        bool hitShape = shape->IntersectRay(rayOrigin, rayDirection, intersectionDistance);
                        if (hitShape)
                        {
                            AZ::Vector3 position = rayOrigin + intersectionDistance * rayDirection;
                            surfacePointList.AddSurfacePoint(GetEntityId(), inPosition, position, surfacePointNormal, m_newPointWeights);
                        }
                    }
                }
            });
    }


    void SurfaceDataShapeComponent::ModifySurfacePoints(
        AZStd::span<const AZ::Vector3> positions,
        AZStd::span<const AZ::EntityId> creatorEntityIds,
        AZStd::span<SurfaceData::SurfaceTagWeights> weights) const
    {
        SURFACE_DATA_PROFILE_FUNCTION_VERBOSE

        AZ_Assert(
            (positions.size() == creatorEntityIds.size()) && (positions.size() == weights.size()),
            "Sizes of the passed-in spans don't match");

        AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);

        if (m_shapeBoundsIsValid && !m_configuration.m_modifierTags.empty())
        {
            const AZ::EntityId entityId = GetEntityId();
            LmbrCentral::ShapeComponentRequestsBus::Event(
                entityId,
                [entityId, this, positions, creatorEntityIds, &weights](LmbrCentral::ShapeComponentRequestsBus::Events* shape)
                {
                    for (size_t index = 0; index < positions.size(); index++)
                    {
                        // Don't bother modifying points that this component created.
                        if (creatorEntityIds[index] == entityId)
                        {
                            continue;
                        }

                        if (m_shapeBounds.Contains(positions[index]) && shape->IsPointInside(positions[index]))
                        {
                            // If the point is inside our shape, add all our modifier tags with a weight of 1.0f.
                            weights[index].AddSurfaceTagWeights(m_configuration.m_modifierTags, 1.0f);
                        }
                    }
                });
        }
    }

    void SurfaceDataShapeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        OnCompositionChanged();
    }

    void SurfaceDataShapeComponent::OnShapeChanged([[maybe_unused]] ShapeChangeReasons changeReason)
    {
        OnCompositionChanged();
    }

    void SurfaceDataShapeComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_refresh)
        {
            UpdateShapeData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SurfaceDataShapeComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void SurfaceDataShapeComponent::UpdateShapeData()
    {
        AZ_PROFILE_FUNCTION(SurfaceData);

        bool shapeValidBeforeUpdate = false;
        bool shapeValidAfterUpdate = false;

        {
            AZStd::unique_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);

            shapeValidBeforeUpdate = m_shapeBoundsIsValid;

            m_shapeBounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(m_shapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            m_shapeBoundsIsValid = m_shapeBounds.IsValid();

            shapeValidAfterUpdate = m_shapeBoundsIsValid;
        }

        SurfaceDataRegistryEntry providerRegistryEntry;
        providerRegistryEntry.m_entityId = GetEntityId();
        providerRegistryEntry.m_bounds = m_shapeBounds;
        providerRegistryEntry.m_tags = m_configuration.m_providerTags;
        providerRegistryEntry.m_maxPointsCreatedPerInput = 1;

        SurfaceDataRegistryEntry modifierRegistryEntry(providerRegistryEntry);
        modifierRegistryEntry.m_tags = m_configuration.m_modifierTags;
        modifierRegistryEntry.m_maxPointsCreatedPerInput = 0;

        if (shapeValidBeforeUpdate && shapeValidAfterUpdate)
        {
            // Our shape was valid before and after, it just changed in some way, so update our registry entries
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != InvalidSurfaceDataRegistryHandle), "Invalid modifier data handle");
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UpdateSurfaceDataProvider(m_providerHandle, providerRegistryEntry);
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UpdateSurfaceDataModifier(m_modifierHandle, modifierRegistryEntry);
        }
        else if (!shapeValidBeforeUpdate && shapeValidAfterUpdate)
        {
            // Our shape has become valid, so register as a provider and save off the registry handles
            AZ_Assert((m_providerHandle == InvalidSurfaceDataRegistryHandle), "Surface Provider data handle is initialized before our shape became valid");
            AZ_Assert((m_modifierHandle == InvalidSurfaceDataRegistryHandle), "Surface Modifier data handle is initialized before our shape became valid");
            m_providerHandle = AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->RegisterSurfaceDataProvider(providerRegistryEntry);
            m_modifierHandle = AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->RegisterSurfaceDataModifier(modifierRegistryEntry);

            // Start listening for surface data events
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
            SurfaceDataModifierRequestBus::Handler::BusConnect(m_modifierHandle);
        }
        else if (shapeValidBeforeUpdate && !shapeValidAfterUpdate)
        {
            // Our shape has stopped being valid, so unregister and stop listening for surface data events
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataProvider(m_providerHandle);
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataModifier(m_modifierHandle);
            m_providerHandle = InvalidSurfaceDataRegistryHandle;
            m_modifierHandle = InvalidSurfaceDataRegistryHandle;

            SurfaceDataProviderRequestBus::Handler::BusDisconnect();
            SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        }
        else
        {
            // We didn't have a valid shape before or after running this, so do nothing.
        }
    }

    const float SurfaceDataShapeComponent::s_rayAABBHeightPadding = 0.1f;
}
