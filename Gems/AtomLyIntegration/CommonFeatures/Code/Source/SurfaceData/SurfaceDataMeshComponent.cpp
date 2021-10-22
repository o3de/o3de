/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SurfaceDataMeshComponent.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>

#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

#include <MathConversion.h>

namespace SurfaceData
{
    void SurfaceDataMeshConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataMeshConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("SurfaceTags", &SurfaceDataMeshConfig::m_tags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceDataMeshConfig>(
                    "Mesh Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceDataMeshConfig::m_tags, "Generated Tags", "")
                    ;
            }
        }
    }

    void SurfaceDataMeshComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
    }

    void SurfaceDataMeshComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
    }

    void SurfaceDataMeshComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("MeshService", 0x71d8a455));
    }

    void SurfaceDataMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceDataMeshConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataMeshComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceDataMeshComponent::m_configuration)
                ;
        }
    }

    SurfaceDataMeshComponent::SurfaceDataMeshComponent()
        : m_nonUniformScaleChangedHandler([this]([[maybe_unused]] const AZ::Vector3& scale) { this->OnCompositionChanged(); })
    {
    }

    SurfaceDataMeshComponent::SurfaceDataMeshComponent(const SurfaceDataMeshConfig& configuration)
        : m_configuration(configuration)
        , m_nonUniformScaleChangedHandler([this]([[maybe_unused]] const AZ::Vector3& scale) { this->OnCompositionChanged(); })
    {
    }

    void SurfaceDataMeshComponent::Activate()
    {
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AZ::Render::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());

        AZ::NonUniformScaleRequestBus::Event(
            GetEntityId(), &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);

        m_providerHandle = InvalidSurfaceDataRegistryHandle;
        m_refresh = false;

        // Update the cached mesh data and bounds, then register the surface data provider
        UpdateMeshData();
    }

    void SurfaceDataMeshComponent::Deactivate()
    {
        if (m_providerHandle != InvalidSurfaceDataRegistryHandle)
        {
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            m_providerHandle = InvalidSurfaceDataRegistryHandle;
        }

        m_nonUniformScaleChangedHandler.Disconnect();
        SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::Render::MeshComponentNotificationBus::Handler::BusDisconnect();
        m_refresh = false;

        // Clear the cached mesh data
        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);
            m_meshAssetData = {};
            m_meshBounds = AZ::Aabb::CreateNull();
            m_meshWorldTM = AZ::Transform::CreateIdentity();
            m_meshWorldTMInverse = AZ::Transform::CreateIdentity();
        }
    }

    bool SurfaceDataMeshComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceDataMeshConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceDataMeshComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceDataMeshConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool SurfaceDataMeshComponent::DoRayTrace(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, AZ::Vector3& outNormal) const
    {
        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        // test AABB as first pass to claim the point
        const AZ::Vector3 testPosition = AZ::Vector3(
            inPosition.GetX(),
            inPosition.GetY(),
            (m_meshBounds.GetMax().GetZ() + m_meshBounds.GetMin().GetZ()) * 0.5f);

        if (!m_meshBounds.Contains(testPosition))
        {
            return false;
        }

        AZ::RPI::ModelAsset* mesh = m_meshAssetData.GetAs<AZ::RPI::ModelAsset>();
        if (!mesh)
        {
            return false;
        }

        const AZ::Vector3 rayStart = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_meshBounds.GetMax().GetZ() + s_rayAABBHeightPadding);
        const AZ::Vector3 rayEnd = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_meshBounds.GetMin().GetZ() - s_rayAABBHeightPadding);
        return GetMeshRayIntersection(
            *mesh, m_meshWorldTM, m_meshWorldTMInverse, m_meshNonUniformScale, rayStart, rayEnd, outPosition, outNormal);
    }


    void SurfaceDataMeshComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const
    {
        AZ::Vector3 hitPosition;
        AZ::Vector3 hitNormal;
        if (DoRayTrace(inPosition, hitPosition, hitNormal))
        {
            SurfacePoint point;
            point.m_entityId = GetEntityId();
            point.m_position = hitPosition;
            point.m_normal = hitNormal;
            AddMaxValueForMasks(point.m_masks, m_configuration.m_tags, 1.0f);
            surfacePointList.push_back(point);
        }
    }

    AZ::Aabb SurfaceDataMeshComponent::GetSurfaceAabb() const
    {
        return m_meshBounds;
    }

    SurfaceTagVector SurfaceDataMeshComponent::GetSurfaceTags() const
    {
        return m_configuration.m_tags;
    }

    void SurfaceDataMeshComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void SurfaceDataMeshComponent::OnModelReady([[maybe_unused]] const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset, [[maybe_unused]] const AZ::Data::Instance<AZ::RPI::Model>& model)
    {
        OnCompositionChanged();
    }

    void SurfaceDataMeshComponent::OnTransformChanged(const AZ::Transform & local, const AZ::Transform & world)
    {
        (void)local;
        (void)world;
        OnCompositionChanged();
    }

    void SurfaceDataMeshComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_refresh)
        {
            UpdateMeshData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SurfaceDataMeshComponent::UpdateMeshData()
    {
        AZ_PROFILE_SCOPE(Entity, "SurfaceDataMeshComponent: UpdateMeshData");

        bool meshValidBeforeUpdate = false;
        bool meshValidAfterUpdate = false;

        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

            meshValidBeforeUpdate = (m_meshAssetData.GetAs<AZ::RPI::ModelAsset>() != nullptr) && (m_meshBounds.IsValid());

            m_meshAssetData = {};
            AZ::Render::MeshComponentRequestBus::EventResult(m_meshAssetData, GetEntityId(), &AZ::Render::MeshComponentRequestBus::Events::GetModelAsset);

            m_meshBounds = AZ::Aabb::CreateNull();
            AZ::Render::MeshComponentRequestBus::EventResult(m_meshBounds, GetEntityId(), &AZ::Render::MeshComponentRequestBus::Events::GetWorldBounds);

            m_meshWorldTM = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(m_meshWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            m_meshWorldTMInverse = m_meshWorldTM.GetInverse();

            m_meshNonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(m_meshNonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

            meshValidAfterUpdate = (m_meshAssetData.GetAs<AZ::RPI::ModelAsset>() != nullptr) && (m_meshBounds.IsValid());
        }

        SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = GetSurfaceAabb();
        registryEntry.m_tags = GetSurfaceTags();

        if (!meshValidBeforeUpdate && !meshValidAfterUpdate)
        {
            // We didn't have a valid mesh asset before or after running this, so do nothing.
        }
        else if (!meshValidBeforeUpdate && meshValidAfterUpdate)
        {
            // Our mesh has become valid, so register as a provider and save off the provider handle
            AZ_Assert((m_providerHandle == InvalidSurfaceDataRegistryHandle), "Surface data handle is initialized before our mesh became active");
            AZ_Assert(m_meshBounds.IsValid(), "Mesh Geometry isn't correctly initialized.");
            SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, registryEntry);

            // Start listening for surface data events
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
        }
        else if (meshValidBeforeUpdate && !meshValidAfterUpdate)
        {
            // Our mesh has stopped being valid, so unregister and stop listening for surface data events
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            m_providerHandle = InvalidSurfaceDataRegistryHandle;

            SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        }
        else if (meshValidBeforeUpdate && meshValidAfterUpdate)
        {
            // Our mesh was valid before and after, it just changed in some way, so update our registry entry.
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, registryEntry);
        }
    }
}
