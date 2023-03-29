/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabUndoUtils
        {
            void AppendAddEntityPatch(
                PrefabDom& patches,
                const PrefabDomValue& newEntityDom,
                const AZStd::string& newEntityAliasPath)
            {
                AZ_Assert(patches.IsArray(), "AppendAddEntityPatch - Provided patches should be an array object DOM value.");

                AppendUpdateValuePatch(patches, newEntityDom, newEntityAliasPath, PatchType::Add);
            }

            void AppendUpdateValuePatch(
                PrefabDom& patches,
                const PrefabDomValue& domValue,
                const AZStd::string& pathToUpdate,
                const PatchType patchType)
            {
                AZ_Assert(patches.IsArray(), "AppendUpdateValuePatch - Provided patches should be an array object DOM value.");

                PrefabDomValue patch(rapidjson::kObjectType);
                rapidjson::Value path = rapidjson::Value(
                    pathToUpdate.data(), aznumeric_caster(pathToUpdate.length()), patches.GetAllocator());
                rapidjson::Value value;
                value.CopyFrom(domValue, patches.GetAllocator(), true);
                if (patchType == PatchType::Add)
                {
                    patch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("add"), patches.GetAllocator());
                }
                else if (patchType == PatchType::Edit)
                {
                    patch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("replace"), patches.GetAllocator());
                }
                else
                {
                    AZ_Assert(false, "AppendUpdateValuePatch - Unsupported operation type.");
                }
                patch.AddMember(rapidjson::StringRef("path"), AZStd::move(path), patches.GetAllocator())
                    .AddMember(rapidjson::StringRef("value"), AZStd::move(value), patches.GetAllocator());
                patches.PushBack(patch.Move(), patches.GetAllocator());
            }

            void AppendRemovePatch(
                PrefabDom& patches,
                const AZStd::string& pathToRemove)
            {
                AZ_Assert(patches.IsArray(), "AppendRemovePatch - Provided patches should be an array object DOM value.");

                PrefabDomValue removeTargetEntityPatch(rapidjson::kObjectType);
                rapidjson::Value path = rapidjson::Value(pathToRemove.data(),
                    aznumeric_caster(pathToRemove.length()), patches.GetAllocator());
                removeTargetEntityPatch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("remove"), patches.GetAllocator())
                    .AddMember(rapidjson::StringRef("path"), AZStd::move(path), patches.GetAllocator());
                patches.PushBack(removeTargetEntityPatch.Move(), patches.GetAllocator());
            }

            void GenerateAndAppendPatch(
                PrefabDom& patches,
                const PrefabDomValue& domValueBeforeUpdate,
                const PrefabDomValue& domValueAfterUpdate,
                const AZStd::string& pathToValue)
            {
                AZ_Assert(patches.IsArray(), "GenerateAndAppendPatch - Provided patches should be an array object DOM value.");

                auto instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
                AZ_Assert(instanceToTemplateInterface, "GenerateAndAppendPatch - Could not get InstanceToTemplateInterface.");

                PrefabDom newPatches(&(patches.GetAllocator()));
                instanceToTemplateInterface->GeneratePatch(newPatches, domValueBeforeUpdate, domValueAfterUpdate);
                instanceToTemplateInterface->PrependPathToPatchPaths(newPatches, pathToValue);

                for (auto& newPatch : newPatches.GetArray())
                {
                    patches.PushBack(newPatch.Move(), patches.GetAllocator());
                }
            }

            void UpdateEntityInPrefabDom(
                PrefabDomReference prefabDom, const PrefabDomValue& entityDom, const AZStd::string& entityAliasPath)
            {
                UpdateValueInPrefabDom(prefabDom, entityDom, entityAliasPath);
            }

            void UpdateValueInPrefabDom(
                PrefabDomReference prefabDom, const PrefabDomValue& domValue, const AZStd::string& pathToValue)
            {
                if (!pathToValue.empty())
                {
                    PrefabDomValue endStateCopy(domValue, prefabDom->get().GetAllocator());

                    PrefabDomPath domPathToValue(pathToValue.c_str());
                    domPathToValue.Set(prefabDom->get(), endStateCopy.Move());
                }
            }

            void RemoveValueInPrefabDom(PrefabDomReference prefabDom, const AZStd::string& pathToRemove)
            {
                if (!pathToRemove.empty())
                {
                    PrefabDomPath domPathToRemove(pathToRemove.c_str());
                    domPathToRemove.Erase(prefabDom->get());
                }
            }

        } // namespace PrefabUndoUtils
    } // namespace Prefab
} // namespace AzToolsFramework
