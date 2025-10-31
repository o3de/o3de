/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DescriptorListComponent.h"
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

namespace Vegetation
{
    void DescriptorListConfig::Reflect(AZ::ReflectContext* context)
    {
        // Ensure that Descriptor has been reflected since we reference it in the ElementAttribute for NameLabelOverride
        Descriptor::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DescriptorListConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("DescriptorListSourceType", &DescriptorListConfig::m_sourceType)
                ->Field("DescriptorListAsset", &DescriptorListConfig::m_descriptorListAsset)
                ->Field("Descriptors", &DescriptorListConfig::m_descriptors)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<DescriptorListConfig>(
                    "Vegetation Asset List", "Vegetation descriptor assets")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DescriptorListConfig::m_sourceType, "Source Type", "Determines if descriptors are embedded or in an external asset.")
                    ->EnumAttribute(DescriptorListSourceType::EMBEDDED, "Embedded")
                    ->EnumAttribute(DescriptorListSourceType::EXTERNAL, "External")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(0, &DescriptorListConfig::m_descriptorListAsset, "External Assets", "Asset containing a set of vegetation descriptors.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &DescriptorListConfig::IsExternalSource)

                    ->DataElement(0, &DescriptorListConfig::m_descriptors, "Embedded Assets", "Set of vegetation descriptors.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &DescriptorListConfig::IsEmbeddedSource)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerReorderAllow, true)
                    ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, &Descriptor::GetDescriptorName)
                    ->ElementAttribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DescriptorListConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Method("GetDescriptorListSourceType", &DescriptorListConfig::GetDescriptorListSourceType)
                ->Method("SetDescriptorListSourceType", &DescriptorListConfig::SetDescriptorListSourceType)
                ->Method("GetDescriptorAssetPath", &DescriptorListConfig::GetDescriptorAssetPath)
                ->Method("SetDescriptorAssetPath", &DescriptorListConfig::SetDescriptorAssetPath)
                ->Method("GetNumDescriptors", &DescriptorListConfig::GetNumDescriptors)
                ->Method("GetDescriptor", &DescriptorListConfig::GetDescriptor)
                ->Method("RemoveDescriptor", &DescriptorListConfig::RemoveDescriptor)
                ->Method("SetDescriptor", &DescriptorListConfig::SetDescriptor)
                ->Method("AddDescriptor", &DescriptorListConfig::AddDescriptor)
                ;
        }
    }

    DescriptorListSourceType DescriptorListConfig::GetDescriptorListSourceType() const
    {
        return m_sourceType;
    }

    void DescriptorListConfig::SetDescriptorListSourceType(DescriptorListSourceType sourceType)
    {
        m_sourceType = sourceType;
    }

    AZStd::string DescriptorListConfig::GetDescriptorAssetPath() const
    {
        AZStd::string assetPathString;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_descriptorListAsset.GetId());
        return assetPathString;
    }

    void DescriptorListConfig::SetDescriptorAssetPath(const AZStd::string& assetPath)
    {
        AZ::Data::AssetId assetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, assetPath.c_str(), AZ::Data::s_invalidAssetType, false);
        if (assetId.IsValid())
        {
            m_descriptorListAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<DescriptorListAsset>(assetId, m_descriptorListAsset.GetAutoLoadBehavior());
        }
    }

    size_t DescriptorListConfig::GetNumDescriptors() const
    {
        return m_descriptors.size();
    }

    Descriptor* DescriptorListConfig::GetDescriptor(int index)
    {
        if (index < m_descriptors.size() && index >= 0)
        {
            return &m_descriptors[index];
        }

        return nullptr;
    }

    void DescriptorListConfig::RemoveDescriptor(int index)
    {
        if (index < m_descriptors.size() && index >= 0)
        {
            m_descriptors.erase(m_descriptors.begin() + index);
        }
    }

    void DescriptorListConfig::SetDescriptor(int index, Descriptor* descriptor)
    {
        if (index < m_descriptors.size() && index >= 0)
        {
            m_descriptors[index] = descriptor ? *descriptor : Descriptor();
        }
    }

    void DescriptorListConfig::AddDescriptor(Descriptor* descriptor)
    {
        m_descriptors.push_back(descriptor ? *descriptor : Descriptor());
    }

    void DescriptorListComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationDescriptorProviderService"));
    }

    void DescriptorListComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationDescriptorProviderService"));
    }

    void DescriptorListComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void DescriptorListComponent::Reflect(AZ::ReflectContext* context)
    {
        DescriptorListConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DescriptorListComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &DescriptorListComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("DescriptorListComponentTypeId", BehaviorConstant(DescriptorListComponentTypeId));

            behaviorContext->Class<DescriptorListComponent>()->RequestBus("DescriptorListRequestBus");

            behaviorContext->EBus<DescriptorListRequestBus>("DescriptorListRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetDescriptorListSourceType", &DescriptorListRequestBus::Events::GetDescriptorListSourceType)
                ->Event("SetDescriptorListSourceType", &DescriptorListRequestBus::Events::SetDescriptorListSourceType)
                ->Event("GetDescriptorAssetPath", &DescriptorListRequestBus::Events::GetDescriptorAssetPath)
                ->Event("SetDescriptorAssetPath", &DescriptorListRequestBus::Events::SetDescriptorAssetPath)
                ->Event("GetNumDescriptors", &DescriptorListRequestBus::Events::GetNumDescriptors)
                ->Event("GetDescriptor", &DescriptorListRequestBus::Events::GetDescriptor)
                ->Event("RemoveDescriptor", &DescriptorListRequestBus::Events::RemoveDescriptor)
                ->Event("SetDescriptor", &DescriptorListRequestBus::Events::SetDescriptor)
                ->Event("AddDescriptor", &DescriptorListRequestBus::Events::AddDescriptor)
                ;
        }
    }

    DescriptorListComponent::DescriptorListComponent(const DescriptorListConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void DescriptorListComponent::Activate()
    {
        // This component is managing a list of Descriptors, each of which contains an InstanceSpawner.  The
        // list itself can either be embedded in the component configuration or loaded from an asset.
        // On activation, the component loads the DescriptorListAsset if one is used, and loads all the assets
        // used by all of the Descriptors.
        // Once all of the assets are loaded, the Descriptors get registered with the vegetation system, at which
        // point they will be used to start placing vegetation.
        // The vegetation system optimizes memory by sharing pointers to identical Descriptors where possible,
        // and to identical InstanceSpawners where possible, so the component also keeps track of the shared pointers
        // returned from the system registration.

        DescriptorListRequestBus::Handler::BusConnect(GetEntityId());
        DescriptorProviderRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceData::SurfaceDataTagEnumeratorRequestBus::Handler::BusConnect(GetEntityId());

        LoadAssets();
    }

    void DescriptorListComponent::Deactivate()
    {
        // First, make sure we unregister with the vegetation system.
        ReleaseUniqueDescriptors();

        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        DescriptorListRequestBus::Handler::BusDisconnect();
        DescriptorProviderRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataTagEnumeratorRequestBus::Handler::BusDisconnect();

        // Stop listening for descriptor load/unload notifications before unloading the assets
        // to ensure that we don't try to process the changes while deactivating.
        DescriptorNotificationBus::MultiHandler::BusDisconnect();

        m_configuration.m_descriptorListAsset.Release();
        for (auto& descriptor : m_configuration.m_descriptors)
        {
            descriptor.UnloadAssets();
        }
    }

    bool DescriptorListComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const DescriptorListConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool DescriptorListComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<DescriptorListConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void DescriptorListComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_Assert(m_configuration.IsExternalSource(), "Unexpected notification of a DescriptorListAsset being loaded.");
        AZ_Assert(m_configuration.m_descriptorListAsset.GetId() == asset.GetId(), "Unexpected notification of a non-DescriptorList asset.");

        m_configuration.m_descriptorListAsset = asset;
        if (m_configuration.m_descriptorListAsset.IsReady())
        {
            LoadAssetsFromDescriptorList();
        }
    }

    void DescriptorListComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void DescriptorListComponent::GetDescriptors(DescriptorPtrVec& descriptors) const
    {
        AZStd::lock_guard<decltype(uniqueDescriptorsMutex)> descriptorLock(uniqueDescriptorsMutex);
        descriptors.insert(descriptors.end(), m_uniqueDescriptors.begin(), m_uniqueDescriptors.end());
    }

    void DescriptorListComponent::GetInclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags, bool& includeAll) const
    {
        if (IsFullyLoaded())
        {
            AZStd::lock_guard<decltype(uniqueDescriptorsMutex)> descriptorLock(uniqueDescriptorsMutex);
            for (const auto& descriptorPtr : m_uniqueDescriptors)
            {
                if (descriptorPtr && descriptorPtr->m_surfaceFilterOverrideMode != OverrideMode::Disable)
                {
                    tags.insert(tags.end(), descriptorPtr->m_inclusiveSurfaceFilterTags.begin(), descriptorPtr->m_inclusiveSurfaceFilterTags.end());

                    // If we're overriding the include list and the include list has no valid tags, that means "include everything"
                    if (!SurfaceData::HasValidTags(descriptorPtr->m_inclusiveSurfaceFilterTags))
                    {
                        includeAll = true;
                    }
                }
            }
        }
    }

    void DescriptorListComponent::GetExclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags) const
    {
        if (IsFullyLoaded())
        {
            AZStd::lock_guard<decltype(uniqueDescriptorsMutex)> descriptorLock(uniqueDescriptorsMutex);
            for (const auto& descriptorPtr : m_uniqueDescriptors)
            {
                if (descriptorPtr && descriptorPtr->m_surfaceFilterOverrideMode != OverrideMode::Disable)
                {
                    tags.insert(tags.end(), descriptorPtr->m_exclusiveSurfaceFilterTags.begin(), descriptorPtr->m_exclusiveSurfaceFilterTags.end());
                }
            }
        }
    }

    void DescriptorListComponent::LoadAssets(AZStd::vector<Descriptor>& descriptors)
    {
        // Before queueing any new assets to load, make sure we stop listening for any
        // load/unload notifications.  We'll listen again after the queueing is complete.
        DescriptorNotificationBus::MultiHandler::BusDisconnect();

        for (auto& descriptor : descriptors)
        {
            if (!descriptor.HasEmptyAssetReferences())
            {
                // If this descriptor has assets, queue them to start loading.
                descriptor.LoadAssets();
            }
        }

        // Register to listen to load/unload notifications for all of the provided descriptors.
        // Any time one of these notifications triggers, we need to either register or unregister with the
        // vegetation system, depending on whether or not we have the full set of expected data available.
        // Registration happens after queueing is complete to ensure that we don't get interruptions while
        // in the process of queueing the loads.  It's safe to ignore the messages while queueing because
        // we proactively check the load status below.
        for (auto& descriptor : descriptors)
        {
            DescriptorNotificationBus::MultiHandler::BusConnect(descriptor.GetDescriptorNotificationBusId());
        }

        // Check our loading status and move on to registration if loading is complete, because it's possible
        // that all of the loading finished before we registered to listen on the DescriptorNotificationBus.
        ProcessDescriptorLoadingStatus();
    }

    void DescriptorListComponent::OnDescriptorAssetsLoaded()
    {
        // Because we've loaded at least one more needed asset, check our overall loading status and move on
        // to registration if loading is complete.
        ProcessDescriptorLoadingStatus();
    }

    void DescriptorListComponent::OnDescriptorAssetsUnloaded()
    {
        // Because we've unloaded at least one needed asset, the following call will deregister from the
        // vegetation system.  We'll register again if one or more OnDescriptorAssetsLoaded calls occur
        // to bring us back to a fully loaded state.
        ProcessDescriptorLoadingStatus();
    }

    void DescriptorListComponent::LoadAssetsFromDescriptorList()
    {
        LoadAssets(m_configuration.m_descriptorListAsset.Get()->m_descriptors);
    }

    void DescriptorListComponent::LoadAssets()
    {
        ReleaseUniqueDescriptors();

        AZ::Data::AssetBus::MultiHandler::BusDisconnect();

        if (m_configuration.IsExternalSource())
        {
            if (m_configuration.m_descriptorListAsset.GetId().IsValid())
            {
                m_configuration.m_descriptorListAsset.QueueLoad();
                AZ::Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_descriptorListAsset.GetId());

                if (m_configuration.m_descriptorListAsset.IsReady())
                {
                    LoadAssetsFromDescriptorList();
                }
            }
        }
        else
        {
            LoadAssets(m_configuration.m_descriptors);
        }
    }

    void DescriptorListComponent::RegisterUniqueDescriptors(AZStd::vector<Descriptor>& descriptors)
    {
        // Stop listening to load/unload notifications from our current descriptor list.
        // On registration, the vegetation system might provide us new Descriptor and/or InstanceSpawner pointers,
        // so we'll register to listen to the newly-returned instances below.
        DescriptorNotificationBus::MultiHandler::BusDisconnect();

        for (auto& descriptor : descriptors)
        {
            if (descriptor.m_weight > 0.0f)
            {
                DescriptorPtr descriptorPtr;
                InstanceSystemRequestBus::BroadcastResult(descriptorPtr, &InstanceSystemRequestBus::Events::RegisterUniqueDescriptor, descriptor);
                if (descriptorPtr)
                {
                    // RegisterUniqueDescriptor can return a pointer to an existing Descriptor (and/or InstanceSpawner),
                    // as opposed to the one we passed in, so make sure we're monitoring its notification bus instead of the
                    // one for the original Descriptor.
                    DescriptorNotificationBus::MultiHandler::BusConnect(descriptorPtr->GetDescriptorNotificationBusId());
                    m_uniqueDescriptors.push_back(descriptorPtr);
                }
            }
        }
    }

    void DescriptorListComponent::ProcessDescriptorLoadingStatus()
    {
        ReleaseUniqueDescriptors();

        if (IsFullyLoaded())
        {
            AZStd::lock_guard<decltype(uniqueDescriptorsMutex)> descriptorLock(uniqueDescriptorsMutex);

            if (m_configuration.IsExternalSource())
            {
                if (m_configuration.m_descriptorListAsset.IsReady())
                {
                    RegisterUniqueDescriptors(m_configuration.m_descriptorListAsset.Get()->m_descriptors);
                }
            }
            else
            {
                RegisterUniqueDescriptors(m_configuration.m_descriptors);
            }

            LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    void DescriptorListComponent::ReleaseUniqueDescriptors()
    {
        AZStd::lock_guard<decltype(uniqueDescriptorsMutex)> descriptorLock(uniqueDescriptorsMutex);
        if (!m_uniqueDescriptors.empty())
        {
            // Stop listening to all Descriptor load/unload notifications until the next time we trigger a load ourselves.
            // m_uniqueDescriptors only contains entries after a load is complete, so there shouldn't be any loads in flight
            // at the point that we disconnect here.
            DescriptorNotificationBus::MultiHandler::BusDisconnect();

            for (auto& descriptorPtr : m_uniqueDescriptors)
            {
                InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::ReleaseUniqueDescriptor, descriptorPtr);
            }

            m_uniqueDescriptors.clear();
            LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    bool DescriptorListComponent::IsFullyLoaded(const AZStd::vector<Descriptor>& descriptors) const
    {
        for (auto& descriptor : descriptors)
        {
            if (!descriptor.HasEmptyAssetReferences() && !descriptor.IsLoaded())
            {
                return false;
            }
        }
        return true;
    }

    bool DescriptorListComponent::IsFullyLoaded() const
    {
        if (m_configuration.IsExternalSource())
        {
            if (m_configuration.m_descriptorListAsset.IsReady())
            {
                return IsFullyLoaded(m_configuration.m_descriptorListAsset.Get()->m_descriptors);
            }
        }
        else
        {
            return IsFullyLoaded(m_configuration.m_descriptors);
        }

        return false;
    }

    DescriptorListSourceType DescriptorListComponent::GetDescriptorListSourceType() const
    {
        return m_configuration.GetDescriptorListSourceType();
    }

    void DescriptorListComponent::SetDescriptorListSourceType(DescriptorListSourceType sourceType)
    {
        m_configuration.SetDescriptorListSourceType(sourceType);
    }

    AZStd::string DescriptorListComponent::GetDescriptorAssetPath() const
    {
        return m_configuration.GetDescriptorAssetPath();
    }

    void DescriptorListComponent::SetDescriptorAssetPath(const AZStd::string& assetPath)
    {
        m_configuration.m_descriptorListAsset.Release();
        DescriptorNotificationBus::MultiHandler::BusDisconnect();
        ReleaseUniqueDescriptors();
        m_configuration.SetDescriptorAssetPath(assetPath);
        LoadAssets();
    }

    size_t DescriptorListComponent::GetNumDescriptors() const
    {
        return m_configuration.GetNumDescriptors();
    }

    Descriptor* DescriptorListComponent::GetDescriptor(int index)
    {
        return m_configuration.GetDescriptor(index);
    }

    void DescriptorListComponent::RemoveDescriptor(int index)
    {
        m_configuration.RemoveDescriptor(index);
        LoadAssets();
    }

    void DescriptorListComponent::SetDescriptor(int index, Descriptor* descriptor)
    {
        m_configuration.SetDescriptor(index, descriptor);
        LoadAssets();
    }

    void DescriptorListComponent::AddDescriptor(Descriptor* descriptor)
    {
        m_configuration.AddDescriptor(descriptor);
        LoadAssets();
    }
}
