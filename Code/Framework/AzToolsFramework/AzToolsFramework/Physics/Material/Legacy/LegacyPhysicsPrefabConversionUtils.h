/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

 // O3DE_DEPRECATION_NOTICE(GHI-9840)
 // Utilities used for legacy material conversion.

namespace Physics::Utils
{
    struct PrefabInfo
    {
        AzToolsFramework::Prefab::TemplateId m_templateId;
        AzToolsFramework::Prefab::Template* m_template = nullptr;
        AZStd::string m_prefabFullPath;
    };

    AZStd::vector<PrefabInfo> CollectPrefabs();

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetPrefabEntities(AzToolsFramework::Prefab::PrefabDom& prefab);

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetEntityComponents(AzToolsFramework::Prefab::PrefabDomValue& entity);

    AZ::TypeId GetComponentTypeId(const AzToolsFramework::Prefab::PrefabDomValue& component);

    AzToolsFramework::Prefab::PrefabDomValue* FindMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, AzToolsFramework::Prefab::PrefabDomValue& prefabComponent);

    const AzToolsFramework::Prefab::PrefabDomValue* FindMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, const AzToolsFramework::Prefab::PrefabDomValue& prefabComponent);

    void RemoveMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, AzToolsFramework::Prefab::PrefabDomValue& prefabComponent);

    template<class T>
    bool LoadObjectFromPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, const AzToolsFramework::Prefab::PrefabDomValue& prefabComponent, T& object)
    {
        const auto* member = FindMemberChainInPrefabComponent(memberChain, prefabComponent);
        if (!member)
        {
            return false;
        }

        auto result = AZ::JsonSerialization::Load(&object, azrtti_typeid<T>(), *member);

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
    }

    template<class T>
    bool StoreObjectToPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain,
        AzToolsFramework::Prefab::PrefabDom& prefabDom,
        AzToolsFramework::Prefab::PrefabDomValue& prefabComponent,
        const T& object)
    {
        auto* member = FindMemberChainInPrefabComponent(memberChain, prefabComponent);
        if (!member)
        {
            return false;
        }

        T defaultObject;

        auto result = AZ::JsonSerialization::Store(*member, prefabDom.GetAllocator(), &object, &defaultObject, azrtti_typeid<T>());

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
    }
} // namespace Physics::Utils
