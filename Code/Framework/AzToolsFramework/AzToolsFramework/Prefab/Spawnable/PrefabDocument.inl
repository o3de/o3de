/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    template<typename Component>
    void PrefabDocument::ListEntitiesWithComponentType(const AZStd::function<bool(AzToolsFramework::Prefab::AliasPath&&)>& callback) const
    {
        ListEntitiesWithComponentType(azrtti_typeid<Component>(), callback);
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
