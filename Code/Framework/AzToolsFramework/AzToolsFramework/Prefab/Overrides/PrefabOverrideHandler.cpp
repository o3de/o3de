/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideHandler.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

 namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace Internal
        {
            // Helper to differentiate between adding/removing
            // an entity object or adding/removing an entity property
            bool IsDirectEntityPatch(PrefabDomConstReference patch)
            {
                auto pathMember = patch->get().FindMember("path");
                if (pathMember != patch->get().MemberEnd())
                {
                    AZ::Dom::Path path(pathMember->value.GetString());
                    if (path.Size() > 1)
                    {
                        return (path[path.Size() - 2] == PrefabDomUtils::EntitiesName);
                    }
                }

                return false;
            }
        }

        bool PrefabOverrideHandler::AreOverridesPresent(AZ::Dom::Path path, LinkId linkId)
        {
            PrefabSystemComponentInterface* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            if (prefabSystemComponentInterface != nullptr)
            {
                LinkReference link = prefabSystemComponentInterface->FindLink(linkId);
                if (link.has_value())
                {
                    return link->get().AreOverridesPresent(path);
                }
            }
            return false;
        }

        AZStd::optional<EntityOverrideType> PrefabOverrideHandler::GetOverrideType(AZ::Dom::Path path, LinkId linkId)
        {
            AZStd::optional<EntityOverrideType> overrideType = {};

            PrefabSystemComponentInterface* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            if (prefabSystemComponentInterface != nullptr)
            {
                LinkReference link = prefabSystemComponentInterface->FindLink(linkId);
                if (link.has_value())
                {
                    // Look for an override in the provided path
                    if (PrefabDomConstReference overridePatch = link->get().FindOverridePatch(path); overridePatch.has_value())
                    {
                        PrefabDomValue::ConstMemberIterator patchEntryIterator = overridePatch->get().FindMember("op");
                        if (patchEntryIterator != overridePatch->get().MemberEnd())
                        {
                            AZStd::string opPath = patchEntryIterator->value.GetString();
                            if (opPath == "remove")
                            {
                                overrideType = Internal::IsDirectEntityPatch(overridePatch)
                                    ? EntityOverrideType::RemoveEntity : EntityOverrideType::EditEntity;
                            }
                            else if (opPath == "add")
                            {
                                overrideType = Internal::IsDirectEntityPatch(overridePatch)
                                    ? EntityOverrideType::AddEntity : EntityOverrideType::EditEntity;
                            }
                            else if (opPath == "replace")
                            {
                                overrideType = EntityOverrideType::EditEntity;
                            }
                        }
                    }
                }
            }

            return overrideType;
        }
    } // namespace Prefab
} // namespace AzToolsFramework
