/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BlockerComponent.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Util.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/InstanceData.h>

#include <VegetationProfiler.h>

namespace Vegetation
{
    namespace BlockerUtil
    {
        static bool UpdateVersion([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 1)
            {
                classElement.RemoveElementByName(AZ_CRC_CE("UseRelativeUVW"));
            }
            return true;
        }
    }

    void BlockerConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BlockerConfig, AreaConfig>()
                ->Version(1, &BlockerUtil::UpdateVersion)
                ->Field("InheritBehavior", &BlockerConfig::m_inheritBehavior)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<BlockerConfig>(
                    "Vegetation Layer Blocker", "Vegetation blocker")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &BlockerConfig::m_inheritBehavior, "Inherit Behavior", "Allow shapes, modifiers, filters of a parent to affect this area.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<BlockerConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("inheritBehavior", BehaviorValueProperty(&BlockerConfig::m_inheritBehavior))
                ;
        }
    }

    void BlockerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetProvidedServices(services);
    }

    void BlockerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetIncompatibleServices(services);
        services.push_back(AZ_CRC_CE("VegetationModifierService"));
    }

    void BlockerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("ShapeService"));
    }

    void BlockerComponent::Reflect(AZ::ReflectContext* context)
    {
        BlockerConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BlockerComponent, AreaComponentBase>()
                ->Version(0)
                ->Field("Configuration", &BlockerComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("BlockerComponentTypeId", BehaviorConstant(BlockerComponentTypeId));

            behaviorContext->Class<BlockerComponent>()->RequestBus("BlockerRequestBus");

            behaviorContext->EBus<BlockerRequestBus>("BlockerRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAreaPriority", &BlockerRequestBus::Events::GetAreaPriority)
                ->Event("SetAreaPriority", &BlockerRequestBus::Events::SetAreaPriority)
                ->VirtualProperty("AreaPriority", "GetAreaPriority", "SetAreaPriority")
                ->Event("GetAreaLayer", &BlockerRequestBus::Events::GetAreaLayer)
                ->Event("SetAreaLayer", &BlockerRequestBus::Events::SetAreaLayer)
                ->VirtualProperty("AreaLayer", "GetAreaLayer", "SetAreaLayer")
                ->Event("GetAreaProductCount", &BlockerRequestBus::Events::GetAreaProductCount)
                ->Event("GetInheritBehavior", &BlockerRequestBus::Events::GetInheritBehavior)
                ->Event("SetInheritBehavior", &BlockerRequestBus::Events::SetInheritBehavior)
                ->VirtualProperty("InheritBehavior", "GetInheritBehavior", "SetInheritBehavior")
                ;
        }
    }

    BlockerComponent::BlockerComponent(const BlockerConfig& configuration)
        : AreaComponentBase(configuration)
        , m_configuration(configuration)
    {
    }

    void BlockerComponent::Activate()
    {
        BlockerRequestBus::Handler::BusConnect(GetEntityId());
        AreaComponentBase::Activate(); //must activate base last to connect AreaRequestBus once everything else is setup

#if VEG_BLOCKER_ENABLE_CACHING
        AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
        m_claimCacheMapping.clear();
#endif
    }

    void BlockerComponent::Deactivate()
    {
        AreaComponentBase::Deactivate(); //must deactivate base first to ensure AreaRequestBus disconnect waits for other threads
        BlockerRequestBus::Handler::BusDisconnect();

#if VEG_BLOCKER_ENABLE_CACHING
        AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
        m_claimCacheMapping.clear();
#endif
    }


    void BlockerComponent::OnCompositionChanged()
    {
        AreaComponentBase::OnCompositionChanged();

#if VEG_BLOCKER_ENABLE_CACHING
        AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
        m_claimCacheMapping.clear();
#endif
    }

    bool BlockerComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        AreaComponentBase::ReadInConfig(baseConfig);
        if (auto config = azrtti_cast<const BlockerConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool BlockerComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        AreaComponentBase::WriteOutConfig(outBaseConfig);
        if (auto config = azrtti_cast<BlockerConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool BlockerComponent::PrepareToClaim([[maybe_unused]] EntityIdStack& stackIds)
    {
        return true;
    }

    bool BlockerComponent::ClaimPosition(EntityIdStack& processedIds, const ClaimPoint& point, InstanceData& instanceData)
    {
        AZ_PROFILE_FUNCTION(Vegetation);

#if VEG_BLOCKER_ENABLE_CACHING
        {
            AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
            auto claimItr = m_claimCacheMapping.find(point.m_handle);
            if (claimItr != m_claimCacheMapping.end())
            {
                return claimItr->second;
            }
        }
#endif

        // test shape bus as first pass to claim the point
        bool isInsideShape = true;
        for (const auto& id : processedIds)
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(isInsideShape, id, &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point.m_position);
            if (!isInsideShape)
            {
#if VEG_BLOCKER_ENABLE_CACHING
                AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
                m_claimCacheMapping[point.m_handle] = false;
#endif
                return false;
            }
        }

        //generate details for a single vegetation instance
        instanceData.m_position = point.m_position;
        instanceData.m_normal = point.m_normal;
        instanceData.m_masks = point.m_masks;

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
#if VEG_BLOCKER_ENABLE_CACHING
                AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
                m_claimCacheMapping[point.m_handle] = false;
#endif
                return false;
            }
        }

#if VEG_BLOCKER_ENABLE_CACHING
        AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
        m_claimCacheMapping[point.m_handle] = true;
#endif
        return true;
    }

    void BlockerComponent::ClaimPositions(EntityIdStack& stackIds, ClaimContext& context)
    {
        AZ_PROFILE_FUNCTION(Vegetation);

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

    void BlockerComponent::UnclaimPosition([[maybe_unused]] const ClaimHandle handle)
    {
    }

    AZ::Aabb BlockerComponent::GetEncompassingAabb() const
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return bounds;
    }

    AZ::u32 BlockerComponent::GetProductCount() const
    {
        return 0;
    }

    AZ::u32 BlockerComponent::GetAreaPriority() const
    {
        return m_configuration.m_priority;
    }

    void BlockerComponent::SetAreaPriority(AZ::u32 priority)
    {
        m_configuration.m_priority = priority;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u32 BlockerComponent::GetAreaLayer() const
    {
        return m_configuration.m_layer;
    }

    void BlockerComponent::SetAreaLayer(AZ::u32 layer)
    {
        m_configuration.m_layer = layer;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u32 BlockerComponent::GetAreaProductCount() const
    {
        return GetProductCount();
    }

    bool BlockerComponent::GetInheritBehavior() const
    {
        return m_configuration.m_inheritBehavior;
    }

    void BlockerComponent::SetInheritBehavior(bool value)
    {
        m_configuration.m_inheritBehavior = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
