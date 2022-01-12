/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/Spawnable/EntityAliasTypes.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabProcessorContext;

    class PrefabDocument final
    {
    public:
        explicit PrefabDocument(AZStd::string name);
        PrefabDocument(const PrefabDocument&) = delete;
        PrefabDocument(PrefabDocument&&) = default;

        PrefabDocument& operator=(const PrefabDocument&) = delete;
        PrefabDocument& operator=(PrefabDocument&&) = default;

        bool SetPrefabDom(const PrefabDom& prefab);
        bool SetPrefabDom(PrefabDom&& prefab);

        const AZStd::string& GetName() const;
        const PrefabDom& GetDom() const;
        PrefabDom&& TakeDom();

        template<typename Component>
        void ListEntitiesWithComponentType(const AZStd::function<bool(AzToolsFramework::Prefab::AliasPath&&)>& callback) const;
        void ListEntitiesWithComponentType(
            AZ::TypeId componentType, const AZStd::function<bool(AzToolsFramework::Prefab::AliasPath&&)>& callback) const;
        AZ::Entity* CreateEntityAlias(
            PrefabDocument& source,
            AzToolsFramework::Prefab::AliasPathView entity,
            AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasType aliasType,
            AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasSpawnableLoadBehavior loadBehavior,
            uint32_t tag,
            AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context);

        // Where possible, prefer functions directly on the PrefabDocument Instead of using the Instance.
        AzToolsFramework::Prefab::Instance& GetInstance();
        const AzToolsFramework::Prefab::Instance& GetInstance() const;

    private:
        bool ConstructInstanceFromPrefabDom(const PrefabDom& prefab);
        
        mutable PrefabDom m_dom;
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> m_instance;
        AZStd::string m_name;
        mutable bool m_isDirty{ false };
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils

#include <AzToolsFramework/Prefab/Spawnable/PrefabDocument.inl>
