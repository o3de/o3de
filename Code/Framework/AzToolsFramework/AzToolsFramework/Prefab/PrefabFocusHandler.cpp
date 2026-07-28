/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabFocusHandler.h>

#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>
#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>
#include <AzToolsFramework/Prefab/PrefabFocusUndo.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework::Prefab
{
    void PrefabFocusHandler::RegisterPrefabFocusInterface()
    {
        AZ::Interface<PrefabFocusInterface>::Register(this);
        AZ::Interface<PrefabFocusPublicInterface>::Register(this);

        EditorEntityInfoNotificationBus::Handler::BusConnect();
        EditorEntityContextNotificationBus::Handler::BusConnect();
        PrefabPublicNotificationBus::Handler::BusConnect();
        PrefabFocusPublicRequestBus::Handler::BusConnect();

        m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
        AZ_Assert(
            m_instanceEntityMapperInterface,
            "Prefab - PrefabFocusHandler - "
            "Instance Entity Mapper Interface could not be found. "
            "Check that it is being correctly initialized.");

        m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
        AZ_Assert(
            m_instanceToTemplateInterface,
            "Prefab - PrefabFocusHandler - "
            "Instance To Template Interface could not be found. "
            "Check that it is being correctly initialized.");

        m_instanceUpdateExecutorInterface = AZ::Interface<InstanceUpdateExecutorInterface>::Get();
        AZ_Assert(
            m_instanceUpdateExecutorInterface,
            "Prefab - PrefabFocusHandler - "
            "Instance Update Executor Interface could not be found. "
            "Check that it is being correctly initialized.");
    }

    void PrefabFocusHandler::UnregisterPrefabFocusInterface()
    {
        m_instanceUpdateExecutorInterface = nullptr;
        m_instanceToTemplateInterface = nullptr;
        m_instanceEntityMapperInterface = nullptr;

        PrefabFocusPublicRequestBus::Handler::BusDisconnect();
        PrefabPublicNotificationBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();

        AZ::Interface<PrefabFocusPublicInterface>::Unregister(this);
        AZ::Interface<PrefabFocusInterface>::Unregister(this);
    }

    void PrefabFocusHandler::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context); behaviorContext)
        {
            behaviorContext->EBus<PrefabFocusPublicRequestBus>("PrefabFocusPublicRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Prefab")
                ->Attribute(AZ::Script::Attributes::Module, "prefab")
                ->Event("FocusOnOwningPrefab", &PrefabFocusPublicInterface::FocusOnOwningPrefab);
        }
    }

    void PrefabFocusHandler::InitializeEditorInterfaces()
    {
        m_containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();
        AZ_Assert(
            m_containerEntityInterface,
            "Prefab - PrefabFocusHandler - "
            "Container Entity Interface could not be found. "
            "Check that it is being correctly initialized.");

        m_focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
        AZ_Assert(
            m_focusModeInterface,
            "Prefab - PrefabFocusHandler - "
            "Focus Mode Interface could not be found. "
            "Check that it is being correctly initialized.");

        m_readOnlyEntityPublicInterface = AZ::Interface<ReadOnlyEntityPublicInterface>::Get();
        AZ_Assert(
            m_readOnlyEntityPublicInterface,
            "Prefab - PrefabFocusHandler - "
            "ReadOnly Entity Public Interface could not be found. "
            "Check that it is being correctly initialized.");

        m_readOnlyEntityQueryInterface = AZ::Interface<ReadOnlyEntityQueryInterface>::Get();
        AZ_Assert(
            m_readOnlyEntityQueryInterface,
            "Prefab - PrefabFocusHandler - "
            "ReadOnly Entity Query Interface could not be found. "
            "Check that it is being correctly initialized.");

        if (IsOutlinerOverrideManagementEnabled())
        {
            m_prefabEditScope = PrefabEditScope::SHOW_NESTED_INSTANCES_CONTENT;
        }
    }

    //! A null id or the editor entity context id addresses the active world; world ids pass through.
    AzFramework::EntityContextId PrefabFocusHandler::ResolveWorldId(const AzFramework::EntityContextId& entityContextId)
    {
        const bool addressesActiveWorld = entityContextId.IsNull() || entityContextId == GetEntityContextId();
        return addressesActiveWorld ? GetActiveWorldId() : entityContextId;
    }

    PrefabFocusHandler::WorldFocus& PrefabFocusHandler::GetWorldFocus(const AzFramework::EntityContextId& worldId) const
    {
        WorldFocus& focus = m_worldFocus[worldId];
        if (focus.m_rootAliasFocusPathLength == 0)
        {
            PrefabEditorEntityOwnershipInterface* ownershipService = GetWorldOwnershipService(worldId);
            InstanceOptionalReference rootInstance =
                ownershipService ? ownershipService->GetRootPrefabInstance() : InstanceOptionalReference();
            if (rootInstance.has_value())
            {
                focus.m_rootAliasFocusPath = rootInstance->get().GetAbsoluteInstanceAliasPath();
                focus.m_rootAliasFocusPathLength =
                    aznumeric_cast<int>(AZStd::distance(focus.m_rootAliasFocusPath.begin(), focus.m_rootAliasFocusPath.end()));
                RefreshInstanceFocusPath(worldId, focus);
            }
        }
        return focus;
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnOwningPrefab(AZ::EntityId entityId)
    {
        // Initialize Undo Batch object
        ScopedUndoBatch undoBatch("Edit Prefab");

        // Clear selection
        {
            const EntityIdList selectedEntities = EntityIdList{};
            auto selectionUndo = aznew SelectionCommand(selectedEntities, "Clear Selection");
            selectionUndo->SetParent(undoBatch.GetUndoBatch());
            ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::SetSelectedEntities, selectedEntities);
        }

        // Add undo element
        {
            auto editUndo = aznew PrefabFocusUndo("Focus Prefab");
            editUndo->Capture(entityId);
            editUndo->SetParent(undoBatch.GetUndoBatch());
            FocusOnPrefabInstanceOwningEntityId(entityId);
        }

        return AZ::Success();
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnParentOfFocusedPrefab(
        AzFramework::EntityContextId entityContextId)
    {
        const AzFramework::EntityContextId worldId = ResolveWorldId(entityContextId);
        WorldFocus& focus = GetWorldFocus(worldId);

        // If only one instance is in the hierarchy, this operation is invalid
        if (focus.m_rootAliasFocusPathLength <= 1)
        {
            return AZ::Failure(AZStd::string(
                "Prefab Focus Handler: Could not complete FocusOnParentOfFocusedPrefab operation while focusing on the root."));
        }

        RootAliasPath parentPath = focus.m_rootAliasFocusPath;
        parentPath.RemoveFilename();

        // Retrieve parent of currently focused prefab.
        InstanceOptionalReference parentInstance = GetInstanceReference(worldId, parentPath);

        // If only one instance is in the hierarchy, this operation is invalid
        if (!parentInstance.has_value())
        {
            return AZ::Failure(AZStd::string(
                "Prefab Focus Handler: Could not retrieve parent of current focus in FocusOnParentOfFocusedPrefab."));
        }

        // Use container entity of parent Instance for focus operations.
        AZ::EntityId entityId = parentInstance->get().GetContainerEntityId();

        // Initialize Undo Batch object
        ScopedUndoBatch undoBatch("Edit Prefab");

        // Clear selection
        {
            const EntityIdList selectedEntities = EntityIdList{};
            auto selectionUndo = aznew SelectionCommand(selectedEntities, "Clear Selection");
            selectionUndo->SetParent(undoBatch.GetUndoBatch());
            ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::SetSelectedEntities, selectedEntities);
        }

        // Add undo element
        {
            auto editUndo = aznew PrefabFocusUndo("Focus Prefab");
            editUndo->Capture(entityId);
            editUndo->SetParent(undoBatch.GetUndoBatch());
            FocusOnPrefabInstanceOwningEntityId(entityId);
        }

        return AZ::Success();
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPathIndex(AzFramework::EntityContextId entityContextId, int index)
    {
        const AzFramework::EntityContextId worldId = ResolveWorldId(entityContextId);
        WorldFocus& focus = GetWorldFocus(worldId);

        if (index < 0 || index >= focus.m_rootAliasFocusPathLength)
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: Invalid index on FocusOnPathIndex."));
        }

        int i = 0;
        RootAliasPath indexedPath;
        for (const auto& pathElement : focus.m_rootAliasFocusPath)
        {
            indexedPath.Append(pathElement);

            if (i == index)
            {
                break;
            }

            ++i;
        }

        InstanceOptionalReference focusedInstance = GetInstanceReference(worldId, indexedPath);

        if (!focusedInstance.has_value())
        {
            return AZ::Failure(AZStd::string::format("Prefab Focus Handler: Could not retrieve instance at index %i.", index));
        }

        return FocusOnOwningPrefab(focusedInstance->get().GetContainerEntityId());
    }

    PrefabFocusOperationResult PrefabFocusHandler::SetOwningPrefabInstanceOpenState(AZ::EntityId entityId, bool openState)
    {
        if (InstanceOptionalReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId); instance.has_value())
        {
            m_containerEntityInterface->SetContainerOpen(instance->get().GetContainerEntityId(), openState);

            if (openState == true)
            {
                PrefabFocusNotificationBus::Broadcast(
                    &PrefabFocusNotifications::OnInstanceOpened, instance->get().GetContainerEntityId());
            }

            return AZ::Success();
        }

        return AZ::Failure(AZStd::string::format("Prefab Focus Handler: Could not find owning instance of entity"));
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPrefabInstanceOwningEntityId(AZ::EntityId entityId)
    {
        InstanceOptionalReference focusedInstance;

        if (!entityId.IsValid())
        {
            PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                GetWorldOwnershipService(ResolveWorldId(AzFramework::EntityContextId::CreateNull()));

            if (!prefabEditorEntityOwnershipInterface)
            {
                return AZ::Failure(AZStd::string("Could not focus on root prefab instance - internal error "
                                                 "(PrefabEditorEntityOwnershipInterface unavailable)."));
            }

            focusedInstance = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
        }
        else
        {
            focusedInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        }

        return FocusOnPrefabInstance(focusedInstance);
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnWorldRootInstance(const AzFramework::EntityContextId& worldId)
    {
        PrefabEditorEntityOwnershipInterface* ownershipService = GetWorldOwnershipService(worldId);
        InstanceOptionalReference rootInstance =
            ownershipService ? ownershipService->GetRootPrefabInstance() : InstanceOptionalReference();
        return FocusOnPrefabInstance(rootInstance);
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPrefabInstance(InstanceOptionalReference focusedInstance)
    {
        if (!focusedInstance.has_value())
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: invalid instance to focus on."));
        }

        const AzFramework::EntityContextId worldId = GetEntityWorldId(focusedInstance->get().GetContainerEntityId());
        WorldFocus& focus = GetWorldFocus(worldId);

        // Close all container entities in the old path.
        SetInstanceContainersOpenState(worldId, focus.m_rootAliasFocusPath, false);

        if (IsOutlinerOverrideManagementEnabled())
        {
            // Always close all nested instances in the old focus subtree.
            SetInstanceContainersOpenStateOfAllDescendantContainers(GetInstanceReference(worldId, focus.m_rootAliasFocusPath), false);
        }

        const RootAliasPath previousContainerRootAliasPath = focus.m_rootAliasFocusPath;
        const InstanceOptionalReference previousFocusedInstance = GetInstanceReference(worldId, previousContainerRootAliasPath);
        focus.m_rootAliasFocusPath = focusedInstance->get().GetAbsoluteInstanceAliasPath();
        focus.m_rootAliasFocusPathLength =
            aznumeric_cast<int>(AZStd::distance(focus.m_rootAliasFocusPath.begin(), focus.m_rootAliasFocusPath.end()));

        // Unset the DOM caching for previous focus and enabled it in new focus to optimize editing.
        if (previousFocusedInstance.has_value())
        {
            previousFocusedInstance->get().EnableDomCaching(false);
        }
        focusedInstance->get().EnableDomCaching(true);

        // Focus on the container entity in the Editor, if the interface is initialized.
        if (m_focusModeInterface)
        {
            const AZ::EntityId containerEntityId =
                (focusedInstance->get().GetParentInstance() != AZStd::nullopt)
                ? focusedInstance->get().GetContainerEntityId()
                : AZ::EntityId();

            if (containerEntityId.IsValid())
            {
                m_focusModeInterface->SetFocusRoot(containerEntityId);
            }
            else
            {
                m_focusModeInterface->ClearFocusRoot(worldId);
            }
        }

        // Refresh the read-only cache, if the interface is initialized.
        if (m_readOnlyEntityQueryInterface)
        {
            EntityIdList entities;

            if (previousFocusedInstance.has_value())
            {
                previousFocusedInstance->get().GetEntities(
                    [&](AZStd::unique_ptr<AZ::Entity>& entity) -> bool
                {
                    entities.push_back(entity->GetId());
                    return true;
                }
                );
                entities.push_back(previousFocusedInstance->get().GetContainerEntityId());
            }
            entities.push_back(focusedInstance->get().GetContainerEntityId());
            focusedInstance->get().GetEntities(
                [&](AZStd::unique_ptr<AZ::Entity>& entity) -> bool
            {
                entities.push_back(entity->GetId());
                return true;
            });

            m_readOnlyEntityQueryInterface->RefreshReadOnlyState(entities);
        }

        // Refresh path variables.
        RefreshInstanceFocusPath(worldId, focus);

        // Open all container entities in the new path.
        SetInstanceContainersOpenState(worldId, focus.m_rootAliasFocusPath, true);

        if (IsOutlinerOverrideManagementEnabled())
        {
            // Set open state on all nested instances in the new focus subtree based on edit scope.
            SetInstanceContainersOpenStateOfAllDescendantContainers(GetInstanceReference(worldId, focus.m_rootAliasFocusPath), true);
        }

        AZ::EntityId previousFocusedInstanceContainerEntityId = previousFocusedInstance.has_value() ?
            previousFocusedInstance->get().GetContainerEntityId() : AZ::EntityId();
        AZ::EntityId currentFocusedInstanceContainerEntityId = focusedInstance.has_value() ?
            focusedInstance->get().GetContainerEntityId() : AZ::EntityId();
        if (previousFocusedInstanceContainerEntityId != currentFocusedInstanceContainerEntityId)
        {
            PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusChanged,
                previousFocusedInstanceContainerEntityId, currentFocusedInstanceContainerEntityId);
        }

        // Force propagation on both the previous and the new focused instances to ensure they are represented correctly.
        // The most common operation is focusing a prefab instance nested in the currently focused instance.
        // Queuing the previous focus before the new one saves some time in the propagation loop on average.
        if (previousFocusedInstance.has_value())
        {
            // No need to update the previous focused instance if it's a descendant of the newly focused instance
            if (!PrefabInstanceUtils::IsDescendantInstance(*previousFocusedInstance, *focusedInstance))
            {
                m_instanceUpdateExecutorInterface->AddInstanceToQueue(previousFocusedInstance);
            }
        }
        m_instanceUpdateExecutorInterface->AddInstanceToQueue(focusedInstance);

        return AZ::Success();
    }
    
    TemplateId PrefabFocusHandler::GetFocusedPrefabTemplateId(AzFramework::EntityContextId entityContextId) const
    {
        const AzFramework::EntityContextId worldId = ResolveWorldId(entityContextId);
        InstanceOptionalReference instance = GetInstanceReference(worldId, GetWorldFocus(worldId).m_rootAliasFocusPath);

        if (instance.has_value())
        {
            return instance->get().GetTemplateId();
        }
        else
        {
            return Prefab::InvalidTemplateId;
        }
    }

    InstanceOptionalReference PrefabFocusHandler::GetFocusedPrefabInstance(
        AzFramework::EntityContextId entityContextId) const
    {
        const AzFramework::EntityContextId worldId = ResolveWorldId(entityContextId);
        return GetInstanceReference(worldId, GetWorldFocus(worldId).m_rootAliasFocusPath);
    }

    InstanceOptionalReference PrefabFocusHandler::GetFocusedPrefabInstanceForEntity(AZ::EntityId entityId) const
    {
        const AzFramework::EntityContextId worldId = GetEntityWorldId(entityId);
        return GetInstanceReference(worldId, GetWorldFocus(worldId).m_rootAliasFocusPath);
    }

    bool PrefabFocusHandler::IsFocusedPrefabInstanceReadOnly(AzFramework::EntityContextId entityContextId) const
    {
        const AzFramework::EntityContextId worldId = ResolveWorldId(entityContextId);
        InstanceOptionalReference instance = GetInstanceReference(worldId, GetWorldFocus(worldId).m_rootAliasFocusPath);

        if (instance.has_value())
        {
            return m_readOnlyEntityPublicInterface->IsReadOnly(instance->get().GetContainerEntityId());
        }

        return false;
    }

    InstanceClimbUpResult PrefabFocusHandler::ClimbUpToFocusedOrRootInstanceFromEntity(AZ::EntityId entityId) const
    {
        // Grab the owning instance.
        InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        AZ_Assert(owningInstance.has_value(), "PrefabFocusHandler::ClimbUpToFocusedOrRootInstanceFromEntity - "
            "The owning instance of the given entity id is null.");

        // Retrieve the path from the entity's world's focused instance to the owningInstance of the given entity id.
        const AzFramework::EntityContextId worldId = GetEntityWorldId(entityId);
        InstanceOptionalReference focusedInstance = GetInstanceReference(worldId, GetWorldFocus(worldId).m_rootAliasFocusPath);
        AZ_Assert(focusedInstance.has_value(), "PrefabFocusHandler::ClimbUpToFocusedOrRootInstanceFromEntity - "
            "The focused instance is null.");
        const Instance* focusedInstancePtr = &(focusedInstance->get());

        // Climb up the instance hierarchy from the owning instance until it hits the focused prefab instance.
        InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(owningInstance->get(), focusedInstancePtr);
        return climbUpResult;
    }

    LinkId PrefabFocusHandler::PrependPathFromFocusedInstanceToPatchPaths(PrefabDom& patches, AZ::EntityId entityId) const
    {
        AZ_Assert(false, "PrefabFocusHandler::PrependPathFromFocusedInstanceToPatchPaths - "
            "The provided patches should an array of patches to update.");

        // Climb up the instance hierarchy from the owning instance until it hits the focused prefab instance.
        InstanceClimbUpResult climbUpResult = ClimbUpToFocusedOrRootInstanceFromEntity(entityId);
        if (!climbUpResult.m_isTargetInstanceReached)
        {
            AZ_Error("Prefab", false, "PrefabFocusHandler::PrependPathFromFocusedInstanceToPatchPaths - "
                "Entity id is not owned by a descendant of the focused prefab instance.");
            return InvalidLinkId;
        }

        // If there are climbed instances, then return the link id stored in the climbed instance
        // closest to the focused instance.
        if (!climbUpResult.m_climbedInstances.empty())
        {
            // Skip the instance closest to the target instance.
            AZStd::string prefix = PrefabInstanceUtils::GetRelativePathFromClimbedInstances(
                climbUpResult.m_climbedInstances, true);

            m_instanceToTemplateInterface->PrependEntityAliasPathToPatchPaths(patches, entityId, AZStd::move(prefix));
            return climbUpResult.m_climbedInstances.back()->GetLinkId();
        }
        else
        {
            m_instanceToTemplateInterface->PrependEntityAliasPathToPatchPaths(patches, entityId);
            return InvalidLinkId;
        }
    }

    AZ::EntityId PrefabFocusHandler::GetFocusedPrefabContainerEntityId(
        AzFramework::EntityContextId entityContextId) const
    {
        const AzFramework::EntityContextId worldId = ResolveWorldId(entityContextId);
        if (const InstanceOptionalReference instance = GetInstanceReference(worldId, GetWorldFocus(worldId).m_rootAliasFocusPath);
            instance.has_value())
        {
            return instance->get().GetContainerEntityId();
        }

        return AZ::EntityId();
    }

    bool PrefabFocusHandler::IsOwningPrefabBeingFocused(AZ::EntityId entityId) const
    {
        if (!entityId.IsValid())
        {
            return false;
        }

        InstanceOptionalReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        if (!instance.has_value())
        {
            return false;
        }

        return (instance->get().GetAbsoluteInstanceAliasPath() == GetWorldFocus(GetEntityWorldId(entityId)).m_rootAliasFocusPath);
    }

    bool PrefabFocusHandler::IsOwningPrefabInFocusHierarchy(AZ::EntityId entityId) const
    {
        if (!entityId.IsValid())
        {
            return false;
        }

        const RootAliasPath& rootAliasFocusPath = GetWorldFocus(GetEntityWorldId(entityId)).m_rootAliasFocusPath;
        InstanceOptionalReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        while (instance.has_value())
        {
            if (instance->get().GetAbsoluteInstanceAliasPath() == rootAliasFocusPath)
            {
                return true;
            }

            instance = instance->get().GetParentInstance();
        }

        return false;
    }

    const AZ::IO::Path& PrefabFocusHandler::GetPrefabFocusPath(AzFramework::EntityContextId entityContextId) const
    {
        return GetWorldFocus(ResolveWorldId(entityContextId)).m_filenameFocusPath;
    }

    const int PrefabFocusHandler::GetPrefabFocusPathLength(AzFramework::EntityContextId entityContextId) const
    {
        return GetWorldFocus(ResolveWorldId(entityContextId)).m_rootAliasFocusPathLength;
    }

    void PrefabFocusHandler::SetPrefabEditScope([[maybe_unused]] AzFramework::EntityContextId entityContextId, PrefabEditScope prefabEditScope)
    {
        m_prefabEditScope = prefabEditScope;
        SwitchToEditScope();
    }

    void PrefabFocusHandler::OnPrepareForContextReset()
    {
        // World 0 reloads its level: park its focus back on its root prefab.
        FocusOnWorldRootInstance(GetEntityContextId());
    }

    void PrefabFocusHandler::OnActiveWorldChanged(
        const AzFramework::EntityContextId& previousWorldId, const AzFramework::EntityContextId& newWorldId)
    {
        // The notification restores the new world's remembered focus in the UI.
        auto focusedContainerOf = [this](const AzFramework::EntityContextId& worldId)
        {
            InstanceOptionalReference instance = GetInstanceReference(worldId, GetWorldFocus(worldId).m_rootAliasFocusPath);
            return instance.has_value() ? instance->get().GetContainerEntityId() : AZ::EntityId();
        };

        const AZ::EntityId previousContainerEntityId = focusedContainerOf(previousWorldId);
        const AZ::EntityId newContainerEntityId = focusedContainerOf(newWorldId);
        if (previousContainerEntityId != newContainerEntityId)
        {
            PrefabFocusNotificationBus::Broadcast(
                &PrefabFocusNotifications::OnPrefabFocusChanged, previousContainerEntityId, newContainerEntityId);
        }
    }

    void PrefabFocusHandler::OnWorldLoaded(const AzFramework::EntityContextId& worldId)
    {
        // Focusing the root opens the world's level container (containers default closed).
        FocusOnWorldRootInstance(worldId);
    }

    void PrefabFocusHandler::OnWorldDestroyed(const AzFramework::EntityContextId& worldId)
    {
        m_worldFocus.erase(worldId);
    }

    void PrefabFocusHandler::OnEntityInfoUpdatedName(AZ::EntityId entityId, [[maybe_unused]]const AZStd::string& name)
    {
        for (auto& [worldId, focus] : m_worldFocus)
        {
            PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface = GetWorldOwnershipService(worldId);
            if (!prefabEditorEntityOwnershipInterface)
            {
                continue;
            }

            // Determine if the entityId is the container for any of the instances in this world's focus path.
            bool match = prefabEditorEntityOwnershipInterface->GetInstancesInRootAliasPath(
                focus.m_rootAliasFocusPath,
                [&](const Prefab::InstanceOptionalReference instance)
                {
                    if (instance->get().GetContainerEntityId() == entityId)
                    {
                        return true;
                    }

                    return false;
                }
            );

            if (match)
            {
                // Refresh the path and notify changes.
                RefreshInstanceFocusPath(worldId, focus);
                PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusRefreshed);
                break;
            }
        }
    }

    void PrefabFocusHandler::OnPrefabInstancePropagationEnd()
    {
        // Refresh the paths and notify changes in case propagation updated any container names.
        for (auto& [worldId, focus] : m_worldFocus)
        {
            RefreshInstanceFocusPath(worldId, focus);
        }
        PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusRefreshed);

        if (IsOutlinerOverrideManagementEnabled())
        {
            SwitchToEditScope();
        }
    }

    void PrefabFocusHandler::OnPrefabTemplateDirtyFlagUpdated(TemplateId templateId, [[maybe_unused]] bool status)
    {
        bool anyMatch = false;
        for (auto& [worldId, focus] : m_worldFocus)
        {
            PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface = GetWorldOwnershipService(worldId);
            if (!prefabEditorEntityOwnershipInterface)
            {
                continue;
            }

            // Determine if the templateId matches any of the instances in this world's focus path.
            bool match = prefabEditorEntityOwnershipInterface->GetInstancesInRootAliasPath(
                focus.m_rootAliasFocusPath,
                [&](const Prefab::InstanceOptionalReference instance)
                {
                    if (instance->get().GetTemplateId() == templateId)
                    {
                        return true;
                    }

                    return false;
                }
            );

            if (match)
            {
                RefreshInstanceFocusPath(worldId, focus);
                anyMatch = true;
            }
        }

        if (anyMatch)
        {
            PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusRefreshed);
        }
    }

    void PrefabFocusHandler::RefreshInstanceFocusPath(const AzFramework::EntityContextId& worldId, WorldFocus& focus) const
    {
        focus.m_filenameFocusPath.clear();

        PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface = GetWorldOwnershipService(worldId);
        PrefabSystemComponentInterface* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

        if (prefabEditorEntityOwnershipInterface && prefabSystemComponentInterface)
        {
            int i = 0;

            prefabEditorEntityOwnershipInterface->GetInstancesInRootAliasPath(
                focus.m_rootAliasFocusPath,
                [&](const Prefab::InstanceOptionalReference instance)
                {
                    if (instance.has_value())
                    {
                        AZStd::string prefabName;

                        if (i == focus.m_rootAliasFocusPathLength - 1)
                        {
                            // Get the full filename.
                            prefabName = instance->get().GetTemplateSourcePath().Filename().Native();
                        }
                        else
                        {
                            // Get the filename without the extension (stem).
                            prefabName = instance->get().GetTemplateSourcePath().Stem().Native();
                        }

                        if (prefabSystemComponentInterface->IsTemplateDirty(instance->get().GetTemplateId()))
                        {
                            prefabName += "*";
                        }

                        focus.m_filenameFocusPath.Append(prefabName);
                    }

                    ++i;
                    return false;
                }
            );
        }
    }

    void PrefabFocusHandler::SetInstanceContainersOpenState(
        const AzFramework::EntityContextId& worldId, const RootAliasPath& rootAliasPath, bool openState) const
    {
        // If this is called outside the Editor, this interface won't be initialized.
        if (!m_containerEntityInterface)
        {
            return;
        }

        PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface = GetWorldOwnershipService(worldId);

        if (prefabEditorEntityOwnershipInterface)
        {
            prefabEditorEntityOwnershipInterface->GetInstancesInRootAliasPath(
                rootAliasPath,
                [&](const Prefab::InstanceOptionalReference instance)
                {
                    // A root instance is a level, and a level container is never closed; closing one would strand
                    // every entity in it under a closed container when focus moves to another root.
                    if (openState || instance->get().GetParentInstance().has_value())
                    {
                        m_containerEntityInterface->SetContainerOpen(instance->get().GetContainerEntityId(), openState);
                    }

                    return false;
                }
            );
        }
    }

    void PrefabFocusHandler::SetInstanceContainersOpenStateOfAllDescendantContainers(
        InstanceOptionalReference instance, bool openState) const
    {
        // If this is called outside the Editor, this interface won't be initialized.
        if (!m_containerEntityInterface)
        {
            return;
        }

        if (!instance.has_value())
        {
            return;
        }

        AZStd::queue<InstanceOptionalReference> instanceQueue;

        // We skip this instance and start from children.
        instance->get().GetNestedInstances(
            [&](AZStd::unique_ptr<Instance>& nestedInstance)
        {
            instanceQueue.push(*nestedInstance.get());
        }
        );

        while (!instanceQueue.empty())
        {
            InstanceOptionalReference currentInstance = instanceQueue.front();
            instanceQueue.pop();

            if (currentInstance.has_value())
            {
                m_containerEntityInterface->SetContainerOpen(currentInstance->get().GetContainerEntityId(), openState);

                currentInstance->get().GetNestedInstances(
                    [&](AZStd::unique_ptr<Instance>& nestedInstance)
                {
                    instanceQueue.push(*nestedInstance.get());
                }
                );
            }
        }
    }

    void PrefabFocusHandler::SwitchToEditScope() const
    {
        const AzFramework::EntityContextId worldId = ResolveWorldId(AzFramework::EntityContextId::CreateNull());
        auto focusInstance = GetInstanceReference(worldId, GetWorldFocus(worldId).m_rootAliasFocusPath);

        switch (m_prefabEditScope)
        {           
        case PrefabEditScope::SHOW_NESTED_INSTANCES_CONTENT:
        {
            SetInstanceContainersOpenStateOfAllDescendantContainers(focusInstance, true);
        }
        break;
        default:
        {
            SetInstanceContainersOpenStateOfAllDescendantContainers(focusInstance, false);
        }
        break;
        }

        PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabEditScopeChanged);
    }

    InstanceOptionalReference PrefabFocusHandler::GetInstanceReference(
        const AzFramework::EntityContextId& worldId, RootAliasPath rootAliasPath) const
    {
        PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface = GetWorldOwnershipService(worldId);

        if (prefabEditorEntityOwnershipInterface)
        {
            return prefabEditorEntityOwnershipInterface->GetInstanceReferenceFromRootAliasPath(rootAliasPath);
        }

        return AZStd::nullopt;
    }
} // namespace AzToolsFramework::Prefab
