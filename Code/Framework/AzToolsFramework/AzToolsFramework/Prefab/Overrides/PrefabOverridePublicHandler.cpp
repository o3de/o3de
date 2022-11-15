/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicHandler.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabOverridePublicHandler::PrefabOverridePublicHandler()
        {
            AZ::Interface<PrefabOverridePublicInterface>::Register(this);

            m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface, "PrefabOverridePublicHandler - InstanceToTemplateInterface could not be found.");

            m_prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            AZ_Assert(m_prefabFocusInterface, "PrefabOverridePublicHandler - PrefabFocusInterface could not be found.");

            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface, "PrefabOverridePublicHandler - PrefabSystemComponentInterface could not be found.");
        }

        PrefabOverridePublicHandler::~PrefabOverridePublicHandler()
        {
            AZ::Interface<PrefabOverridePublicInterface>::Unregister(this);
        }

        bool PrefabOverridePublicHandler::AreOverridesPresent(AZ::EntityId entityId)
        {
            AZStd::pair<AZ::Dom::Path, LinkId> pathAndLinkIdPair = GetPathAndLinkIdFromFocusedPrefab(entityId);
            if (!pathAndLinkIdPair.first.IsEmpty() && pathAndLinkIdPair.second != InvalidLinkId)
            {
                return m_prefabOverrideHandler.AreOverridesPresent(pathAndLinkIdPair.first, pathAndLinkIdPair.second);
            }
            return false;
        }

        bool PrefabOverridePublicHandler::RevertOverrides(AZ::EntityId entityId)
        {
            AZStd::pair<AZ::Dom::Path, LinkId> pathAndLinkIdPair = GetPathAndLinkIdFromFocusedPrefab(entityId);
            if (!pathAndLinkIdPair.first.IsEmpty() && pathAndLinkIdPair.second != InvalidLinkId)
            {
                return m_prefabOverrideHandler.RevertOverrides(pathAndLinkIdPair.first, pathAndLinkIdPair.second);
            }
            return false;
        }

        AZStd::pair<AZ::Dom::Path, LinkId> PrefabOverridePublicHandler::GetPathAndLinkIdFromFocusedPrefab(AZ::EntityId entityId)
        {
            AzFramework::EntityContextId editorEntityContextId;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                editorEntityContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);
            InstanceOptionalReference focusedInstance = m_prefabFocusInterface->GetFocusedPrefabInstance(editorEntityContextId);

            AZ::Dom::Path absoluteEntityAliasPath = m_instanceToTemplateInterface->GenerateEntityPathFromFocusedPrefab(entityId);

            // The first 2 tokens of the path will represent the path of the instance below the focused prefab.
            // Eg: "Instances/InstanceA/Instances/InstanceB/....'. The override tree doesn't store the topmost instance to avoid
            // redundant checks Eg: "Instances/InstanceB/....' . So we skip the first 2 tokens here.
            if (focusedInstance.has_value() && absoluteEntityAliasPath.size() > 2)
            {
                AZStd::string_view topMostInstanceKey = absoluteEntityAliasPath[1].GetKey().GetStringView();
                InstanceOptionalReference topMostInstance = focusedInstance->get().FindNestedInstance(topMostInstanceKey);
                if (topMostInstance.has_value())
                {
                    auto pathIterator = absoluteEntityAliasPath.begin() + 2;
                    return AZStd::pair(AZ::Dom::Path(pathIterator, absoluteEntityAliasPath.end()), topMostInstance->get().GetLinkId());
                }
            }
            return AZStd::pair(AZ::Dom::Path(), InvalidLinkId);
        }
    } // namespace Prefab
} // namespace AzToolsFramework
