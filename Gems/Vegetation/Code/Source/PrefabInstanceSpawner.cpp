/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Vegetation/PrefabInstanceSpawner.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Vegetation/AreaComponentBase.h>
#include <Vegetation/InstanceData.h>
#include <Vegetation/Ebuses/DescriptorNotificationBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>



namespace Vegetation
{

    PrefabInstanceSpawner::PrefabInstanceSpawner()
    {
        UnloadAssets();
    }

    PrefabInstanceSpawner::~PrefabInstanceSpawner()
    {
        UnloadAssets();
        AZ_Assert(m_instanceTickets.empty(), "Destroying spawner while %u spawn tickets still exist!", m_instanceTickets.size());
    }

    void PrefabInstanceSpawner::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<PrefabInstanceSpawner, InstanceSpawner>()
                ->Version(0)->Field(
                "SpawnableAsset", &PrefabInstanceSpawner::m_spawnableAsset)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<PrefabInstanceSpawner>(
                    "Prefab", "Prefab Instance")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &PrefabInstanceSpawner::m_spawnableAsset, "Prefab Asset", "Prefab asset")
                    ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, false)
                    ->Attribute(AZ::Edit::Attributes::HideProductFilesInAssetPicker, true)
                    ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "a Prefab")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PrefabInstanceSpawner::SpawnableAssetChanged)
                    ;
            }
        }
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PrefabInstanceSpawner>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Constructor()
                ->Method("GetPrefabAssetPath", &PrefabInstanceSpawner::GetSpawnableAssetPath)
                ->Method("SetPrefabAssetPath", &PrefabInstanceSpawner::SetSpawnableAssetPath)
                ->Method("GetPrefabAssetId", &PrefabInstanceSpawner::GetSpawnableAssetId)
                ->Method("SetPrefabAssetId", &PrefabInstanceSpawner::SetSpawnableAssetId);
        }
    }

    bool PrefabInstanceSpawner::DataIsEquivalent(const InstanceSpawner& baseRhs) const
    {
        if (const auto* rhs = azrtti_cast<const PrefabInstanceSpawner*>(&baseRhs))
        {
            return m_spawnableAsset == rhs->m_spawnableAsset;
        }

        // Not the same subtypes, so definitely not a data match.
        return false;
    }

    void PrefabInstanceSpawner::LoadAssets()
    {
        UnloadAssets();

        // Note that the spawnable tickets manage and track asset loading as well.  We *could* just rely on that and mark
        // the spawner as immediately ready for use (i.e. always return "true" in IsLoaded() and IsSpawnable() ), but this
        // would cause us to wait until the first instance is spawned to load the asset, creating a delay right at the point
        // that the vegetation is becoming visible.  It would also cause the asset to get auto-unloaded every time all the
        // instances using it are despawned. By loading it *prior* to marking things as ready, we can ensure that we have the
        // asset at the point that the first instance is spawned, and that it won't get auto-unloaded every time the instances
        // are despawned.
        m_spawnableAsset.QueueLoad();
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_spawnableAsset.GetId());
    }

    void PrefabInstanceSpawner::UnloadAssets()
    {
        // It's possible under some circumstances that we might unload assets before destroying all spawned instances
        // due to the way the vegetation system queues up delete requests and descriptor unregistrations. If so,
        // despawn the actual spawned instances here, but leave the ticket entries in the instance ticket map and don't
        // delete the ticket pointers. The tickets will get cleaned up when the vegetation system gets around to requesting
        // the instance destroy.
        if (!m_instanceTickets.empty())
        {
            for (auto& ticket : m_instanceTickets)
            {
                DespawnAssetInstance(ticket);
            }
        }
        ResetSpawnableAsset();
        NotifyOnAssetsUnloaded();
    }

    void PrefabInstanceSpawner::ResetSpawnableAsset()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();

        m_spawnableAsset.Release();
        UpdateCachedValues();
        m_spawnableAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::QueueLoad);
    }

    void PrefabInstanceSpawner::UpdateCachedValues()
    {
        // Once our assets are loaded and at the point that they're getting registered,
        // cache off the spawnable state for use from multiple threads.

        m_assetLoadedAndSpawnable = m_spawnableAsset.IsReady();
    }

    void PrefabInstanceSpawner::OnRegisterUniqueDescriptor()
    {
        UpdateCachedValues();
    }

    void PrefabInstanceSpawner::OnReleaseUniqueDescriptor()
    {
    }

    bool PrefabInstanceSpawner::HasEmptyAssetReferences() const
    {
        // If we don't have a valid Spawnable Asset, then that means we're expecting to spawn empty instances.
        return !m_spawnableAsset.GetId().IsValid();
    }

    bool PrefabInstanceSpawner::IsLoaded() const
    {
        return m_assetLoadedAndSpawnable;
    }

    bool PrefabInstanceSpawner::IsSpawnable() const
    {
        return m_assetLoadedAndSpawnable;
    }

    AZStd::string PrefabInstanceSpawner::GetName() const
    {
        AZStd::string assetName;
        if (!HasEmptyAssetReferences())
        {
            // Get the asset file name
            assetName = m_spawnableAsset.GetHint();
            if (!m_spawnableAsset.GetHint().empty())
            {
                AzFramework::StringFunc::Path::GetFileName(m_spawnableAsset.GetHint().c_str(), assetName);
            }
        }
        else
        {
            assetName = "<asset name>";
        }

        return assetName;
    }

    bool PrefabInstanceSpawner::ValidateAssetContents(const AZ::Data::Asset<AZ::Data::AssetData> asset) const
    {
        bool validAsset = true;

        // Basic safety check:  Make sure the asset is a spawnable.
        auto spawnableAsset = azrtti_cast<AzFramework::Spawnable*>(asset.GetData());
        if (!spawnableAsset)
        {
            return false;
        }

        // Loop through all the components on all the entities in the spawnable, looking for any type of Vegetation Area.
        // If we try to dynamically spawn vegetation areas, as they spawn in they will non-deterministically start spawning
        // (or blocking) other vegetation while we're in the midst of spawning the higher-level vegetation area.  Threading
        // and timing affects which one wins out.  It may also cause other bugs.

        const AzFramework::Spawnable::EntityList& entities = spawnableAsset->GetEntities();
        for (auto& entity : entities)
        {
            auto components = entity->GetComponents();
            for (auto component : components)
            {
                if (azrtti_istypeof<AreaComponentBase*>(component))
                {
                    validAsset = false;
                    AZ_Error("Vegetation", false,
                             "Vegetation system cannot spawn prefabs containing a component of type '%s'",
                             component->RTTI_GetTypeName());
                }
            }
        }

        return validAsset;
    }

    void PrefabInstanceSpawner::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_spawnableAsset.GetId() == asset.GetId())
        {
            // Make sure that the spawnable asset we're loading doesn't contain any data incompatible with
            // the dynamic vegetation system.
            // This check needs to be performed at asset loading time as opposed to authoring / configuration
            // time because the spawnable asset can be changed independently from the authoring of this component.
            bool validAsset = ValidateAssetContents(asset);

            ResetSpawnableAsset();
            if (validAsset)
            {
                m_spawnableAsset = asset;
            }
            UpdateCachedValues();
            NotifyOnAssetsLoaded();
        }
    }

    void PrefabInstanceSpawner::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    AZStd::string PrefabInstanceSpawner::GetSpawnableAssetPath() const
    {
        AZStd::string assetPathString;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_spawnableAsset.GetId());
        return assetPathString;
    }

    void PrefabInstanceSpawner::SetSpawnableAssetPath(const AZStd::string& assetPath)
    {
        if (!assetPath.empty())
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, assetPath.c_str(),
                AZ::Data::s_invalidAssetType, false);
            if (assetId.IsValid())
            {
                SetSpawnableAssetId(assetId);
            }
            else
            {
                AZ_Error("Vegetation", false, "Asset '%s' is invalid.", assetPath.c_str());
            }
        }
        else
        {
            SetSpawnableAssetId(AZ::Data::AssetId());
        }
    }

    AZ::Data::AssetId PrefabInstanceSpawner::GetSpawnableAssetId() const
    {
        return m_spawnableAsset.GetId();
    }

    void PrefabInstanceSpawner::SetSpawnableAssetId(const AZ::Data::AssetId& assetId)
    {
        if (assetId.IsValid())
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
            if (assetInfo.m_assetType == m_spawnableAsset.GetType())
            {
                m_spawnableAsset.Create(assetId, false);
                LoadAssets();
            }
            else
            {
                AZ_Error(
                    "Vegetation", false, "Asset '%s' is of type %s, but expected a Spawnable type.",
                    assetId.ToString<AZStd::string>().c_str(), assetInfo.m_assetType.ToString<AZStd::string>().c_str());
            }
        }
        else
        {
            // An invalid asset ID is treated as a valid way to spawn "empty" instances, so don't print an error, just clear out
            // the asset to that it has an invalid asset reference.  (See also HasEmptyAssetReferences() above)
            m_spawnableAsset = AZ::Data::Asset<AzFramework::Spawnable>();
            LoadAssets();
        }
    }

    AZ::u32 PrefabInstanceSpawner::SpawnableAssetChanged()
    {
        // Whenever we change the spawnable asset, force a refresh of the Entity Inspector
        // since we want the Descriptor List to refresh the name of the entry.
        NotifyOnAssetsUnloaded();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    InstancePtr PrefabInstanceSpawner::CreateInstance(const InstanceData& instanceData)
    {
        InstancePtr opaqueInstanceData = nullptr;

        // Create a Transform that represents our instance.
        AZ::Transform world = AZ::Transform::CreateFromQuaternionAndTranslation(
            instanceData.m_alignment * instanceData.m_rotation, instanceData.m_position);
        world.MultiplyByUniformScale(instanceData.m_scale);

        // Create a callback for SpawnAllEntities that will set the transform of the root entity to the correct position / rotation / scale
        // for our spawned instance.
        auto preSpawnCB = [world](
             [[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId, AzFramework::SpawnableEntityContainerView view)
        {
            AZ::Entity* rootEntity = *view.begin();

            AzFramework::TransformComponent* entityTransform = rootEntity->FindComponent<AzFramework::TransformComponent>();

            if (entityTransform)
            {
                entityTransform->SetWorldTM(world);
            }
        };

        // Create the EntitySpawnTicket here.  This pointer is going to get handed off to the vegetation system as opaque instance data,
        // where it will be tracked and held onto for the lifetime of the vegetation instance.  The vegetation system will pass it back
        // in to DestroyInstance at the end of the lifetime, so that's the one place where we will delete the ticket pointers.
        AzFramework::EntitySpawnTicket* ticket = new AzFramework::EntitySpawnTicket(m_spawnableAsset);
        if (ticket->IsValid())
        {
            // Track the ticket that we've created.
            m_instanceTickets.emplace(ticket);

            AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
            optionalArgs.m_preInsertionCallback = AZStd::move(preSpawnCB);
            AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(*ticket, AZStd::move(optionalArgs));

            opaqueInstanceData = ticket;
        }
        else
        {
            // Something went wrong!
            AZ_Assert(ticket->IsValid(), "Unable to instantiate spawnable asset");
            delete ticket;
        }

        return opaqueInstanceData;
    }

    void PrefabInstanceSpawner::DespawnAssetInstance(AzFramework::EntitySpawnTicket* ticket)
    {
        if (ticket->IsValid())
        {
            AzFramework::SpawnableEntitiesInterface::Get()->DespawnAllEntities(*ticket);
        }
    }

    void PrefabInstanceSpawner::DestroyInstance([[maybe_unused]] InstanceId id, InstancePtr instance)
    {
        if (instance)
        {
            auto ticket = reinterpret_cast<AzFramework::EntitySpawnTicket*>(instance);

            // If the spawnable asset instantiated successfully, we should have a record of it.
            auto foundInstance = m_instanceTickets.find(ticket);
            AZ_Assert(foundInstance != m_instanceTickets.end(), "Couldn't find CreateInstance entry for the EntitySpawnTicket.");
            if (foundInstance != m_instanceTickets.end())
            {
                // The call to DespawnAssetInstance above is technically redundant right now, because when we delete the ticket pointer
                // below it will automatically despawn everything anyways. However, it's nice to have a single explicit call to despawn,
                // in case we ever need a place to add logging, or have a callback when despawning is complete, etc.
                DespawnAssetInstance(ticket);
                m_instanceTickets.erase(foundInstance);
            }

            // The vegetation system has stopped tracking this instance, so it's now safe to delete the ticket pointer.
            delete ticket;
        }
    }
} // namespace Vegetation
