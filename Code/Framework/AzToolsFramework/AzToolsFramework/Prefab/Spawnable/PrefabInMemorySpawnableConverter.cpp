/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/Prefab/PrefabLoader.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabConverterStackProfileNames.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabInMemorySpawnableConverter.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabToInMemorySpawnableNotificationBus.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableAssetBus.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    PrefabInMemorySpawnableConverter::~PrefabInMemorySpawnableConverter()
    {
        Deactivate();
    }

    bool PrefabInMemorySpawnableConverter::Activate(AZStd::string_view stackProfile)
    {
        AZ_Assert(!IsActivated(),
            "PrefabInMemorySpawnableConverter - Unable to activate an instance of PrefabInMemorySpawnableConverter as the instance is already active.");

        m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        AZ_Assert(m_prefabSystemComponentInterface, "PrefabInMemorySpawnableConverter - Could not retrieve instance of PrefabSystemComponentInterface");

        m_loaderInterface = AZ::Interface<Prefab::PrefabLoaderInterface>::Get();
        AZ_Assert(m_loaderInterface, "PrefabInMemorySpawnableConverter - Could not retrieve instance of PrefabLoaderInterface");

        m_stackProfile = stackProfile;
        return m_converter.LoadStackProfile(m_stackProfile);
    }

    void PrefabInMemorySpawnableConverter::Deactivate()
    {
        m_assetContainer.ClearAllInMemorySpawnableAssets();
        m_stackProfile = "";
        m_loaderInterface = nullptr;
        m_prefabSystemComponentInterface = nullptr;
    }

    bool PrefabInMemorySpawnableConverter::IsActivated() const
    {
        return m_converter.IsLoaded();
    }

    AZStd::string_view PrefabInMemorySpawnableConverter::GetStackProfile() const
    {
        return m_stackProfile;
    }

    AzFramework::InMemorySpawnableAssetContainer::CreateSpawnableResult PrefabInMemorySpawnableConverter::CreateInMemorySpawnableAsset(
        AzToolsFramework::Prefab::TemplateId templateId, AZStd::string_view spawnableName, bool loadReferencedAssets)
    {
        if (!IsActivated())
        {
            return AZ::Failure(AZStd::string::format("Failed to create a prefab processing stack from key '%.*s'.", AZ_STRING_ARG(m_stackProfile)));
        }

        if (m_assetContainer.HasInMemorySpawnableAsset(spawnableName))
        {
            return AZ::Failure(AZStd::string::format("In-memory Spawnable '%.*s' already exists.", AZ_STRING_ARG(spawnableName)));
        }

        TemplateReference templateReference = m_prefabSystemComponentInterface->FindTemplate(templateId);
        if (!templateReference.has_value())
        {
            return AZ::Failure(AZStd::string::format("Could not get Template DOM for given Template's id %llu .", templateId));
        }

        // Use a random uuid as this is only a temporary source.
        PrefabConversionUtils::PrefabProcessorContext context(AZ::Uuid::CreateRandom());
        PrefabDocument document(spawnableName);
        document.SetPrefabDom(templateReference->get().GetPrefabDom());
        context.AddPrefab(AZStd::move(document));
        m_converter.ProcessPrefab(context);

        if (!context.HasCompletedSuccessfully() || context.GetProcessedObjects().empty())
        {
            return AZ::Failure(AZStd::string::format(
                "Failed to convert the prefab into assets. Please confirm that the '%.*s' prefab processor stack is capable of producing a usable product asset.",
                AZ_STRING_ARG(PrefabConversionUtils::IntegrationTests)));
        }

        // Convert products to in-memory spawnable assets
        AzFramework::InMemorySpawnableAssetContainer::AssetDataInfoContainer assetDataInfoContainer;
        for (auto& product : context.GetProcessedObjects())
        {
            AZ::Data::AssetInfo info;
            info.m_assetId = product.GetAsset().GetId();
            info.m_assetType = product.GetAssetType();
            info.m_relativePath = product.GetId();
            AZ::Data::AssetData* assetData = product.ReleaseAsset().release();
            assetDataInfoContainer.emplace_back(AZStd::make_pair(assetData, info));

            if (info.m_assetType == azrtti_typeid<AzFramework::Spawnable>())
            {
                auto spawnable = azrtti_cast<AzFramework::Spawnable*>(assetData);

                PrefabToInMemorySpawnableNotificationBus::Broadcast(&PrefabToInMemorySpawnableNotifications::OnPreparingInMemorySpawnableFromPrefab,
                    *spawnable, info.m_relativePath);
            }
        }

        return m_assetContainer.CreateInMemorySpawnableAsset(assetDataInfoContainer, loadReferencedAssets, spawnableName);
    }

    AzFramework::InMemorySpawnableAssetContainer::CreateSpawnableResult PrefabInMemorySpawnableConverter::CreateInMemorySpawnableAsset(
        AZStd::string_view prefabFilePath, AZStd::string_view spawnableName, bool loadReferencedAssets)
    {
        AZ::IO::Path relativePath = m_loaderInterface->GenerateRelativePath(prefabFilePath);
        auto templateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(relativePath);
        if (templateId == InvalidTemplateId)
        {
            return AZ::Failure(AZStd::string::format("Template with source path '%.*s' is not found.", AZ_STRING_ARG(prefabFilePath)));
        }

        return CreateInMemorySpawnableAsset(templateId, spawnableName, loadReferencedAssets);
    }

    const AzFramework::InMemorySpawnableAssetContainer& PrefabInMemorySpawnableConverter::GetAssetContainerConst() const
    {
        return m_assetContainer;
    }

    AzFramework::InMemorySpawnableAssetContainer& PrefabInMemorySpawnableConverter::GetAssetContainer()
    {
        return m_assetContainer;
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
