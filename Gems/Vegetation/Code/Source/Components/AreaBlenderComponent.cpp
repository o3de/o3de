/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <VegetationProfiler.h>
#include "AreaBlenderComponent.h"
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/InstanceData.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/DebugNotificationBus.h>

#include <VegetationProfiler.h>

namespace Vegetation
{
    void AreaBlenderConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaBlenderConfig, AreaConfig>()
                ->Version(0)
                ->Field("InheritBehavior", &AreaBlenderConfig::m_inheritBehavior)
                ->Field("PropagateBehavior", &AreaBlenderConfig::m_propagateBehavior)
                ->Field("Operations", &AreaBlenderConfig::m_vegetationAreaIds)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<AreaBlenderConfig>(
                    "Vegetation Layer Blender", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &AreaBlenderConfig::m_inheritBehavior, "Inherit Behavior", "Allow shapes, modifiers, filters of a parent to affect this area.")
                    ->DataElement(0, &AreaBlenderConfig::m_propagateBehavior, "Propagate Behavior", "Allow shapes, modifiers, filters to affect referenced areas.")
                    ->DataElement(0, &AreaBlenderConfig::m_vegetationAreaIds, "Vegetation Areas", "Ordered list of vegetation areas.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->ElementAttribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("VegetationAreaService"));
                ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AreaBlenderConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("inheritBehavior", BehaviorValueProperty(&AreaBlenderConfig::m_inheritBehavior))
                ->Property("propogateBehavior", BehaviorValueProperty(&AreaBlenderConfig::m_propagateBehavior))
                ->Method("GetNumAreas", &AreaBlenderConfig::GetNumAreas)
                ->Method("GetAreaEntityId", &AreaBlenderConfig::GetAreaEntityId)
                ->Method("RemoveAreaEntityId", &AreaBlenderConfig::RemoveAreaEntityId)
                ->Method("AddAreaEntityId", &AreaBlenderConfig::AddAreaEntityId)
                ;
        }
    }

    size_t AreaBlenderConfig::GetNumAreas() const
    {
        return m_vegetationAreaIds.size();
    }

    AZ::EntityId AreaBlenderConfig::GetAreaEntityId(int index) const
    {
        if (index < m_vegetationAreaIds.size() && index >= 0)
        {
            return m_vegetationAreaIds[index];
        }

        return AZ::EntityId();
    }

    void AreaBlenderConfig::RemoveAreaEntityId(int index)
    {
        if (index < m_vegetationAreaIds.size() && index >= 0)
        {
            m_vegetationAreaIds.erase(m_vegetationAreaIds.begin() + index);
        }
    }

    void AreaBlenderConfig::AddAreaEntityId(AZ::EntityId entityId)
    {
        m_vegetationAreaIds.push_back(entityId);
    }

    void AreaBlenderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetProvidedServices(services);
    }

    void AreaBlenderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetIncompatibleServices(services);
    }

    void AreaBlenderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetRequiredServices(services);
    }

    void AreaBlenderComponent::Reflect(AZ::ReflectContext* context)
    {
        AreaBlenderConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaBlenderComponent, AreaComponentBase>()
                ->Version(0)
                ->Field("Configuration", &AreaBlenderComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("AreaBlenderComponentTypeId", BehaviorConstant(AreaBlenderComponentTypeId));

            behaviorContext->Class<AreaBlenderComponent>()->RequestBus("AreaBlenderRequestBus");

            behaviorContext->EBus<AreaBlenderRequestBus>("AreaBlenderRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAreaPriority", &AreaBlenderRequestBus::Events::GetAreaPriority)
                ->Event("SetAreaPriority", &AreaBlenderRequestBus::Events::SetAreaPriority)
                ->VirtualProperty("AreaPriority", "GetAreaPriority", "SetAreaPriority")
                ->Event("GetAreaLayer", &AreaBlenderRequestBus::Events::GetAreaLayer)
                ->Event("SetAreaLayer", &AreaBlenderRequestBus::Events::SetAreaLayer)
                ->VirtualProperty("AreaLayer", "GetAreaLayer", "SetAreaLayer")
                ->Event("GetAreaProductCount", &AreaBlenderRequestBus::Events::GetAreaProductCount)
                ->Event("GetInheritBehavior", &AreaBlenderRequestBus::Events::GetInheritBehavior)
                ->Event("SetInheritBehavior", &AreaBlenderRequestBus::Events::SetInheritBehavior)
                ->VirtualProperty("InheritBehavior", "GetInheritBehavior", "SetInheritBehavior")
                ->Event("GetPropagateBehavior", &AreaBlenderRequestBus::Events::GetPropagateBehavior)
                ->Event("SetPropagateBehavior", &AreaBlenderRequestBus::Events::SetPropagateBehavior)
                ->VirtualProperty("PropagateBehavior", "GetPropagateBehavior", "SetPropagateBehavior")
                ->Event("GetNumAreas", &AreaBlenderRequestBus::Events::GetNumAreas)
                ->Event("GetAreaEntityId", &AreaBlenderRequestBus::Events::GetAreaEntityId)
                ->Event("RemoveAreaEntityId", &AreaBlenderRequestBus::Events::RemoveAreaEntityId)
                ->Event("AddAreaEntityId", &AreaBlenderRequestBus::Events::AddAreaEntityId)
                ;
        }
    }

    AreaBlenderComponent::AreaBlenderComponent(const AreaBlenderConfig& configuration)
        : AreaComponentBase(configuration)
        , m_configuration(configuration)
    {
    }

    void AreaBlenderComponent::SetupDependencies()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependencies(m_configuration.m_vegetationAreaIds);
    }

    void AreaBlenderComponent::Activate()
    {
        // remove all invalid area IDs or self from the operations list
        m_configuration.m_vegetationAreaIds.erase(
            AZStd::remove_if(
                m_configuration.m_vegetationAreaIds.begin(),
                m_configuration.m_vegetationAreaIds.end(),
                [this](const auto& id) {return !id.IsValid() || id == GetEntityId();}),
            m_configuration.m_vegetationAreaIds.end());

        for (const auto& id : m_configuration.m_vegetationAreaIds)
        {
            AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::MuteArea, id);
        }

        SetupDependencies();

        AreaComponentBase::Activate(); //must activate base last to connect AreaRequestBus once everything else is setup
        AreaBlenderRequestBus::Handler::BusConnect(GetEntityId());
    }

    void AreaBlenderComponent::Deactivate()
    {
        AreaComponentBase::Deactivate(); //must deactivate base first to ensure AreaRequestBus disconnect waits for other threads

        m_dependencyMonitor.Reset();

        for (const auto& id : m_configuration.m_vegetationAreaIds)
        {
            AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::UnmuteArea, id);
        }

        AreaBlenderRequestBus::Handler::BusDisconnect();
    }

    bool AreaBlenderComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        AreaComponentBase::ReadInConfig(baseConfig);
        if (auto config = azrtti_cast<const AreaBlenderConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool AreaBlenderComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        AreaComponentBase::WriteOutConfig(outBaseConfig);
        if (auto config = azrtti_cast<AreaBlenderConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool AreaBlenderComponent::PrepareToClaim(EntityIdStack& stackIds)
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        bool result = true;

        AZ_ErrorOnce("Vegetation", !AreaRequestBus::HasReentrantEBusUseThisThread(),
            "Detected cyclic dependencies with vegetation entity references on entity '%s' (%s)", GetEntity()->GetName().c_str(),
            GetEntityId().ToString().c_str());

        if (!AreaRequestBus::HasReentrantEBusUseThisThread())
        {
            //build a "modifier stack" of contributing entity ids considering inherit and propagate flags
            EntityIdStack emptyIds;
            EntityIdStack& processedIds = m_configuration.m_inheritBehavior && m_configuration.m_propagateBehavior ? stackIds : emptyIds;
            EntityIdStackPusher stackPusher(processedIds, m_configuration.m_propagateBehavior ? GetEntityId() : AZ::EntityId());

            for (const auto& entityId : m_configuration.m_vegetationAreaIds)
            {
                if (AreaNotificationBus::GetNumOfEventHandlers(entityId) == 0) // hidden areas are deactivated -> disconnected from the bus.
                {
                    continue;
                }
                bool prepared = false;
                AreaNotificationBus::Event(entityId, &AreaNotificationBus::Events::OnAreaConnect);
                AreaRequestBus::EventResult(prepared, entityId, &AreaRequestBus::Events::PrepareToClaim, processedIds);
                AreaNotificationBus::Event(entityId, &AreaNotificationBus::Events::OnAreaDisconnect);
                if (!prepared)
                {
                    result = false;
                    break;
                }
            }
        }
        return result;
    }

    void AreaBlenderComponent::ClaimPositions(EntityIdStack& stackIds, ClaimContext& context)
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        if (context.m_availablePoints.empty())
        {
            return;
        }

        AZ_ErrorOnce(
            "Vegetation", !AreaRequestBus::HasReentrantEBusUseThisThread(),
            "Detected cyclic dependencies with vegetation entity references on entity '%s' (%s)", GetEntity()->GetName().c_str(),
            GetEntityId().ToString().c_str());

        if (!AreaRequestBus::HasReentrantEBusUseThisThread())
        {
            //build a "modifier stack" of contributing entity ids considering inherit and propagate flags
            EntityIdStack emptyIds;
            EntityIdStack& processedIds = m_configuration.m_inheritBehavior && m_configuration.m_propagateBehavior ? stackIds : emptyIds;
            EntityIdStackPusher stackPusher(processedIds, m_configuration.m_propagateBehavior ? GetEntityId() : AZ::EntityId());

            for (const auto& entityId : m_configuration.m_vegetationAreaIds)
            {
                VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FillAreaStart, entityId, AZStd::chrono::steady_clock::now()));
                if (context.m_availablePoints.empty())
                {
                    break;
                }

                AreaNotificationBus::Event(entityId, &AreaNotificationBus::Events::OnAreaConnect);
                AreaRequestBus::Event(entityId, &AreaRequestBus::Events::ClaimPositions, processedIds, context);
                AreaNotificationBus::Event(entityId, &AreaNotificationBus::Events::OnAreaDisconnect);
                VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FillAreaEnd, entityId, AZStd::chrono::steady_clock::now(), aznumeric_cast<AZ::u32>(context.m_availablePoints.size())));
            }
        }
    }

    void AreaBlenderComponent::UnclaimPosition(const ClaimHandle handle)
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        AZ_ErrorOnce(
            "Vegetation", !AreaRequestBus::HasReentrantEBusUseThisThread(),
            "Detected cyclic dependencies with vegetation entity references on entity '%s' (%s)", GetEntity()->GetName().c_str(),
            GetEntityId().ToString().c_str());

        if (!AreaRequestBus::HasReentrantEBusUseThisThread())
        {
            for (const auto& entityId : m_configuration.m_vegetationAreaIds)
            {
                AreaNotificationBus::Event(entityId, &AreaNotificationBus::Events::OnAreaConnect);
                AreaRequestBus::Event(entityId, &AreaRequestBus::Events::UnclaimPosition, handle);
                AreaNotificationBus::Event(entityId, &AreaNotificationBus::Events::OnAreaDisconnect);
            }
        }
    }

    AZ::Aabb AreaBlenderComponent::GetEncompassingAabb() const
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        AZ::Aabb bounds = AZ::Aabb::CreateNull();

        if (m_configuration.m_propagateBehavior)
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        }

        AZ_ErrorOnce(
            "Vegetation", !AreaInfoBus::HasReentrantEBusUseThisThread(),
            "Detected cyclic dependencies with vegetation entity references on entity '%s' (%s)", GetEntity()->GetName().c_str(),
            GetEntityId().ToString().c_str());

        if (!AreaInfoBus::HasReentrantEBusUseThisThread())
        {
            for (const auto& entityId : m_configuration.m_vegetationAreaIds)
            {
                if (entityId != GetEntityId())
                {
                    AZ::Aabb operationBounds = AZ::Aabb::CreateNull();
                    AreaInfoBus::EventResult(operationBounds, entityId, &AreaInfoBus::Events::GetEncompassingAabb);
                    bounds.AddAabb(operationBounds);
                }
            }
        }
        return bounds;
    }

    AZ::u32 AreaBlenderComponent::GetProductCount() const
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        AZ::u32 count = 0;

        AZ_ErrorOnce(
            "Vegetation", !AreaInfoBus::HasReentrantEBusUseThisThread(),
            "Detected cyclic dependencies with vegetation entity references on entity '%s' (%s)", GetEntity()->GetName().c_str(),
            GetEntityId().ToString().c_str());

        if (!AreaInfoBus::HasReentrantEBusUseThisThread())
        {
            for (const auto& entityId : m_configuration.m_vegetationAreaIds)
            {
                if (entityId != GetEntityId())
                {
                    AZ::u32 operationCount = 0;
                    AreaInfoBus::EventResult(operationCount, entityId, &AreaInfoBus::Events::GetProductCount);
                    count += operationCount;
                }
            }
        }
        return count;
    }

    AZ::u32 AreaBlenderComponent::GetAreaPriority() const
    {
        return m_configuration.m_priority;
    }

    void AreaBlenderComponent::SetAreaPriority(AZ::u32 priority)
    {
        m_configuration.m_priority = priority;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u32 AreaBlenderComponent::GetAreaLayer() const
    {
        return m_configuration.m_layer;
    }

    void AreaBlenderComponent::SetAreaLayer(AZ::u32 layer)
    {
        m_configuration.m_layer = layer;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u32 AreaBlenderComponent::GetAreaProductCount() const
    {
        return GetProductCount();
    }

    bool AreaBlenderComponent::GetInheritBehavior() const
    {
        return m_configuration.m_inheritBehavior;
    }

    void AreaBlenderComponent::SetInheritBehavior(bool value)
    {
        m_configuration.m_inheritBehavior = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool AreaBlenderComponent::GetPropagateBehavior() const
    {
        return m_configuration.m_propagateBehavior;
    }

    void AreaBlenderComponent::SetPropagateBehavior(bool value)
    {
        m_configuration.m_propagateBehavior = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    size_t AreaBlenderComponent::GetNumAreas() const
    {
        return m_configuration.GetNumAreas();
    }

    AZ::EntityId AreaBlenderComponent::GetAreaEntityId(int index) const
    {
        return m_configuration.GetAreaEntityId(index);
    }

    void AreaBlenderComponent::RemoveAreaEntityId(int index)
    {
        m_configuration.RemoveAreaEntityId(index);
        SetupDependencies();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void AreaBlenderComponent::AddAreaEntityId(AZ::EntityId entityId)
    {
        m_configuration.AddAreaEntityId(entityId);
        SetupDependencies();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
