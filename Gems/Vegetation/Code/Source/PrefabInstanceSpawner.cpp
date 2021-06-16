/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "Vegetation_precompiled.h"
#include <Vegetation/PrefabInstanceSpawner.h>

#include <AzCore/Asset/AssetManager.h>
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
                ;
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
                    AZ_Error("Vegetation", false, "Asset '%s' is of type %s, but expected a Spawnable type.",
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

        auto preSpawnCB = [this, world](
                              [[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId, AzFramework::SpawnableEntityContainerView view)
        {
            AZ::Entity* rootEntity = *view.begin();

            AzFramework::TransformComponent* entityTransform = rootEntity->FindComponent<AzFramework::TransformComponent>();

            if (entityTransform)
            {
                entityTransform->SetWorldTM(world);
            }
        };

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
            AZ_Assert(foundInstance != m_instanceTickets.end(), "Couldn't find CreateInstance entry for slice instance");
            if (foundInstance != m_instanceTickets.end())
            {
                DespawnAssetInstance(ticket);
                m_instanceTickets.erase(foundInstance);
            }

            delete ticket;
        }
    }
} // namespace Vegetation
