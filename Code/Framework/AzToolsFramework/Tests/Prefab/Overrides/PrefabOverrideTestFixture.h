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

    class PrefabOverrideTestFixture : public PrefabTestFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;

        AZStd::pair<AZ::EntityId, AZ::EntityId> CreateEntityInNestedPrefab();
        void CreateAndValidateOverride(AZ::EntityId entityId);

    PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;
    };
} // namespace UnitTest
