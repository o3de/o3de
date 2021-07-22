/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <MockPrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace PrefabDependencyViewer
{
    using PrefabDom    = AzToolsFramework::Prefab::PrefabDom;
    using PrefabDomMap = AZStd::unordered_map<AZStd::string, PrefabDom>;

    struct PrefabDependencyViewerFixture : public UnitTest::ScopedAllocatorSetupFixture
    {
        PrefabDependencyViewerFixture()
            : m_prefabDomsCases(PrefabDomMap())
            , m_prefabSystemComponent(aznew MockPrefabSystemComponent())
            {}

        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();

            // Setup for an invalid empty prefab
            m_prefabDomsCases["emptyJSON"] = PrefabDom();
            m_prefabDomsCases["emptyJSON"].SetObject();

            // Setup for a root level Prefab with only Source Attribute
            CreatePrefabAddSourceAndValue("emptyJSONWithSource", "Prefabs/emptySavedJSON.prefab");

            // Setup for root level Prefab with nested instances but one of the nested instances is missing source
            CreatePrefabAddSourceAndValue("NestedPrefabWithAtleastOneInvalidNestedInstance", "Prefabs/Root.prefab");
            CreatePrefabAddSourceAndValue("GoodNestedPrefab", "Prefabs/goodPrefab.prefab");
            CreatePrefabAddSourceAndValue("BadNestedPrefab", "");
            AddInstance("NestedPrefabWithAtleastOneInvalidNestedInstance", "GoodNestedPrefab");
            AddInstance("NestedPrefabWithAtleastOneInvalidNestedInstance", "BadNestedPrefab");

            // Setup for valid nested Prefab
            CreatePrefabAddSourceAndValue("ValidPrefab", "Prefabs/ValidPrefab.prefab");
            CreatePrefabAddSourceAndValue("level11Prefab", "Prefabs/level11.prefab");
            CreatePrefabAddSourceAndValue("level12Prefab", "Prefabs/level12.prefab");
            CreatePrefabAddSourceAndValue("level13Prefab", "Prefabs/level13.prefab");
            AddInstance("ValidPrefab", "level11Prefab");
            AddInstance("ValidPrefab", "level12Prefab");
            AddInstance("ValidPrefab", "level13Prefab");
        }

        void CreatePrefabAddSourceAndValue(AZStd::string prefabName, const char* prefabSource)
        {
            const char* sourceKey = AzToolsFramework::Prefab::PrefabDomUtils::SourceName;
            m_prefabDomsCases[prefabName] = PrefabDom();
            m_prefabDomsCases[prefabName].SetObject();

            auto& allocator = m_prefabDomsCases[prefabName].GetAllocator();
            rapidjson::Value key(sourceKey, allocator);

            rapidjson::Value value(prefabSource, allocator);

            m_prefabDomsCases[prefabName].AddMember(key, value, allocator);
        }

        void AddInstance(AZStd::string root, AZStd::string child)
        {
            const char* sourceKey = AzToolsFramework::Prefab::PrefabDomUtils::SourceName;
            const char* instancesName = AzToolsFramework::Prefab::PrefabDomUtils::InstancesName;

            auto& allocator = m_prefabDomsCases[root].GetAllocator();

            if (m_prefabDomsCases[root].HasMember(instancesName))
            {
                AddInstance(m_prefabDomsCases[root][instancesName], m_prefabDomsCases[child][sourceKey].GetString(), allocator);
            }
            else
            {
                rapidjson::Value instancesKey(instancesName, allocator);

                rapidjson::Value instancesValue;
                instancesValue.SetObject();

                AddInstance(instancesValue, m_prefabDomsCases[child][sourceKey].GetString(), allocator);
                m_prefabDomsCases[root].AddMember(instancesKey, instancesValue, allocator);
            }

        }

        void AddInstance(rapidjson::Value& instancesValue, const char* nestedInstanceSource, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            static int counter = 0;
            const char* sourceKey = AzToolsFramework::Prefab::PrefabDomUtils::SourceName;

            // Instance alias can be anything as long as they are unique.
            // So using counter as a unique name for it.
            rapidjson::Value nestedInstanceAliasKey(AZStd::to_string(counter).c_str(), allocator);

            rapidjson::Value nestedInstanceAliasValue;
            nestedInstanceAliasValue.SetObject();

            {
                rapidjson::Value nestedInstanceSourceKey(sourceKey, allocator);
                rapidjson::Value nestedInstanceSourceValue(nestedInstanceSource, allocator);

                nestedInstanceAliasValue.AddMember(nestedInstanceSourceKey, nestedInstanceSourceValue, allocator);
            }

            instancesValue.AddMember(nestedInstanceAliasKey, nestedInstanceAliasValue, allocator);
            ++counter;
        }

        void TearDown() override
        {
            delete m_prefabSystemComponent;
        }

        PrefabDomMap m_prefabDomsCases;
        MockPrefabSystemComponent* m_prefabSystemComponent;
        const TemplateId InvalidTemplateId = AzToolsFramework::Prefab::InvalidTemplateId;
    };
} // namespace PrefabDependencyViewer
