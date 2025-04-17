/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <VegetationProfiler.h>
#include "SpawnerComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <Vegetation/Ebuses/AreaDebugBus.h>
#include <Vegetation/Ebuses/AreaInfoBus.h>
#include <Vegetation/Ebuses/DescriptorProviderRequestBus.h>
#include <Vegetation/InstanceData.h>
#include <GradientSignal/Util.h>
#include <SurfaceData/SurfaceDataTagEnumeratorRequestBus.h>
#include <AzCore/std/sort.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <Vegetation/Ebuses/DebugNotificationBus.h>

namespace Vegetation
{
    namespace SpawnerUtil
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

    void SpawnerConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SpawnerConfig, AreaConfig>()
                ->Version(1, &SpawnerUtil::UpdateVersion)
                ->Field("InheritBehavior", &SpawnerConfig::m_inheritBehavior)
                ->Field("AllowEmptyMeshes", &SpawnerConfig::m_allowEmptyMeshes)
                ->Field("FilterStage", &SpawnerConfig::m_filterStage)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SpawnerConfig>(
                    "Vegetation Layer Spawner", "Vegetation spawner")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SpawnerConfig::m_inheritBehavior, "Inherit Behavior", "Allow shapes, modifiers, filters of a parent to affect this area.")
                    ->DataElement(0, &SpawnerConfig::m_allowEmptyMeshes, "Allow Empty Assets", "Allow unspecified asset references in the Descriptors to claim space and block other vegetation.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SpawnerConfig::m_filterStage, "Filter Stage", "Determines if filter is applied before (PreProcess) or after (PostProcess) modifiers.")
                    ->EnumAttribute(FilterStage::PreProcess, "PreProcess")
                    ->EnumAttribute(FilterStage::PostProcess, "PostProcess")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SpawnerConfig>("VegetationSpawnerConfig")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("filterStage",
                    [](SpawnerConfig* config) { return (AZ::u8&)(config->m_filterStage); },
                    [](SpawnerConfig* config, const AZ::u8& i) { config->m_filterStage = (FilterStage)i; })
                ->Property("inheritBehavior", BehaviorValueProperty(&SpawnerConfig::m_inheritBehavior))
                ->Property("allowEmptyMeshes", BehaviorValueProperty(&SpawnerConfig::m_allowEmptyMeshes))
                ;
        }
    }

    void SpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetProvidedServices(services);
    }

    void SpawnerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetIncompatibleServices(services);
    }

    void SpawnerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        AreaComponentBase::GetRequiredServices(services);
        services.push_back(AZ_CRC_CE("VegetationDescriptorProviderService"));
        services.push_back(AZ_CRC_CE("ShapeService"));
    }

    void SpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        SpawnerConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SpawnerComponent, AreaComponentBase>()
                ->Version(0)
                ->Field("Configuration", &SpawnerComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("VegetationSpawnerComponentTypeId", BehaviorConstant(SpawnerComponentTypeId));

            behaviorContext->Class<SpawnerComponent>("VegetationSpawner")->RequestBus("SpawnerRequestBus");

            behaviorContext->EBus<SpawnerRequestBus>("VegetationSpawnerRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Event("GetAreaPriority", &SpawnerRequestBus::Events::GetAreaPriority)
                ->Event("SetAreaPriority", &SpawnerRequestBus::Events::SetAreaPriority)
                ->VirtualProperty("AreaPriority", "GetAreaPriority", "SetAreaPriority")
                ->Event("GetAreaLayer", &SpawnerRequestBus::Events::GetAreaLayer)
                ->Event("SetAreaLayer", &SpawnerRequestBus::Events::SetAreaLayer)
                ->VirtualProperty("AreaLayer", "GetAreaLayer", "SetAreaLayer")
                ->Event("GetAreaProductCount", &SpawnerRequestBus::Events::GetAreaProductCount)
                ->Event("GetInheritBehavior", &SpawnerRequestBus::Events::GetInheritBehavior)
                ->Event("SetInheritBehavior", &SpawnerRequestBus::Events::SetInheritBehavior)
                ->VirtualProperty("InheritBehavior", "GetInheritBehavior", "SetInheritBehavior")
                ->Event("GetAllowEmptyMeshes", &SpawnerRequestBus::Events::GetAllowEmptyMeshes)
                ->Event("SetAllowEmptyMeshes", &SpawnerRequestBus::Events::SetAllowEmptyMeshes)
                ->VirtualProperty("AllowEmptyMeshes", "GetAllowEmptyMeshes", "SetAllowEmptyMeshes")
                ->Event("GetFilterStage", &SpawnerRequestBus::Events::GetFilterStage)
                ->Event("SetFilterStage", &SpawnerRequestBus::Events::SetFilterStage)
                ->VirtualProperty("FilterStage", "GetFilterStage", "SetFilterStage")
                ;
        }
    }

    SpawnerComponent::SpawnerComponent(const SpawnerConfig& configuration)
        : AreaComponentBase(configuration)
        , m_configuration(configuration)
    {
    }

    void SpawnerComponent::Activate()
    {
        ClearSelectableDescriptors();

        SpawnerRequestBus::Handler::BusConnect(GetEntityId());

        AreaComponentBase::Activate(); //must activate base last to connect AreaRequestBus once everything else is setup
    }

    void SpawnerComponent::Deactivate()
    {
        AreaComponentBase::Deactivate(); //must deactivate base first to ensure AreaRequestBus disconnect waits for other threads

        SpawnerRequestBus::Handler::BusDisconnect();

        OnUnregisterArea();
    }

    void SpawnerComponent::OnRegisterArea()
    {
        // Every time our area becomes valid and registered, make sure we clear our
        // temporary descriptor caches.  These should only contain valid data during
        // PrepareToClaim / Claim, but it doesn't hurt to ensure they're cleared.
        ClearSelectableDescriptors();
    }

    void SpawnerComponent::OnUnregisterArea()
    {
        // Every time our area becomes invalid and unregistered, make sure we destroy
        // all instances that this component spawned.  Also clear our temporary descriptor
        // caches just to keep things tidy.
        ClearSelectableDescriptors();
        DestroyAllInstances();
    }

    bool SpawnerComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        AreaComponentBase::ReadInConfig(baseConfig);
        if (auto config = azrtti_cast<const SpawnerConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SpawnerComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        AreaComponentBase::WriteOutConfig(outBaseConfig);
        if (auto config = azrtti_cast<SpawnerConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool SpawnerComponent::PrepareToClaim(EntityIdStack& stackIds)
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        //adding entity id to the stack of entity ids affecting vegetation
        EntityIdStack emptyIds;
        //when the inherit flag is disabled, as opposed to always inheriting, the stack must be cleared but preserved so redirecting to an empty stack to avoid copying
        EntityIdStack& processedIds = m_configuration.m_inheritBehavior ? stackIds : emptyIds;
        //adding current entity id to be processed uniformly
        EntityIdStackPusher stackPusher(processedIds, GetEntityId());

#if defined(VEG_PROFILE_ENABLED)
        CalcInstanceDebugColor(processedIds);
#endif

        //gather tags from all sources so we can early out of processing this area
        bool includeAll = false;
        m_inclusiveTagsToConsider.clear();
        m_exclusiveTagsToConsider.clear();
        for (const auto& id : processedIds)
        {
            SurfaceData::SurfaceDataTagEnumeratorRequestBus::Event(id, &SurfaceData::SurfaceDataTagEnumeratorRequestBus::Events::GetInclusionSurfaceTags, m_inclusiveTagsToConsider, includeAll);
            SurfaceData::SurfaceDataTagEnumeratorRequestBus::Event(id, &SurfaceData::SurfaceDataTagEnumeratorRequestBus::Events::GetExclusionSurfaceTags, m_exclusiveTagsToConsider);
        }

        // If anything is telling us to include all surfaces, clear out our list, as this means "check everything".
        if (includeAll)
        {
            m_inclusiveTagsToConsider.clear();
        }

        AZStd::sort(m_inclusiveTagsToConsider.begin(), m_inclusiveTagsToConsider.end());
        m_inclusiveTagsToConsider.erase(  
            std::unique(m_inclusiveTagsToConsider.begin(), m_inclusiveTagsToConsider.end()),
            m_inclusiveTagsToConsider.end());
        AZStd::sort(m_exclusiveTagsToConsider.begin(), m_exclusiveTagsToConsider.end());
        m_exclusiveTagsToConsider.erase(
            std::unique(m_exclusiveTagsToConsider.begin(), m_exclusiveTagsToConsider.end()),
            m_exclusiveTagsToConsider.end());

        //reset selectable descriptors
        ClearSelectableDescriptors();

        //gather all descriptors to be used for vegetation selection
        AZStd::lock_guard<decltype(m_selectableDescriptorMutex)> selectableDescriptorLock(m_selectableDescriptorMutex);
        for (const auto& id : processedIds)
        {
            DescriptorProviderRequestBus::Event(id, &DescriptorProviderRequestBus::Events::GetDescriptors, m_selectableDescriptorCache);
        }

        return !m_selectableDescriptorCache.empty();
    }

    bool SpawnerComponent::CreateInstance([[maybe_unused]] const ClaimPoint &point, InstanceData& instanceData)
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        instanceData.m_instanceId = InvalidInstanceId;
        if (instanceData.m_descriptorPtr && instanceData.m_descriptorPtr->IsSpawnable())
        {
            InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::CreateInstance, instanceData);
        }

        return instanceData.m_instanceId != InvalidInstanceId || m_configuration.m_allowEmptyMeshes;
    }

    void SpawnerComponent::ClearSelectableDescriptors()
    {
        AZStd::lock_guard<decltype(m_selectableDescriptorMutex)> selectableDescriptorLock(m_selectableDescriptorMutex);
        m_selectedDescriptors.clear();
        m_selectableDescriptorCache.clear();
    }

    bool SpawnerComponent::EvaluateFilters(EntityIdStack& processedIds, InstanceData& instanceData, const FilterStage intendedStage) const
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        bool accepted = true;
        for (const auto& id : processedIds)
        {
            FilterRequestBus::EnumerateHandlersId(id, [this, &instanceData, &accepted, intendedStage](FilterRequestBus::Events* handler) {
                const FilterStage stage = handler->GetFilterStage();
                if (stage == intendedStage || (stage == FilterStage::Default && m_configuration.m_filterStage == intendedStage))
                {
                    accepted = handler->Evaluate(instanceData);
                }
                return accepted;
            });
            if (!accepted)
            {
                break;
            }
        }
        return accepted;
    }

    bool SpawnerComponent::ProcessInstance(EntityIdStack& processedIds, const ClaimPoint& point, InstanceData& instanceData, DescriptorPtr descriptorPtr)
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        if (!descriptorPtr)
        {
            AZ_Error("vegetation", descriptorPtr, "DescriptorPtr should always be valid when spawning!");
            return false;
        }

        // If this is an empty mesh asset (no valid id) AND we don't allow empty meshes, skip this descriptor
        if (!m_configuration.m_allowEmptyMeshes && descriptorPtr->HasEmptyAssetReferences())
        {
            return false;
        }

        //generate details for a single vegetation instance using the current descriptor
        static const AZ::Quaternion identityQuat = AZ::Quaternion::CreateIdentity();
        instanceData.m_descriptorPtr = descriptorPtr;
        instanceData.m_instanceId = InvalidInstanceId;
        instanceData.m_position = point.m_position;
        instanceData.m_normal = point.m_normal;
        instanceData.m_masks = point.m_masks;
        instanceData.m_rotation = identityQuat;
        instanceData.m_alignment = identityQuat;
        instanceData.m_scale = 1.0f;

        // run pre-process filters on unmodified instance data
        if (!EvaluateFilters(processedIds, instanceData, FilterStage::PreProcess))
        {
            // Once a filter rejects the instance, no point in running the rest.
            return false;
        }

        // If all of the pre-process filters have allowed this instance to be placed, run the modifiers and post-process filters.
        for (const auto& id : processedIds)
        {
            ModifierRequestBus::Event(id, &ModifierRequestBus::Events::Execute, instanceData);
        }

        // run post-process filters on modified instance data
        if (!EvaluateFilters(processedIds, instanceData, FilterStage::PostProcess))
        {
            // Once a filter rejects the instance, no point in running the rest.
            return false;
        }

        // If we made it through all the filters successfully, this descriptor can claim this instance.
        return true;
    }

    bool SpawnerComponent::ClaimPosition(EntityIdStack& processedIds, const ClaimPoint& point, InstanceData& instanceData)
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

