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

            void AppendUpdateValuePatchByComparison(
                PrefabDom& patches,
                const PrefabDomValue& domValueBeforeUpdate,
                const PrefabDomValue& domValueAfterUpdate,
                const AZStd::string& pathToValue)
            {
                AZ_Assert(patches.IsArray(), "AppendUpdateValuePatchByComparison - Provided patches should be an array object DOM value.");

                auto instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
                AZ_Assert(instanceToTemplateInterface, "AppendUpdateValuePatchByComparison - Could not get InstanceToTemplateInterface.");

                PrefabDom newPatches(&(patches.GetAllocator()));
                instanceToTemplateInterface->GeneratePatch(newPatches, domValueBeforeUpdate, domValueAfterUpdate);
                instanceToTemplateInterface->AppendEntityAliasPathToPatchPaths(newPatches, pathToValue);

                for (auto& newPatch : newPatches.GetArray())
                {
                    patches.PushBack(newPatch.Move(), patches.GetAllocator());
                }
            }

            void UpdateValueInInstanceDom(
                PrefabDomReference instanceDom, const PrefabDomValue& domValue, const AZStd::string& pathToValue)
            {
                if (!pathToValue.empty())
                {
                    PrefabDomValue endStateCopy(domValue, instanceDom->get().GetAllocator());

                    PrefabDomPath domPathToValue(pathToValue.c_str());
                    domPathToValue.Set(instanceDom->get(), endStateCopy.Move());
                }
            }

            void RemoveValueInInstanceDom(PrefabDomReference instanceDom, const AZStd::string& pathToRemove)
            {
                if (!pathToRemove.empty())
                {
                    PrefabDomPath domPathToRemove(pathToRemove.c_str());
                    domPathToRemove.Erase(instanceDom->get());
                }
            }

        } // namespace PrefabUndoUtils
    } // namespace Prefab
} // namespace AzToolsFramework
