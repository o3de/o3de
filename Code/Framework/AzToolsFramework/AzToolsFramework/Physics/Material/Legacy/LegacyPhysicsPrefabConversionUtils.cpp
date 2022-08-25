/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/Spawnable/Spawnable.h>

#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsMaterialConversionUtils.h>
#include <AzToolsFramework/Physics/Material/Legacy/LegacyPhysicsPrefabConversionUtils.h>

namespace Physics::Utils
{
    AZStd::vector<PrefabInfo> CollectPrefabs()
    {
        AZStd::vector<PrefabInfo> prefabsWithLegacyMaterials;

        auto* prefabLoader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
        auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&prefabsWithLegacyMaterials, prefabLoader,
            prefabSystemComponent](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType != AzFramework::Spawnable::RTTI_Type())
            {
                return;
            }

            AZStd::optional<AZStd::string> assetFullPath = GetFullSourceAssetPathById(assetId);
            if (!assetFullPath.has_value())
            {
                return;
            }

            if (auto templateId = prefabLoader->LoadTemplateFromFile(assetFullPath->c_str());
                templateId != AzToolsFramework::Prefab::InvalidTemplateId)
            {
                if (auto templateResult = prefabSystemComponent->FindTemplate(templateId); templateResult.has_value())
                {
                    AzToolsFramework::Prefab::Template& templateRef = templateResult->get();
                    prefabsWithLegacyMaterials.push_back({ templateId, &templateRef, AZStd::move(*assetFullPath) });
                }
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, assetEnumerationCB, nullptr);

        return prefabsWithLegacyMaterials;
    }

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetPrefabEntities(AzToolsFramework::Prefab::PrefabDom& prefab)
    {
        if (!prefab.IsObject())
        {
            return {};
        }

        AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> entities;

        if (auto entitiesIter = prefab.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::EntitiesName);
            entitiesIter != prefab.MemberEnd() && entitiesIter->value.IsObject())
        {
            entities.reserve(entitiesIter->value.MemberCount());

            for (auto entityIter = entitiesIter->value.MemberBegin(); entityIter != entitiesIter->value.MemberEnd(); ++entityIter)
            {
                if (entityIter->value.IsObject())
                {
                    entities.push_back(&entityIter->value);
                }
            }
        }

        return entities;
    }

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetEntityComponents(AzToolsFramework::Prefab::PrefabDomValue& entity)
    {
        AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> components;

        if (auto componentsIter = entity.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::ComponentsName);
            componentsIter != entity.MemberEnd() && componentsIter->value.IsObject())
        {
            components.reserve(componentsIter->value.MemberCount());

            for (auto componentIter = componentsIter->value.MemberBegin(); componentIter != componentsIter->value.MemberEnd();
                ++componentIter)
            {
                if (!componentIter->value.IsObject())
                {
                    continue;
                }

                components.push_back(&componentIter->value);
            }
        }

        return components;
    }

    AZ::TypeId GetComponentTypeId(const AzToolsFramework::Prefab::PrefabDomValue& component)
    {
        const auto typeFieldIter = component.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::TypeName);
        if (typeFieldIter == component.MemberEnd())
        {
            return AZ::TypeId::CreateNull();
        }

        AZ::TypeId typeId = AZ::TypeId::CreateNull();
        AZ::JsonSerialization::LoadTypeId(typeId, typeFieldIter->value);

        return typeId;
    }

    AzToolsFramework::Prefab::PrefabDomValue* FindMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        if (memberChain.empty())
        {
            return nullptr;
        }

        auto memberIter = prefabComponent.FindMember(memberChain[0].c_str());
        if (memberIter == prefabComponent.MemberEnd())
        {
            return nullptr;
        }

        for (size_t i = 1; i < memberChain.size(); ++i)
        {
            auto memberFoundIter = memberIter->value.FindMember(memberChain[i].c_str());
            if (memberFoundIter == memberIter->value.MemberEnd())
            {
                return nullptr;
            }
            memberIter = memberFoundIter;
        }
        return &memberIter->value;
    }

    const AzToolsFramework::Prefab::PrefabDomValue* FindMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, const AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        return FindMemberChainInPrefabComponent(memberChain, const_cast<AzToolsFramework::Prefab::PrefabDomValue&>(prefabComponent));
    }

    void RemoveMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        if (memberChain.empty())
        {
            return;
        }
        else if (memberChain.size() == 1)
        {
            // If the member chain is only one member, just remove it from the component.
            prefabComponent.RemoveMember(memberChain[0].c_str());
            return;
        }

        // If the chain is 2 or more members, find the member previous to last.

        auto memberIter = prefabComponent.FindMember(memberChain[0].c_str());
        if (memberIter == prefabComponent.MemberEnd())
        {
            return;
        }

        for (size_t i = 1; i < memberChain.size() - 1; ++i)
        {
            auto memberFoundIter = memberIter->value.FindMember(memberChain[i].c_str());
            if (memberFoundIter == memberIter->value.MemberEnd())
            {
                return;
            }
            memberIter = memberFoundIter;
        }

        // Remove the last member in the chain.
        memberIter->value.RemoveMember(memberChain.back().c_str());
    }
} // namespace Physics::Utils