#if VEG_SPAWNER_ENABLE_CACHING
        {
            //return early if the point has already been rejected
            AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
            if (m_rejectedClaimCache.find(point.m_handle) != m_rejectedClaimCache.end())
            {
                return false;
            }

            //return early if an instance has already been generated and cached for this point
            auto acceptedClaimCacheItr = m_acceptedClaimCache.find(point.m_handle);
            if (acceptedClaimCacheItr != m_acceptedClaimCache.end())
            {
                instanceData = acceptedClaimCacheItr->second;
                instanceData.m_instanceId = InvalidInstanceId;
                return true;
            }
        }
#endif

        // test shape bus as first pass to claim the point
        for (const auto& id : processedIds)
        {
            bool accepted = true;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(accepted, id, &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point.m_position);
            if (!accepted)
            {
                VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FilterInstance, instanceData.m_id, AZStd::string_view("ShapeFilter")));
                return false;
            }
        }

        //generate uvw sample coordinates
        DescriptorSelectorParams selectorParams;
        selectorParams.m_position = point.m_position;

        //copy the set of all selectable descriptors then remove any that don't pass the selection filter
        AZStd::lock_guard<decltype(m_selectableDescriptorMutex)> selectableDescriptorLock(m_selectableDescriptorMutex);
        m_selectedDescriptors = m_selectableDescriptorCache;
        for (const auto& id : processedIds)
        {
            DescriptorSelectorRequestBus::Event(id, &DescriptorSelectorRequestBus::Events::SelectDescriptors, selectorParams, m_selectedDescriptors);
        }

        for (DescriptorPtr descriptorPtr : m_selectedDescriptors)
        {
            if (ProcessInstance(processedIds, point, instanceData, descriptorPtr))
            {
                return true;
            }
        }

        // All the descriptors were filtered out, so don't claim the point.
        return false;
    }

    void SpawnerComponent::ClaimPositions(EntityIdStack& stackIds, ClaimContext& context)
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        //reject entire spawner if there are inclusion tags to consider that don't exist in the context
        if (context.m_masks.HasValidTags() &&
            SurfaceData::HasValidTags(m_inclusiveTagsToConsider) &&
            !context.m_masks.HasAnyMatchingTags(m_inclusiveTagsToConsider))
        {
            VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::MarkAreaRejectedByMask, GetEntityId()));
            return;
        }

        //see comments in PrepareToClaim
        EntityIdStack emptyIds;
        EntityIdStack& processedIds = m_configuration.m_inheritBehavior ? stackIds : emptyIds;
        EntityIdStackPusher stackPusher(processedIds, GetEntityId());

        InstanceData instanceData;
        instanceData.m_id = GetEntityId();
        instanceData.m_changeIndex = GetChangeIndex();

        size_t numAvailablePoints = context.m_availablePoints.size();
        for (size_t pointIndex = 0; pointIndex < numAvailablePoints; )
        {
            ClaimPoint& point = context.m_availablePoints[pointIndex];

            bool accepted = false;
            if (ClaimPosition(processedIds, point, instanceData))
            {
                // Check if an identical instance already exists for reuse
                if (context.m_existedCallback(point, instanceData))
                {
                    accepted = true;
                }
                else
                {
                    if (CreateInstance(point, instanceData))
                    {
                        accepted = true;

                        //notify the caller that this claim succeeded so it can do any cleanup or registration
                        context.m_createdCallback(point, instanceData);

                        //only store the instance id after all claim logic executes in case prior claim and instance gets released
                        AZStd::lock_guard<decltype(m_claimInstanceMappingMutex)> claimInstanceMappingMutexLock(m_claimInstanceMappingMutex);
                        m_claimInstanceMapping[point.m_handle] = instanceData.m_instanceId;
                    }
                }
            }

            if (accepted)
            {
                //Swap an available point from the end of the list
                AZStd::swap(point, context.m_availablePoints.at(numAvailablePoints - 1));
                --numAvailablePoints;

#if VEG_SPAWNER_ENABLE_CACHING
                AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
                m_acceptedClaimCache[point.m_handle] = instanceData;
                m_rejectedClaimCache.erase(point.m_handle);
#endif
            }
            else
            {
                UnclaimPosition(point.m_handle);
                ++pointIndex;

#if VEG_SPAWNER_ENABLE_CACHING
                AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
                m_acceptedClaimCache.erase(point.m_handle);
                m_rejectedClaimCache.insert(point.m_handle);
#endif
            }
        }

        //resize to remove all used points
        context.m_availablePoints.resize(numAvailablePoints);

        //release residual descriptors and asset references used this claim attempt
        AZStd::lock_guard<decltype(m_selectableDescriptorMutex)> selectableDescriptorLock(m_selectableDescriptorMutex);
        m_selectedDescriptors.clear();
    }

    void SpawnerComponent::UnclaimPosition(const ClaimHandle handle)
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        InstanceId instanceId = InvalidInstanceId;
        {
            AZStd::lock_guard<decltype(m_claimInstanceMappingMutex)> claimInstanceMappingMutexLock(m_claimInstanceMappingMutex);
            auto claimItr = m_claimInstanceMapping.find(handle);
            if (claimItr != m_claimInstanceMapping.end())
            {
                instanceId = claimItr->second;
                m_claimInstanceMapping.erase(claimItr);
            }
        }

        if (instanceId != InvalidInstanceId)
        {
            InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::DestroyInstance, instanceId);
        }
    }

    AZ::Aabb SpawnerComponent::GetEncompassingAabb() const
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return bounds;
    }

    AZ::u32 SpawnerComponent::GetProductCount() const
    {
        AZStd::lock_guard<decltype(m_claimInstanceMappingMutex)> claimInstanceMappingMutexLock(m_claimInstanceMappingMutex);
        return static_cast<AZ::u32>(m_claimInstanceMapping.size());
    }

    void SpawnerComponent::OnCompositionChanged()
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE
        AreaComponentBase::OnCompositionChanged();

