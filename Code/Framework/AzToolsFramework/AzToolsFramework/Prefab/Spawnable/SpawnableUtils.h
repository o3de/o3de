/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>

namespace AzToolsFramework::Prefab::SpawnableUtils
{
    bool CreateSpawnable(AzFramework::Spawnable& spawnable, const PrefabDom& prefabDom);
    bool CreateSpawnable(AzFramework::Spawnable& spawnable, const PrefabDom& prefabDom, AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets);

    void SortEntitiesByTransformHierarchy(AzFramework::Spawnable& spawnable);

    template <typename EntityPtr>
    void SortEntitiesByTransformHierarchy(AZStd::vector<EntityPtr>& entities);

} // namespace AzToolsFramework::Prefab::SpawnableUtils
