/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/Link/PrefabLinkDomTestFixture.h>
#include <Prefab/MockPrefabFileIOActionValidator.h>
#include <Prefab/PrefabTestDomUtils.h>

namespace UnitTest
{
    void PrefabLinkDomTestFixture::SetUpEditorFixtureImpl()
    {
        PrefabTestFixture::SetUpEditorFixtureImpl();

        m_templateData.m_filePath = "PathToSourceTemplate";

        MockPrefabFileIOActionValidator mockIOActionValidator;
        mockIOActionValidator.ReadPrefabDom(m_templateData.m_filePath, PrefabTestDomUtils::CreatePrefabDom());

        m_templateData.m_id = m_prefabLoaderInterface->LoadTemplateFromFile(m_templateData.m_filePath);

        m_link = AZStd::make_unique<Link>(0);
        m_link->SetTargetTemplateId(1);
        m_link->SetSourceTemplateId(m_templateData.m_id);
        m_link->SetInstanceName("SomeInstanceName");
    }

    void PrefabLinkDomTestFixture::TearDownEditorFixtureImpl()
    {
        m_link.reset();
        PrefabTestFixture::TearDownEditorFixtureImpl();
    }
} // namespace UnitTest
