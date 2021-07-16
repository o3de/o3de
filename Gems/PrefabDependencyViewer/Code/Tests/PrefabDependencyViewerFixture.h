/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <MockPrefabSystemComponentInterface.h>

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
            
            m_prefabDomsCases["emptyJSON"] = PrefabDom();

            for (AZStd::pair<AZStd::string, PrefabDom>& p : m_prefabDomsCases)
            {
                p.second.SetObject();
            }
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
