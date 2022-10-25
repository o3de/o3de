/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabUndoUtils
        {
            void GenerateAddEntityPatch(
                PrefabDom& patch,
                PrefabDom& newEntityDom,
                const AZStd::string& newEntityAliasPath)
            {
                PrefabDomValue addNewEntityPatch(rapidjson::kObjectType);
                rapidjson::Value path = rapidjson::Value(newEntityAliasPath.data(),
                    aznumeric_caster(newEntityAliasPath.length()), patch.GetAllocator());
                rapidjson::Value patchValue;
                patchValue.CopyFrom(newEntityDom, patch.GetAllocator(), true);
                addNewEntityPatch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("add"), patch.GetAllocator())
                    .AddMember(rapidjson::StringRef("path"), AZStd::move(path), patch.GetAllocator())
                    .AddMember(rapidjson::StringRef("value"), AZStd::move(patchValue), patch.GetAllocator());
                patch.PushBack(addNewEntityPatch.Move(), patch.GetAllocator());
            }

            void GenerateRemoveEntityPatch(
                PrefabDom& patch,
                const AZStd::string& targetEntityAliasPath)
            {
                PrefabDomValue removeTargetEntityPatch(rapidjson::kObjectType);
                rapidjson::Value path = rapidjson::Value(targetEntityAliasPath.data(),
                    aznumeric_caster(targetEntityAliasPath.length()), patch.GetAllocator());
                removeTargetEntityPatch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("remove"), patch.GetAllocator())
                    .AddMember(rapidjson::StringRef("path"), AZStd::move(path), patch.GetAllocator());
                patch.PushBack(removeTargetEntityPatch.Move(), patch.GetAllocator());
            }

            void UpdateCachedOwningInstanceDom(
                PrefabDomReference cachedOwningInstanceDom,
                const PrefabDom& entityDom,
                const AZStd::string& entityAliasPath)
            {
                // Create a copy of the DOM of the end state so that it shares the lifecycle of the cached DOM.
                PrefabDom endStateCopy;
                endStateCopy.CopyFrom(entityDom, cachedOwningInstanceDom->get().GetAllocator());
                PrefabDomPath entityPathInDom(entityAliasPath.c_str());

                // Update the cached instance DOM corresponding to the entity so that the same modified entity isn't reloaded again.
                entityPathInDom.Set(cachedOwningInstanceDom->get(), AZStd::move(endStateCopy));
            }
        } // namespace PrefabUndoUtils
    } // namespace Prefab
} // namespace AzToolsFramework
