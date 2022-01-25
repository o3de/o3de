/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Vegetation/DynamicSliceInstanceSpawner.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Vegetation/Ebuses/DescriptorNotificationBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/InstanceData.h>

#include <AzCore/Math/Transform.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Entity/SliceGameEntityOwnershipServiceBus.h>

#include <Vegetation/AreaComponentBase.h>
#include <LmbrCentral/Scripting/SpawnerComponentBus.h>

namespace Vegetation
{

    DynamicSliceInstanceSpawner::DynamicSliceInstanceSpawner()
    {
        UnloadAssets();
    }

    DynamicSliceInstanceSpawner::~DynamicSliceInstanceSpawner()
    {
        UnloadAssets();
        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect();
        AZ_Assert(m_ticketToEntityMap.empty(), "Destroying spawner while instances still exist!");
    }

    void DynamicSliceInstanceSpawner::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DynamicSliceInstanceSpawner, InstanceSpawner>()
                ->Version(0)
                ->Field("SliceAsset", &DynamicSliceInstanceSpawner::m_sliceAsset)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<DynamicSliceInstanceSpawner>(
                    "Dynamic Slice", "Dynamic Slice Instance")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(0, &DynamicSliceInstanceSpawner::m_sliceAsset, "Slice Asset", "Dynamic slice asset")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &DynamicSliceInstanceSpawner::SliceAssetChanged)
                    ;
            }
        }
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DynamicSliceInstanceSpawner>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Constructor()
                ->Method("GetSliceAssetPath", &DynamicSliceInstanceSpawner::GetSliceAssetPath)
                ->Method("SetSliceAssetPath", &DynamicSliceInstanceSpawner::SetSliceAssetPath)
                ;
        }
    }

    bool DynamicSliceInstanceSpawner::DataIsEquivalent(const InstanceSpawner & baseRhs) const
    {
        if (const auto* rhs = azrtti_cast<const DynamicSliceInstanceSpawner*>(&baseRhs))
        {
            return m_sliceAsset == rhs->m_sliceAsset;
        }

        // Not the same subtypes, so definitely not a data match.
        return false;
    }

    void DynamicSliceInstanceSpawner::LoadAssets()
    {
        UnloadAssets();

        m_sliceAsset.QueueLoad();
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_sliceAsset.GetId());
    }

    void DynamicSliceInstanceSpawner::UnloadAssets()
    {
        // It's possible under some circumstances that we might unload assets before
        // destroying all spawned instances due to the way the vegetation system queues
        // up delete requests and descriptor unregistrations. If so, delete the actual
        // spawned instances here, but leave the ticket entries in the slice ticket map.
        // The ticket entries will get cleaned up when the vegetation system gets around
        // to requesting the instance destroy.
        if (!m_ticketToEntityMap.empty())
        {
            for (auto& entry : m_ticketToEntityMap)
            {
                DeleteSliceInstance(entry.first, entry.second);
                entry.second = AZ::EntityId();
            }
        }

        ResetSliceAsset();
        NotifyOnAssetsUnloaded();
    }

    void DynamicSliceInstanceSpawner::ResetSliceAsset()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();

        m_sliceAsset.Release();
        UpdateCachedValues();
        m_sliceAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::QueueLoad);
    }

    void DynamicSliceInstanceSpawner::UpdateCachedValues()
    {
        // Once our assets are loaded and at the point that they're getting registered,
        // cache off the spawnable state for use from multiple threads.

        m_sliceLoadedAndSpawnable = m_sliceAsset.IsReady();
    }

    void DynamicSliceInstanceSpawner::OnRegisterUniqueDescriptor()
    {
        UpdateCachedValues();
    }

    void DynamicSliceInstanceSpawner::OnReleaseUniqueDescriptor()
    {
    }

    bool DynamicSliceInstanceSpawner::HasEmptyAssetReferences() const
    {
        // If we don't have a valid Slice Asset, then we're spawning empty instances.
        return !m_sliceAsset.GetId().IsValid();
    }

    bool DynamicSliceInstanceSpawner::IsLoaded() const
    {
        return m_sliceLoadedAndSpawnable;
    }

    bool DynamicSliceInstanceSpawner::IsSpawnable() const
    {
        return m_sliceLoadedAndSpawnable;
    }

    AZStd::string DynamicSliceInstanceSpawner::GetName() const
    {
        AZStd::string assetName;
        if (!HasEmptyAssetReferences())
        {
            // Get the asset file name
            assetName = m_sliceAsset.GetHint();
            if (!m_sliceAsset.GetHint().empty())
            {
                AzFramework::StringFunc::Path::GetFileName(m_sliceAsset.GetHint().c_str(), assetName);
            }
        }
        else
        {
            assetName = "<asset name>";
        }

        return assetName;
    }

    bool DynamicSliceInstanceSpawner::ValidateSliceContents(const AZ::Data::Asset<AZ::Data::AssetData> asset) const
    {
        bool validSlice = true;

        // Basic safety check:  Make sure the asset is a dynamic slice.
        auto sliceAsset = azrtti_cast<AZ::DynamicSliceAsset*>(asset.GetData());
        if (!sliceAsset)
        {
            return false;
        }

        // Make sure the dynamic slice has a slice component.
        auto slice = sliceAsset->GetComponent();
        if (!slice)
        {
            return false;
        }

        // Loop through all the components on all the entities in the slice, looking for the following
        // incompatible components:
        // 1) Any type of Vegetation Area.  If we try to dynamically spawn vegetation areas, as they spawn
        // in they will non-deterministically start spawning other vegetation where we're trying to spawn
        // vegetation areas.  Threading and timing affects which one wins out.  It may also cause other bugs.
        // 2) Gameplay Spawner components.  These can spawn dynamic slices with vegetation areas, which leads
        // back to problem #1, but in a way that's even harder to detect.  Also, if "destroy on deactivate" is
        // unselected on the component, they will spawn entities that continue to remain in the level even after
        // the dynamic vegetation system destroys the spawner.

        AZ::SliceComponent::EntityList entities;
        slice->GetEntities(entities);
        for (auto entity : entities)
        {
            auto components = entity->GetComponents();
            for (auto component : components)
            {
                if (azrtti_istypeof<AreaComponentBase*>(component) ||
                    (azrtti_typeid(component) == LmbrCentral::SpawnerComponentTypeId))
                {
                    validSlice = false;
                    AZ_Error("Vegetation", false,
                             "Vegetation system cannot spawn dynamic slices containing a component of type '%s'",
                             component->RTTI_GetTypeName());
                }
            }
        }

        return validSlice;
    }

    void DynamicSliceInstanceSpawner::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_sliceAsset.GetId() == asset.GetId())
        {
            // Make sure that the slice we're loading doesn't contain any data incompatible with
            // the dynamic vegetation system.
            // This check needs to be performed at slice loading time as opposed to authoring / configuration
            // time because the slice can be changed independently from the authoring of this component.
            bool validSlice = ValidateSliceContents(asset);

            ResetSliceAsset();
            if (validSlice)
            {
                m_sliceAsset = asset;
            }
            UpdateCachedValues();
            NotifyOnAssetsLoaded();
        }
    }

    void DynamicSliceInstanceSpawner::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    AZStd::string DynamicSliceInstanceSpawner::GetSliceAssetPath() const
    {
        AZStd::string assetPathString;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_sliceAsset.GetId());
        return assetPathString;
    }

    void DynamicSliceInstanceSpawner::SetSliceAssetPath(const AZStd::string& assetPath)
    {
        if (!assetPath.empty())
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, assetPath.c_str(), AZ::Data::s_invalidAssetType, false);
            if (assetId.IsValid())
            {
                AZ::Data::AssetInfo assetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
                if (assetInfo.m_assetType == m_sliceAsset.GetType())
                {
                    m_sliceAsset.Create(assetId, false);
                    LoadAssets();
                }
                else
                {
                    AZ_Error("Vegetation", false, "Asset '%s' is of type %s, but expected a DynamicSliceAsset type.",
                        assetPath.c_str(), assetInfo.m_assetType.ToString<AZStd::string>().c_str());
                }
            }
            else
            {
                AZ_Error("Vegetation", false, "Asset '%s' is invalid.", assetPath.c_str());
            }
        }
        else
        {
            m_sliceAsset = AZ::Data::Asset<AZ::DynamicSliceAsset>();
            LoadAssets();
        }
    }

    AZ::u32 DynamicSliceInstanceSpawner::SliceAssetChanged()
    {
        // Whenever we change the slice asset, force a refresh of the Entity Inspector
        // since we want the Descriptor List to refresh the name of the entry.
        NotifyOnAssetsUnloaded();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    InstancePtr DynamicSliceInstanceSpawner::CreateInstance(const InstanceData& instanceData)
    {
        InstancePtr opaqueInstanceData = nullptr;

        // Create a Transform that represents our instance.
        AZ::Transform world = AZ::Transform::CreateFromQuaternionAndTranslation(instanceData.m_alignment * instanceData.m_rotation, instanceData.m_position);
        world.MultiplyByUniformScale(instanceData.m_scale);

        // Request a new dynamic slice instance.
        AzFramework::SliceInstantiationTicket* ticket = new AzFramework::SliceInstantiationTicket();
        AzFramework::SliceGameEntityOwnershipServiceRequestBus::BroadcastResult(
            *ticket,
            &AzFramework::SliceGameEntityOwnershipServiceRequestBus::Events::InstantiateDynamicSlice,
            m_sliceAsset,
            world,
            nullptr);

        if (ticket->IsValid())
        {
            // Create entry for ticket, with no entities listed yet.  These will get filled in
            // once the slice is fully instantiated.
            m_ticketToEntityMap.emplace(*ticket);

            // Listen for completion / failure events.
            AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(*ticket);

            opaqueInstanceData = ticket;
        }
        else
        {
            // Something went wrong!
            AZ_Assert(ticket->IsValid(), "Unable to instantiate dynamic slice");
            delete ticket;
        }

        return opaqueInstanceData;
    }

    void DynamicSliceInstanceSpawner::DeleteSliceInstance(const AzFramework::SliceInstantiationTicket &ticket, AZ::EntityId firstEntityInSlice)
    {
        // Stop listening for instantiation events, and cancel the instantiation if it's still in-flight.
        // (If not, the cancel just won't do anything, but there's no harm in calling it anyways.)
        AzFramework::SliceGameEntityOwnershipServiceRequestBus::Broadcast(
            &AzFramework::SliceGameEntityOwnershipServiceRequestBus::Events::CancelDynamicSliceInstantiation, ticket);
        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        // If we have a list of entities from our slice, use the first one to look up and destroy our instance.
        if (firstEntityInSlice.IsValid())
        {
            bool result = false;
            AzFramework::SliceGameEntityOwnershipServiceRequestBus::BroadcastResult(result,
                &AzFramework::SliceGameEntityOwnershipServiceRequestBus::Events::DestroyDynamicSliceByEntity, firstEntityInSlice);
            AZ_Assert(result, "Failed to destroy slice instance.");
        }

    }

    void DynamicSliceInstanceSpawner::DestroyInstance([[maybe_unused]] InstanceId id, InstancePtr instance)
    {
        if (instance)
        {
            auto ticket = reinterpret_cast<AzFramework::SliceInstantiationTicket*>(instance);

            // If this slice instantiated successfully, we should have a record of it.
            auto foundInstance = m_ticketToEntityMap.find(*ticket);
            AZ_Assert(foundInstance != m_ticketToEntityMap.end(), "Couldn't find CreateInstance entry for slice instance");
            if (foundInstance != m_ticketToEntityMap.end())
            {
                AZ::EntityId firstEntityInSlice = foundInstance->second;
                DeleteSliceInstance(*ticket, firstEntityInSlice);
                m_ticketToEntityMap.erase(foundInstance);
            }

            delete ticket;
        }
    }

    void DynamicSliceInstanceSpawner::OnSliceInstantiated([[maybe_unused]] const AZ::Data::AssetId & sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress & sliceAddress)
    {
        const AzFramework::SliceInstantiationTicket ticket = (*AzFramework::SliceInstantiationResultBus::GetCurrentBusId());

        // Stop listening for this ticket (since it's done). We can have have multiple tickets in flight.
        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        // Keep a record of the first entity in our slice instance, we'll need this later to be able to look up
        // and destroy the instance.
        const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;
        AZ_Assert(!entities.empty(), "No entities found in the instantiated slice.");
        if (!entities.empty())
        {
            AZ_Assert(!m_ticketToEntityMap[ticket].IsValid(), "Slice entry already had a valid entity ID");
            m_ticketToEntityMap[ticket] = entities[0]->GetId();
        }
    }

    void DynamicSliceInstanceSpawner::OnSliceInstantiationFailedOrCanceled([[maybe_unused]] const AZ::Data::AssetId & sliceAssetId, bool cancelled)
    {
        const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        if (!cancelled)
        {
            AZ_Error("DynamicSliceInstanceSpawner", false, "Slice %s failed to instantiate", m_sliceAsset.ToString<AZStd::string>().c_str());
            AZ_Assert(sliceAssetId == m_sliceAsset.GetId(), "Current slice asset doesn't match slice that failed to instantiate");
        }
    }

} // namespace Vegetation
