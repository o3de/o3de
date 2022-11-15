/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;

    class PrefabLinkDomTestFixture
        : public PrefabTestFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        // Object to store data about template used for tests.
        TemplateData m_templateData;

        // Link used for testing DOM operations.
        AZStd::unique_ptr<Link> m_link;
        
    };
} // namespace UnitTest
