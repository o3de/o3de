/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/PrefabDomTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabUndoUtils
        {
            void GenerateAddEntityPatch(
                PrefabDom& patch,
                PrefabDom& newEntityDom,
                const AZStd::string& newEntityAliasPath);

            void GenerateRemoveEntityPatch(
                PrefabDom& patch,
                const AZStd::string& targetEntityAliasPath);

            void UpdateCachedOwningInstanceDom(
                PrefabDomReference cachedOwningInstanceDom,
                const PrefabDom& entityDom,
                const AZStd::string& entityAliasPath);
        } // namespace PrefabUndoUtils
    } // namespace Prefab
} // namespace AzToolsFramework

