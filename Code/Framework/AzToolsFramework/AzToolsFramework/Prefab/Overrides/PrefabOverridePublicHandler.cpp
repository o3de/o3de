/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicHandler.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
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
            PrefabOverridePublicRequestBus::Handler::BusConnect();

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
            PrefabOverridePublicRequestBus::Handler::BusDisconnect();
        }

        void PrefabOverridePublicHandler::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context); behaviorContext)
            {
                behaviorContext->EBus<PrefabOverridePublicRequestBus>("PrefabOverridePublicRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Prefab")
                    ->Attribute(AZ::Script::Attributes::Module, "prefab")
                    ->Event("AreOverridesPresent", &PrefabOverridePublicInterface::AreOverridesPresent)
                    ->Event("RevertOverrides", &PrefabOverridePublicInterface::RevertOverrides);
            }
        }

        bool PrefabOverridePublicHandler::AreOverridesPresent(AZ::EntityId entityId, AZStd::string_view relativePathFromEntity)
        {
            AZStd::pair<AZ::Dom::Path, LinkId> pathAndLinkIdPair = GetEntityPathAndLinkIdFromFocusedPrefab(entityId);
            if (!pathAndLinkIdPair.first.IsEmpty() && pathAndLinkIdPair.second != InvalidLinkId)
            {
                if (!relativePathFromEntity.empty())
                {
                    pathAndLinkIdPair.first /= AZ::Dom::Path(relativePathFromEntity);
                }
                return m_prefabOverrideHandler.AreOverridesPresent(pathAndLinkIdPair.first, pathAndLinkIdPair.second);
            }

            return false;
        }

        AZStd::optional<OverrideType> PrefabOverridePublicHandler::GetEntityOverrideType(AZ::EntityId entityId)
        {
            AZStd::optional<OverrideType> overrideType = {};

            AZStd::pair<AZ::Dom::Path, LinkId> pathAndLinkIdPair = GetEntityPathAndLinkIdFromFocusedPrefab(entityId);
            if (!pathAndLinkIdPair.first.IsEmpty() && pathAndLinkIdPair.second != InvalidLinkId)
            {
                AZStd::optional<PatchType> patchType = m_prefabOverrideHandler.GetPatchType(pathAndLinkIdPair.first, pathAndLinkIdPair.second);
                if (patchType.has_value())
                {
                    switch (patchType.value())
                    {
                    case PatchType::Add:
                        overrideType = OverrideType::AddEntity;
                        break;
                    case PatchType::Remove:
                        overrideType = OverrideType::RemoveEntity;
                        break;
                    case PatchType::Edit:
                        overrideType = OverrideType::EditEntity;
                        break;
                    default:
                        break;
                    }
                }
            }

            return overrideType;
        }

        AZStd::optional<OverrideType> PrefabOverridePublicHandler::GetComponentOverrideType(const AZ::EntityComponentIdPair& entityComponentIdPair)
        {
            AZStd::optional<OverrideType> overrideType = {};

            AZStd::pair<AZ::Dom::Path, LinkId> pathAndLinkIdPair = GetComponentPathAndLinkIdFromFocusedPrefab(entityComponentIdPair);
            if (!pathAndLinkIdPair.first.IsEmpty() && pathAndLinkIdPair.second != InvalidLinkId)
            {
                AZStd::optional<PatchType> patchType = m_prefabOverrideHandler.GetPatchType(pathAndLinkIdPair.first, pathAndLinkIdPair.second);
                if (patchType.has_value())
                {
                    switch (patchType.value())
                    {
                    case PatchType::Add:
                        overrideType = OverrideType::AddComponent;
                        break;
                    case PatchType::Remove:
                        overrideType = OverrideType::RemoveComponent;
                        break;
                    case PatchType::Edit:
                        overrideType = OverrideType::EditComponent;
                        break;
                    default:
                        break;
                    }
                }
            }

            return overrideType;
        }

        bool PrefabOverridePublicHandler::RevertOverrides(AZ::EntityId entityId, AZStd::string_view relativePathFromEntity)
        {
            AZStd::pair<AZ::Dom::Path, LinkId> pathAndLinkIdPair = GetEntityPathAndLinkIdFromFocusedPrefab(entityId);
            if (!pathAndLinkIdPair.first.IsEmpty() && pathAndLinkIdPair.second != InvalidLinkId)
            {
                if (!relativePathFromEntity.empty())
                {
                    pathAndLinkIdPair.first /= AZ::Dom::Path(relativePathFromEntity);
                }
                return m_prefabOverrideHandler.RevertOverrides(pathAndLinkIdPair.first, pathAndLinkIdPair.second);
            }
            return false;
        }

        bool PrefabOverridePublicHandler::RevertComponentOverrides(const AZ::EntityComponentIdPair& entityComponentIdPair)
        {
            AZStd::pair<AZ::Dom::Path, LinkId> pathAndLinkIdPair = GetComponentPathAndLinkIdFromFocusedPrefab(entityComponentIdPair);
            if (!pathAndLinkIdPair.first.IsEmpty() && pathAndLinkIdPair.second != InvalidLinkId)
            {
                return m_prefabOverrideHandler.RevertOverrides(pathAndLinkIdPair.first, pathAndLinkIdPair.second);
            }
            return false;
        }

        AZStd::pair<AZ::Dom::Path, LinkId> PrefabOverridePublicHandler::GetEntityPathAndLinkIdFromFocusedPrefab(AZ::EntityId entityId)
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

        AZStd::pair<AZ::Dom::Path, LinkId> PrefabOverridePublicHandler::GetComponentPathAndLinkIdFromFocusedPrefab(
            const AZ::EntityComponentIdPair& entityComponentIdPair)
        {
            AZStd::pair<AZ::Dom::Path, LinkId> pathAndLinkIdPair;

            // Find entity owned by instance
            AZ::EntityId entityId = entityComponentIdPair.GetEntityId();
            EntityOptionalConstReference entity;
            if (auto instanceEntityMapperInterface = AZ::Interface<Prefab::InstanceEntityMapperInterface>::Get())
            {
                if (InstanceOptionalReference owningInstance = instanceEntityMapperInterface->FindOwningInstance(entityId);
                    owningInstance.has_value())
                {
                    if (EntityAliasOptionalReference entityAlias =
                            owningInstance->get().GetEntityAlias(entityId);
                        entityAlias.has_value())
                    {
                        entity = owningInstance->get().GetEntity(entityAlias->get());
                    }
                }
            }

            if (entity.has_value())
            {
                if (AZ::Component* component = entity->get().FindComponent(entityComponentIdPair.GetComponentId()))
                {
                    pathAndLinkIdPair = GetEntityPathAndLinkIdFromFocusedPrefab(entityId);
                    if (!pathAndLinkIdPair.first.IsEmpty() && pathAndLinkIdPair.second != InvalidLinkId)
                    {
                        pathAndLinkIdPair.first /= PrefabDomUtils::ComponentsName;
                        pathAndLinkIdPair.first /= component->GetSerializedIdentifier();
                    }
                }
                else
                {
                    AZ_Warning(
                        "PrefabOverridePublicHandler",
                        false,
                        "FindComponent failed - could not find component pointer from componentId provided.");
                }
            }
            else
            {
                AZ_Warning(
                    "PrefabOverridePublicHandler",
                    false,
                    "FindEntity failed - could not find entity pointer from entityId provided.");
            }

            return pathAndLinkIdPair;
        }
    } // namespace Prefab
} // namespace AzToolsFramework
