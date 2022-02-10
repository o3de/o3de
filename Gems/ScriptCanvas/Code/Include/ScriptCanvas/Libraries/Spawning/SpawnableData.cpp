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
#include <ScriptCanvas/Libraries/Spawning/SpawnableData.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    void SpawnableData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->Class<SpawnableData>()
                ->Field("SpawnableAsset", &SpawnableData::m_spawnableAsset)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SpawnableData::OnSpawnAssetChanged)
                ->Field("EntityIndices", &SpawnableData::m_entityIndices);

            {
                if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<SpawnableData>>::GetGenericInfo())
                {
                    genericClassInfo->Reflect(serializeContext);
                }
            }
            {
                if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::unordered_map<AZStd::string, SpawnableData>>::GetGenericInfo())
                {
                    genericClassInfo->Reflect(serializeContext);
                }
            }

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<SpawnableData>("SpawnData", "A wrapper around spawnable asset to be used as a variable in Script Canvas.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::Category, "Spawning")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Placeholder.png")
                    // m_spawnableAsset
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnableData::m_spawnableAsset, "m_spawnableAsset", "")
                    ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, false)
                    ->Attribute(AZ::Edit::Attributes::HideProductFilesInAssetPicker, true)
                    ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "a Prefab")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SpawnableData::OnSpawnAssetChanged)
                    // m_entityIndices
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnableData::m_entityIndices, "m_entityIndices", "");
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SpawnableData>("SpawnableData")
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Spawning")
                ->Attribute(AZ::Script::Attributes::Module, "Spawning")
                ->Property("m_spawnableAsset", BehaviorValueProperty(&SpawnableData::m_spawnableAsset));

            behaviorContext
                ->Class<ContainerTypeReflection::BehaviorClassReflection<SpawnableData>>(
                    AZStd::string::format("ReflectOnDemandTargets_%s", ScriptCanvas::Data::Traits<SpawnableData>::GetName().data()).data())
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Ignore, true)
                ->Method(
                    "ReflectVector",
                    [](const AZStd::vector<SpawnableData>&){})
                ->Method(
                    "Map_String_to_SpawnData_Func",
                    [](const AZStd::unordered_map<AZStd::string, SpawnableData>&)
                    {
                    });
        }

        ContainerTypeReflection::HashContainerReflector<SpawnableData>::Reflect(context);
    }

    SpawnableData::SpawnableData()
    {
    }

    SpawnableData::SpawnableData(const SpawnableData& rhs)
        : m_spawnableAsset(rhs.m_spawnableAsset)
        , m_entityIndices(rhs.m_entityIndices)
        , m_ticket(rhs.m_ticket)
    {
    }

    SpawnableData& SpawnableData::operator=(const SpawnableData& rhs)
    {
        m_spawnableAsset = rhs.m_spawnableAsset;
        m_entityIndices = rhs.m_entityIndices;
        m_ticket = rhs.m_ticket;
        return *this;
    }

    void SpawnableData::OnSpawnAssetChanged()
    {
        if (m_spawnableAsset.GetId().IsValid())
        {
            AZStd::string rootSpawnableFile;
            AzFramework::StringFunc::Path::GetFileName(m_spawnableAsset.GetHint().c_str(), rootSpawnableFile);

            rootSpawnableFile += AzFramework::Spawnable::DotFileExtension;

            AZ::u32 rootSubId = AzFramework::SpawnableAssetHandler::BuildSubId(AZStd::move(rootSpawnableFile));

            if (m_spawnableAsset.GetId().m_subId != rootSubId)
            {
                AZ::Data::AssetId rootAssetId = m_spawnableAsset.GetId();
                rootAssetId.m_subId = rootSubId;

                m_spawnableAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AzFramework::Spawnable>(
                    rootAssetId, AZ::Data::AssetLoadBehavior::Default);
            }
            else
            {
                m_spawnableAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::Default);
            }
        }

        UpdateTicket();
    }

    void SpawnableData::UpdateTicket()
    {
        if (!m_spawnableAsset.GetId().IsValid())
        {
            m_ticket.reset();
        }
        else
        {
            m_ticket = AZStd::make_shared<AzFramework::EntitySpawnTicket>(m_spawnableAsset);
        }
    }
}
