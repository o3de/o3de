/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <ScriptCanvas/Libraries/Core/ContainerTypeReflection.h>
#include <ScriptCanvas/Libraries/Spawning/SpawnableAsset.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    void SpawnableAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->Class<SpawnableAsset>()
                ->Field("Asset", &SpawnableAsset::m_asset);

            {
                if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<SpawnableAsset>>::GetGenericInfo())
                {
                    genericClassInfo->Reflect(serializeContext);
                }
            }
            {
                if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::unordered_map<
                    AZStd::string, SpawnableAsset>>::GetGenericInfo())
                {
                    genericClassInfo->Reflect(serializeContext);
                }
            }

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<SpawnableAsset>("SpawnableAsset", "A wrapper around spawnable asset to be used as a variable in Script Canvas.")
                    // m_asset
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnableAsset::m_asset, "m_asset", "")
                    ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, false)
                    ->Attribute(AZ::Edit::Attributes::HideProductFilesInAssetPicker, true)
                    ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "Spawnable Asset")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SpawnableAsset::OnSpawnAssetChanged);
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext
                ->Class<SpawnableAsset>("SpawnableAsset")
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Spawning")
                ->Attribute(AZ::Script::Attributes::Module, "Spawning")
                ->Property("m_asset", BehaviorValueProperty(&SpawnableAsset::m_asset));

            behaviorContext
                ->Class<ContainerTypeReflection::BehaviorClassReflection<SpawnableAsset>>(
                    AZStd::string::format("ReflectOnDemandTargets_%s", Data::Traits<SpawnableAsset>::GetName().data()).data())
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Ignore, true)
                ->Method(
                    "ReflectVector",
                    [](const AZStd::vector<SpawnableAsset>&)
                    {
                    })
                ->Method(
                    "Map_String_to_SpawnableAsset_Func",
                    [](const AZStd::unordered_map<AZStd::string, SpawnableAsset>&)
                    {
                    });
        }

        ContainerTypeReflection::HashContainerReflector<SpawnableAsset>::Reflect(context);
    }

    SpawnableAsset::SpawnableAsset(const SpawnableAsset& rhs)
        : m_asset(rhs.m_asset)
    {
    }

    SpawnableAsset& SpawnableAsset::operator=(const SpawnableAsset& rhs)
    {
        m_asset = rhs.m_asset;
        return *this;
    }

    void SpawnableAsset::OnSpawnAssetChanged()
    {
        if (m_asset.GetId().IsValid())
        {
            AZStd::string rootSpawnableFile;
            AzFramework::StringFunc::Path::GetFileName(m_asset.GetHint().c_str(), rootSpawnableFile);

            rootSpawnableFile += AzFramework::Spawnable::DotFileExtension;

            AZ::u32 rootSubId = AzFramework::SpawnableAssetHandler::BuildSubId(move(rootSpawnableFile));

            if (m_asset.GetId().m_subId != rootSubId)
            {
                AZ::Data::AssetId rootAssetId = m_asset.GetId();
                rootAssetId.m_subId = rootSubId;

                m_asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AzFramework::Spawnable>(
                    rootAssetId, AZ::Data::AssetLoadBehavior::Default);
            }
            else
            {
                m_asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::Default);
            }
        }
    }
}
