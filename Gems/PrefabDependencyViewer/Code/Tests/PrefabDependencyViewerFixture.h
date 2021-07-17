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
            
            m_prefabDomsCases["emptyUnsavedJSON"] = PrefabDom();
            m_prefabDomsCases["emptySavedJSON"] = PrefabDom();

            for (AZStd::pair<AZStd::string, PrefabDom>& p : m_prefabDomsCases)
            {
                p.second.SetObject();
            }
            
            rapidjson::Value emptySavedJSONSourceKey(
                AzToolsFramework::Prefab::PrefabDomUtils::SourceName,
                m_prefabDomsCases["emptySavedJSON"].GetAllocator());

            rapidjson::Value emptySavedJSONSourceValue(
                "Prefabs/emptySavedJSON.prefab",
                m_prefabDomsCases["emptySavedJSON"].GetAllocator());

            m_prefabDomsCases["emptySavedJSON"].AddMember(
                emptySavedJSONSourceKey,
                emptySavedJSONSourceValue,
                m_prefabDomsCases["emptySavedJSON"].GetAllocator()
            );
            
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