#if VEG_SPAWNER_ENABLE_CACHING
        //wipe the cache when content changes
        AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
        m_acceptedClaimCache.clear();
        m_rejectedClaimCache.clear();
#endif
    }

    void SpawnerComponent::DestroyAllInstances()
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        ClaimInstanceMapping claimInstanceMapping;
        {
            AZStd::lock_guard<decltype(m_claimInstanceMappingMutex)> claimInstanceMappingMutexLock(m_claimInstanceMappingMutex);
            AZStd::swap(claimInstanceMapping, m_claimInstanceMapping);
        }

        for (const auto& claim : claimInstanceMapping)
        {
            const auto instanceId = claim.second;
            InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::DestroyInstance, instanceId);
        }

#if VEG_SPAWNER_ENABLE_CACHING
        //wipe the cache
        AZStd::lock_guard<decltype(m_cacheMutex)> cacheLock(m_cacheMutex);
        m_acceptedClaimCache.clear();
        m_rejectedClaimCache.clear();
#endif
    }

    void SpawnerComponent::CalcInstanceDebugColor(const EntityIdStack& processedIds)
    {
        AreaDebugBus::Event(GetEntityId(), &AreaDebugBus::Events::ResetBlendedDebugDisplayData);
        for (const auto& id : processedIds)
        {
            AreaDebugDisplayData debugDisplayData;
            AreaDebugBus::EventResult(debugDisplayData, id, &AreaDebugBus::Events::GetBaseDebugDisplayData);
            AreaDebugBus::Event(GetEntityId(), &AreaDebugBus::Events::AddBlendedDebugDisplayData, debugDisplayData);
        }
    }

    AZ::u32 SpawnerComponent::GetAreaPriority() const
    {
        return m_configuration.m_priority;
    }

    void SpawnerComponent::SetAreaPriority(AZ::u32 priority)
    {
        m_configuration.m_priority = priority;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u32 SpawnerComponent::GetAreaLayer() const
    {
        return m_configuration.m_layer;
    }

    void SpawnerComponent::SetAreaLayer(AZ::u32 layer)
    {
        m_configuration.m_layer = layer;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u32 SpawnerComponent::GetAreaProductCount() const
    {
        return GetProductCount();
    }

    bool SpawnerComponent::GetInheritBehavior() const
    {
        return m_configuration.m_inheritBehavior;
    }

    void SpawnerComponent::SetInheritBehavior(bool value)
    {
        m_configuration.m_inheritBehavior = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool SpawnerComponent::GetAllowEmptyMeshes() const
    {
        return m_configuration.m_allowEmptyMeshes;
    }

    void SpawnerComponent::SetAllowEmptyMeshes(bool value)
    {
        m_configuration.m_allowEmptyMeshes = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    FilterStage SpawnerComponent::GetFilterStage() const
    {
        return m_configuration.m_filterStage;
    }

    void SpawnerComponent::SetFilterStage(FilterStage filterStage)
    {
        m_configuration.m_filterStage = filterStage;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
