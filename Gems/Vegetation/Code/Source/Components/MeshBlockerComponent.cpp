/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MeshBlockerComponent.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/InstanceData.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace Vegetation
{
    void MeshBlockerConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<MeshBlockerConfig, AreaConfig>()
                ->Version(2)
                ->Field("InheritBehavior", &MeshBlockerConfig::m_inheritBehavior)
                ->Field("MeshHeightPercentMin", &MeshBlockerConfig::m_meshHeightPercentMin)
                ->Field("MeshHeightPercentMax", &MeshBlockerConfig::m_meshHeightPercentMax)
                ->Field("BlockWhenInvisible", &MeshBlockerConfig::m_blockWhenInvisible)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<MeshBlockerConfig>(
                    "Vegetation Layer Blocker (Mesh)", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &MeshBlockerConfig::m_inheritBehavior, "Inherit Behavior", "Allow shapes, modifiers, filters of a parent to affect this area.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshBlockerConfig::m_meshHeightPercentMin, "Mesh Height Percent Min", "The percentage of the mesh height (from the bottom up) used as the lower bound for intersection tests")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshBlockerConfig::m_meshHeightPercentMax, "Mesh Height Percent Max", "The percentage of the mesh height (from the bottom up) used as the upper bound for intersection tests")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &MeshBlockerConfig::m_blockWhenInvisible, "Block When Invisible", "Continue to block vegetation even if the mesh is invisible.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MeshBlockerConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("inheritBehavior", BehaviorValueProperty(&MeshBlockerConfig::m_inheritBehavior))
                ->Property("meshHeightPercentMin", BehaviorValueProperty(&MeshBlockerConfig::m_meshHeightPercentMin))
                ->Property("meshHeightPercentMax", BehaviorValueProperty(&MeshBlockerConfig::m_meshHeightPercentMax))
                ->Property("blockWhenInvisible", BehaviorValueProperty(&MeshBlockerConfig::m_blockWhenInvisible))
                ;
        }
    }

    void MeshBlockerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetProvidedServices(services);
    }

    void MeshBlockerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetIncompatibleServices(services);
        services.push_back(AZ_CRC("VegetationModifierService", 0xc551fca6));
    }

    void MeshBlockerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetRequiredServices(services);
        services.push_back(AZ_CRC("MeshService", 0x71d8a455));
    }

    void MeshBlockerComponent::Reflect(AZ::ReflectContext* context)
    {
        MeshBlockerConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<MeshBlockerComponent, AreaComponentBase>()
                ->Version(0)
                ->Field("Configuration", &MeshBlockerComponent::m_configuration)
                ;
        }


        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("MeshBlockerComponentTypeId", BehaviorConstant(MeshBlockerComponentTypeId));

            behaviorContext->Class<MeshBlockerComponent>()->RequestBus("MeshBlockerRequestBus");

            behaviorContext->EBus<MeshBlockerRequestBus>("MeshBlockerRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAreaPriority", &MeshBlockerRequestBus::Events::GetAreaPriority)
                ->Event("SetAreaPriority", &MeshBlockerRequestBus::Events::SetAreaPriority)
                ->VirtualProperty("AreaPriority", "GetAreaPriority", "SetAreaPriority")
                ->Event("GetAreaLayer", &MeshBlockerRequestBus::Events::GetAreaLayer)
                ->Event("SetAreaLayer", &MeshBlockerRequestBus::Events::SetAreaLayer)
                ->VirtualProperty("AreaLayer", "GetAreaLayer", "SetAreaLayer")
                ->Event("GetAreaProductCount", &MeshBlockerRequestBus::Events::GetAreaProductCount)
                ->Event("GetInheritBehavior", &MeshBlockerRequestBus::Events::GetInheritBehavior)
                ->Event("SetInheritBehavior", &MeshBlockerRequestBus::Events::SetInheritBehavior)
                ->VirtualProperty("InheritBehavior", "GetInheritBehavior", "SetInheritBehavior")
                ->Event("GetMeshHeightPercentMin", &MeshBlockerRequestBus::Events::GetMeshHeightPercentMin)
                ->Event("SetMeshHeightPercentMin", &MeshBlockerRequestBus::Events::SetMeshHeightPercentMin)
                ->VirtualProperty("MeshHeightPercentMin", "GetMeshHeightPercentMin", "SetMeshHeightPercentMin")
                ->Event("GetMeshHeightPercentMax", &MeshBlockerRequestBus::Events::GetMeshHeightPercentMax)
                ->Event("SetMeshHeightPercentMax", &MeshBlockerRequestBus::Events::SetMeshHeightPercentMax)
                ->VirtualProperty("MeshHeightPercentMax", "GetMeshHeightPercentMax", "SetMeshHeightPercentMax")
                ->Event("GetBlockWhenInvisible", &MeshBlockerRequestBus::Events::GetBlockWhenInvisible)
                ->Event("SetBlockWhenInvisible", &MeshBlockerRequestBus::Events::SetBlockWhenInvisible)
                ->VirtualProperty("BlockWhenInvisible", "GetBlockWhenInvisible", "SetBlockWhenInvisible")
                ;
        }
    }

    MeshBlockerComponent::MeshBlockerComponent()
        : AreaComponentBase()
        , m_nonUniformScaleChangedHandler([this]([[maybe_unused]] const AZ::Vector3& scale) { this->OnCompositionChanged(); })
    {
    }

    MeshBlockerComponent::MeshBlockerComponent(const MeshBlockerConfig& configuration)
        : AreaComponentBase(configuration)
        , m_configuration(configuration)
        , m_nonUniformScaleChangedHandler([this]([[maybe_unused]] const AZ::Vector3& scale) { this->OnCompositionChanged(); })
    {
    }

    void MeshBlockerComponent::Activate()
    {
        AZ::Render::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());

        AZ::NonUniformScaleRequestBus::Event(
            GetEntityId(), &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);

        UpdateMeshData();
        m_refresh = false;

        MeshBlockerRequestBus::Handler::BusConnect(GetEntityId());

        SurfaceData::SurfaceDataSystemNotificationBus::Handler::BusConnect();

        AreaComponentBase::Activate(); //must activate base last to connect AreaRequestBus once everything else is setup
    }

    void MeshBlockerComponent::Deactivate()
    {
        AreaComponentBase::Deactivate(); //must deactivate base first to ensure AreaRequestBus disconnect waits for other threads

        m_nonUniformScaleChangedHandler.Disconnect();
        SurfaceData::SurfaceDataSystemNotificationBus::Handler::BusDisconnect();

        m_refresh = false;
        AZ::TickBus::Handler::BusDisconnect();
        AZ::Render::MeshComponentNotificationBus::Handler::BusDisconnect();
        MeshBlockerRequestBus::Handler::BusDisconnect();
    }

    bool MeshBlockerComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        AreaComponentBase::ReadInConfig(baseConfig);
        if (auto config = azrtti_cast<const MeshBlockerConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool MeshBlockerComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        AreaComponentBase::WriteOutConfig(outBaseConfig);
        if (auto config = azrtti_cast<MeshBlockerConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool MeshBlockerComponent::PrepareToClaim([[maybe_unused]] EntityIdStack& stackIds)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);

        if (!m_meshAssetData.GetId().IsValid())
        {
            return false;
        }

        AZ::RPI::ModelAsset* mesh = m_meshAssetData.GetAs<AZ::RPI::ModelAsset>();
        if (!mesh)
        {
            return false;
        }

        return m_meshBoundsForIntersection.IsValid() && (m_meshVisible || m_configuration.m_blockWhenInvisible);
    }

    bool MeshBlockerComponent::ClaimPosition(EntityIdStack& processedIds, const ClaimPoint& point, InstanceData& instanceData)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);

        // If we've previously looked up this point for collision, reuse the results.
        // Note that this performs lookups based on ClaimPoint handles, so the cache needs to be invalidated on anything
        // that can cause the handles to change.  (See OnSurfaceChanged below)
        auto itCachedRayHit = m_cachedRayHits.find(point.m_handle);
        if (itCachedRayHit != m_cachedRayHits.end())
        {
            return itCachedRayHit->second;
        }

        // test AABB as first pass to claim the point
        if (!m_meshBoundsForIntersection.Contains(point.m_position))
        {
            return false;
        }


        AZ::RPI::ModelAsset* mesh = m_meshAssetData.GetAs<AZ::RPI::ModelAsset>();
        if (!mesh)
        {
            return false;
        }

        // test shape bus as first pass to claim the point
        bool isInsideShape = true;
        for (const auto& id : processedIds)
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(isInsideShape, id, &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point.m_position);
            if (!isInsideShape)
            {
                m_cachedRayHits[point.m_handle] = false;
                return false;
            }
        }

        //determine if an instance can be created using the generated details
        for (const auto& id : processedIds)
        {
            bool accepted = true;
            FilterRequestBus::EnumerateHandlersId(id, [&instanceData, &accepted](FilterRequestBus::Events* handler) {
                accepted = handler->Evaluate(instanceData);
                return accepted;
            });
            if (!accepted)
            {
                m_cachedRayHits[point.m_handle] = false;
                return false;
            }
        }

        AZ::Vector3 outPosition;
        AZ::Vector3 outNormal;
        const AZ::Vector3 rayStart(point.m_position.GetX(), point.m_position.GetY(), m_meshBoundsForIntersection.GetMax().GetZ());
        const AZ::Vector3 rayEnd(point.m_position.GetX(), point.m_position.GetY(), m_meshBoundsForIntersection.GetMin().GetZ());
        bool intersected = SurfaceData::GetMeshRayIntersection(
            *mesh, m_meshWorldTM, m_meshWorldTMInverse, m_meshNonUniformScale, rayStart, rayEnd, outPosition, outNormal) &&
            m_meshBoundsForIntersection.Contains(outPosition);
        m_cachedRayHits[point.m_handle] = intersected;
        return intersected;
    }

    void MeshBlockerComponent::ClaimPositions(EntityIdStack& stackIds, ClaimContext& context)
    {
        AZ_PROFILE_FUNCTION(Entity);

        //adding entity id to the stack of entity ids affecting vegetation
        EntityIdStack emptyIds;
        //when the inherit flag is disabled, as opposed to always inheriting, the stack must be cleared but preserved so redirecting to an empty stack to avoid copying
        EntityIdStack& processedIds = m_configuration.m_inheritBehavior ? stackIds : emptyIds;
        //adding current entity id to be processed uniformly
        EntityIdStackPusher stackPusher(processedIds, GetEntityId());

        InstanceData instanceData;
        instanceData.m_id = GetEntityId();
        instanceData.m_changeIndex = GetChangeIndex();

        size_t numAvailablePoints = context.m_availablePoints.size();
        for (size_t pointIndex = 0; pointIndex < numAvailablePoints; )
        {
            ClaimPoint& point = context.m_availablePoints[pointIndex];

            //generate details for a single vegetation instance
            instanceData.m_position = point.m_position;
            instanceData.m_normal = point.m_normal;
            instanceData.m_masks = point.m_masks;

            if (ClaimPosition(processedIds, point, instanceData))
            {
                context.m_createdCallback(point, instanceData);

                //Swap an available point from the end of the list
                AZStd::swap(point, context.m_availablePoints.at(numAvailablePoints - 1));
                --numAvailablePoints;
                continue;
            }

            ++pointIndex;
        }
        //resize to remove all used points
        context.m_availablePoints.resize(numAvailablePoints);
    }

    void MeshBlockerComponent::UnclaimPosition([[maybe_unused]] const ClaimHandle handle)
    {
    }

    AZ::Aabb MeshBlockerComponent::GetEncompassingAabb() const
    {
        AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
        return m_meshBounds;
    }

    AZ::u32 MeshBlockerComponent::GetProductCount() const
    {
        return 0;
    }

    void MeshBlockerComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void MeshBlockerComponent::OnSurfaceChanged(const AZ::EntityId& /*entityId*/, const AZ::Aabb& /*oldBounds*/, const AZ::Aabb& /*newBounds*/)
    {
        // If our surfaces have changed, we will need to refresh our cache.  
        // Our cache performs lookups based on ClaimPoint handles, but the list of handles can potentially change
        // from any type of surface change anywhere, so refresh even if the area doesn't overlap.
        OnCompositionChanged();
    }

    void MeshBlockerComponent::OnModelReady([[maybe_unused]] const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset, [[maybe_unused]]const AZ::Data::Instance<AZ::RPI::Model>& model)
    {
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void MeshBlockerComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_refresh)
        {
            UpdateMeshData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void MeshBlockerComponent::UpdateMeshData()
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);

        m_cachedRayHits.clear();

        m_meshAssetData = {};
        AZ::Render::MeshComponentRequestBus::EventResult(m_meshAssetData, GetEntityId(), &AZ::Render::MeshComponentRequests::GetModelAsset);

        m_meshBounds = AZ::Aabb::CreateNull();
        AZ::Render::MeshComponentRequestBus::EventResult(m_meshBounds, GetEntityId(), &AZ::Render::MeshComponentRequestBus::Events::GetWorldBounds);
        m_meshBoundsForIntersection = m_meshBounds;
        if (m_meshBoundsForIntersection.IsValid())
        {
            const auto heights = AZStd::minmax(
                m_meshBoundsForIntersection.GetMin().GetZ() + m_meshBoundsForIntersection.GetExtents().GetZ() * m_configuration.m_meshHeightPercentMin,
                m_meshBoundsForIntersection.GetMin().GetZ() + m_meshBoundsForIntersection.GetExtents().GetZ() * m_configuration.m_meshHeightPercentMax);

            AZ::Vector3 cornerMin = m_meshBoundsForIntersection.GetMin();
            cornerMin.SetZ(heights.first - s_rayAABBHeightPadding);

            AZ::Vector3 cornerMax = m_meshBoundsForIntersection.GetMax();
            cornerMax.SetZ(heights.second + s_rayAABBHeightPadding);

            m_meshBoundsForIntersection.Set(cornerMin, cornerMax);
        }

        m_meshVisible = false;
        AZ::Render::MeshComponentRequestBus::EventResult(m_meshVisible, GetEntityId(), &AZ::Render::MeshComponentRequests::GetVisibility);

        m_meshWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_meshWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        m_meshWorldTMInverse = m_meshWorldTM.GetInverse();

        m_meshNonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_meshNonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        AreaComponentBase::OnCompositionChanged();
    }

    AZ::u32 MeshBlockerComponent::GetAreaPriority() const
    {
        return m_configuration.m_priority;
    }

    void MeshBlockerComponent::SetAreaPriority(AZ::u32 priority)
    {
        m_configuration.m_priority = priority;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u32 MeshBlockerComponent::GetAreaLayer() const
    {
        return m_configuration.m_layer;
    }

    void MeshBlockerComponent::SetAreaLayer(AZ::u32 layer)
    {
        m_configuration.m_layer = layer;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u32 MeshBlockerComponent::GetAreaProductCount() const
    {
        return GetProductCount();
    }

    bool MeshBlockerComponent::GetInheritBehavior() const
    {
        return m_configuration.m_inheritBehavior;
    }

    void MeshBlockerComponent::SetInheritBehavior(bool value)
    {
        m_configuration.m_inheritBehavior = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float MeshBlockerComponent::GetMeshHeightPercentMin() const
    {
        return m_configuration.m_meshHeightPercentMax;
    }

    void MeshBlockerComponent::SetMeshHeightPercentMin(float meshHeightPercentMin)
    {
        m_configuration.m_meshHeightPercentMax = meshHeightPercentMin;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float MeshBlockerComponent::GetMeshHeightPercentMax() const
    {
        return m_configuration.m_meshHeightPercentMax;
    }

    void MeshBlockerComponent::SetMeshHeightPercentMax(float meshHeightPercentMax)
    {
        m_configuration.m_meshHeightPercentMax = meshHeightPercentMax;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool MeshBlockerComponent::GetBlockWhenInvisible() const
    {
        return m_configuration.m_blockWhenInvisible;
    }

    void MeshBlockerComponent::SetBlockWhenInvisible(bool value)
    {
        m_configuration.m_blockWhenInvisible = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
