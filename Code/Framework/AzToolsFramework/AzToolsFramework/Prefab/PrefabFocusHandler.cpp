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
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
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

    SelectionCommand* PrefabFocusHandler::CreateSelectionCommandForFocusedPrefab(AZ::EntityId referenceId)
    {
        using AzFramework::EntityContextId;
        using AzFramework::EntityIdContextQueryBus;

        // update selection - if there is a focused instance, select its container.  Default to the editor context.
        EntityContextId contextId = AZ::Uuid::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(contextId, &EditorEntityContextRequests::GetEditorEntityContextId);

        if (referenceId.IsValid())
        {
            EntityIdContextQueryBus::EventResult(contextId, referenceId, &EntityIdContextQueryBus::Events::GetOwningContextId);
        }

        EntityIdList selectedEntities;
        if (AZ::EntityId focusedId = GetFocusedPrefabContainerEntityId(contextId); focusedId.IsValid())
        {
            selectedEntities.push_back(focusedId);
        }
        return new SelectionCommand(selectedEntities, "Select entities");
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnOwningPrefab(AZ::EntityId entityId)
    {
        
        const auto target = FindFocusInstance(entityId);
        if (!target)
        {
            return AZ::Failure(AZStd::string("Could not find the owning prefab instance."));
        }
        ScopedUndoBatch undoBatch("Focus on Prefab");
        auto apply = [this, owner = AZStd::weak_ptr<RootAliasPath>(m_activePath)](const RootAliasPath& targetPath)
        {
            if (auto path = owner.lock())
            {
                if (path == m_activePath)
                {
                    FocusOnPrefabInstance(GetInstanceReference(targetPath), *path);
                }
                else
                {
                    *path = targetPath;
                    PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusRefreshed);
                }
            }
        };
        auto* editUndo = aznew PrefabFocusUndo(*m_activePath, target->get().GetAbsoluteInstanceAliasPath(), AZStd::move(apply));
        editUndo->SetParent(undoBatch.GetUndoBatch());
        editUndo->Redo();

        if (auto selectionUndo = CreateSelectionCommandForFocusedPrefab(entityId); selectionUndo)
        {
            selectionUndo->SetParent(undoBatch.GetUndoBatch());
            selectionUndo->Redo();
        }

        return AZ::Success();
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnParentOfFocusedPrefab(
        [[maybe_unused]] AzFramework::EntityContextId entityContextId)
    {
        // If only one instance is in the hierarchy, this operation is invalid
        if (m_rootAliasFocusPathLength <= 1)
        {
            return AZ::Failure(AZStd::string(
                "Prefab Focus Handler: Could not complete FocusOnParentOfFocusedPrefab operation while focusing on the root."));
        }

        RootAliasPath parentPath = *m_activePath;
        parentPath.RemoveFilename();

        // Retrieve parent of currently focused prefab.
        InstanceOptionalReference parentInstance = GetInstanceReference(parentPath);

        // If only one instance is in the hierarchy, this operation is invalid
        if (!parentInstance.has_value())
        {
            return AZ::Failure(AZStd::string(
                "Prefab Focus Handler: Could not retrieve parent of current focus in FocusOnParentOfFocusedPrefab."));
        }

        return FocusOnOwningPrefab(parentInstance->get().GetContainerEntityId());
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPathIndex([[maybe_unused]] AzFramework::EntityContextId entityContextId, int index)
    {
        if (index < 0 || index >= m_rootAliasFocusPathLength)
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: Invalid index on FocusOnPathIndex."));
        }

        int i = 0;
        RootAliasPath indexedPath;
        for (const auto& pathElement : *m_activePath)
        {
            indexedPath.Append(pathElement);

            if (i == index)
            {
                break;
            }

            ++i;
        }

        InstanceOptionalReference focusedInstance = GetInstanceReference(indexedPath);

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
        return FocusOnPrefabInstance(FindFocusInstance(entityId), *m_activePath);
    }

    InstanceOptionalReference PrefabFocusHandler::FindFocusInstance(AZ::EntityId entityId) const
    {
        if (entityId.IsValid())
        {
            return m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        }
        auto* ownership = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
        return ownership ? ownership->GetRootPrefabInstance() : InstanceOptionalReference{};
    }

    InstanceOptionalReference PrefabFocusHandler::GetViewportInstance(AzFramework::ViewportId viewportId) const
    {
        if (const auto it = m_viewportPaths.find(viewportId); it != m_viewportPaths.end() && !it->second->empty())
        {
            if (auto instance = GetInstanceReference(*it->second))
            {
                return instance;
            }
        }
        return FindFocusInstance(AZ::EntityId());
    }

    void PrefabFocusHandler::ActivateViewport(AzFramework::ViewportId viewportId)
    {
        auto& path = m_viewportPaths[viewportId];
        if (!path)
        {
            path = AZStd::make_shared<RootAliasPath>();
        }
        auto previousPath = m_activePath;
        m_activePath = path;
        if (!previousPath->empty())
        {
            auto instance = GetViewportInstance(viewportId);
            if (instance && instance->get().GetAbsoluteInstanceAliasPath() != *previousPath)
            {
                FocusOnPrefabInstance(instance, *previousPath);
            }
            else
            {
                *path = *previousPath;
            }
        }
    }

    void PrefabFocusHandler::RemoveViewport(AzFramework::ViewportId viewportId)
    {
        m_viewportPaths.erase(viewportId);
    }

    bool PrefabFocusHandler::IsEntityVisibleInViewportFocus(
        AzFramework::ViewportId viewportId, AZ::EntityId entityId, bool visibleInActiveFocus) const
    {
        const auto it = m_viewportPaths.find(viewportId);
        if (m_activePath->empty() || (it != m_viewportPaths.end() && *it->second == *m_activePath))
        {
            return visibleInActiveFocus;
        }
        const auto focus = GetViewportInstance(viewportId);
        auto owner = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        if (!focus || !owner)
        {
            return visibleInActiveFocus;
        }
        if (&owner->get() == &focus->get())
        {
            return true;
        }
        // A closed prefab still displays its own container icon.
        if (entityId == owner->get().GetContainerEntityId())
        {
            owner = owner->get().GetParentInstance();
        }
        return owner && (&owner->get() == &focus->get() ||
            (m_prefabEditScope == PrefabEditScope::SHOW_NESTED_INSTANCES_CONTENT &&
             PrefabInstanceUtils::IsDescendantInstance(owner->get(), focus->get())));
    }

    AZ::IO::Path PrefabFocusHandler::GetViewportFocusPath(AzFramework::ViewportId viewportId) const
    {
        if (m_activePath->empty())
        {
            return {};
        }
        auto instance = GetViewportInstance(viewportId);
        return instance ? BuildFocusPath(instance->get().GetAbsoluteInstanceAliasPath()) : AZ::IO::Path{};
    }

    AZ::EntityId PrefabFocusHandler::GetViewportFocusRoot(AzFramework::ViewportId viewportId) const
    {
        const auto it = m_viewportPaths.find(viewportId);
        if (it != m_viewportPaths.end() && it->second->HasParentPath())
        {
            if (auto instance = GetInstanceReference(*it->second))
            {
                return instance->get().GetContainerEntityId();
            }
        }
        return AZ::EntityId{};
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPrefabInstance(InstanceOptionalReference focusedInstance, const RootAliasPath& previousPath)
    {
        if (!focusedInstance.has_value())
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: invalid instance to focus on."));
        }

        // Close all container entities in the old path.
        SetInstanceContainersOpenState(previousPath, false);

        if (IsOutlinerOverrideManagementEnabled())
        {
            // Always close all nested instances in the old focus subtree.
            SetInstanceContainersOpenStateOfAllDescendantContainers(GetInstanceReference(previousPath), false);
        }

        const InstanceOptionalReference previousFocusedInstance = GetInstanceReference(previousPath);
        *m_activePath = focusedInstance->get().GetAbsoluteInstanceAliasPath();
        m_rootAliasFocusPathLength = aznumeric_cast<int>(AZStd::distance(m_activePath->begin(), m_activePath->end()));

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

            m_focusModeInterface->SetFocusRoot(containerEntityId);
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
        m_filenameFocusPath = BuildFocusPath(*m_activePath);

        // Open all container entities in the new path.
        SetInstanceContainersOpenState(*m_activePath, true);

        if (IsOutlinerOverrideManagementEnabled())
        {
            // Set open state on all nested instances in the new focus subtree based on edit scope.
            SetInstanceContainersOpenStateOfAllDescendantContainers(GetInstanceReference(*m_activePath), true);
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
    
    TemplateId PrefabFocusHandler::GetFocusedPrefabTemplateId([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        InstanceOptionalReference instance = GetInstanceReference(*m_activePath);

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
        [[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return GetInstanceReference(*m_activePath);
    }

    bool PrefabFocusHandler::IsFocusedPrefabInstanceReadOnly([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        InstanceOptionalReference instance = GetInstanceReference(*m_activePath);

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

        // Retrieve the path from the focused prefab instance to the owningInstance of the given entity id.
        InstanceOptionalReference focusedInstance = GetInstanceReference(*m_activePath);
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
        [[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        if (const InstanceOptionalReference instance = GetInstanceReference(*m_activePath); instance.has_value())
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

        return (instance->get().GetAbsoluteInstanceAliasPath() == *m_activePath);
    }

    bool PrefabFocusHandler::IsOwningPrefabInFocusHierarchy(AZ::EntityId entityId) const
    {
        if (!entityId.IsValid())
        {
            return false;
        }

        InstanceOptionalReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        while (instance.has_value())
        {
            if (instance->get().GetAbsoluteInstanceAliasPath() == *m_activePath)
            {
                return true;
            }

            instance = instance->get().GetParentInstance();
        }

        return false;
    }

    const AZ::IO::Path& PrefabFocusHandler::GetPrefabFocusPath([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return m_filenameFocusPath;
    }

    const int PrefabFocusHandler::GetPrefabFocusPathLength([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return m_rootAliasFocusPathLength;
    }

    void PrefabFocusHandler::SetPrefabEditScope([[maybe_unused]] AzFramework::EntityContextId entityContextId, PrefabEditScope prefabEditScope)
    {
        m_prefabEditScope = prefabEditScope;
        SwitchToEditScope();
    }

    void PrefabFocusHandler::OnPrepareForContextReset()
    {
        for (auto& [viewportId, path] : m_viewportPaths)
        {
            if (path != m_activePath)
            {
                path->clear();
            }
        }
        // Focus on the root prefab (AZ::EntityId() will default to it)
        FocusOnPrefabInstanceOwningEntityId(AZ::EntityId());
    }

    void PrefabFocusHandler::OnEntityInfoUpdatedName(AZ::EntityId entityId, [[maybe_unused]] const AZStd::string& name)
    {
        if (auto instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            instance && instance->get().GetContainerEntityId() == entityId)
        {
            m_filenameFocusPath = BuildFocusPath(*m_activePath);
            PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusRefreshed);
        }
    }

    void PrefabFocusHandler::OnPrefabInstancePropagationEnd()
    {
        for (auto& [viewportId, path] : m_viewportPaths)
        {
            if (!path->empty() && !GetInstanceReference(*path))
            {
                if (path == m_activePath)
                {
                    FocusOnPrefabInstanceOwningEntityId(AZ::EntityId());
                }
                else
                {
                    path->clear();
                }
            }
        }
        // Refresh the path and notify changes in case propagation updated any container names.
        m_filenameFocusPath = BuildFocusPath(*m_activePath);
        PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusRefreshed);

        if (IsOutlinerOverrideManagementEnabled())
        {
            SwitchToEditScope();
        }
    }

    void PrefabFocusHandler::OnPrefabTemplateDirtyFlagUpdated([[maybe_unused]] TemplateId templateId, [[maybe_unused]] bool status)
    {
        m_filenameFocusPath = BuildFocusPath(*m_activePath);
        PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusRefreshed);
    }

    AZ::IO::Path PrefabFocusHandler::BuildFocusPath(const RootAliasPath& path) const
    {
        AZ::IO::Path filenamePath(AZ::IO::PosixPathSeparator);
        if (path.empty())
        {
            return filenamePath;
        }

        PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
            AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
        PrefabSystemComponentInterface* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

        if (prefabEditorEntityOwnershipInterface && prefabSystemComponentInterface)
        {
            auto element = path.begin();
            prefabEditorEntityOwnershipInterface->GetInstancesInRootAliasPath(
                path,
                [&](const Prefab::InstanceOptionalReference instance)
                {
                    if (instance.has_value())
                    {
                        AZStd::string prefabName;

                        if (++element == path.end())
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

                        filenamePath.Append(prefabName);
                    }

                    return false;
                }
            );
        }
        return filenamePath;
    }

    void PrefabFocusHandler::SetInstanceContainersOpenState(const RootAliasPath& rootAliasPath, bool openState) const
    {
        // If this is called outside the Editor, this interface won't be initialized.
        if (!m_containerEntityInterface)
        {
            return;
        }

        PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
            AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();

        if (prefabEditorEntityOwnershipInterface)
        {
            prefabEditorEntityOwnershipInterface->GetInstancesInRootAliasPath(
                rootAliasPath,
                [&](const Prefab::InstanceOptionalReference instance)
                {
                    m_containerEntityInterface->SetContainerOpen(instance->get().GetContainerEntityId(), openState);

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
        auto focusInstance = GetInstanceReference(*m_activePath);

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

    InstanceOptionalReference PrefabFocusHandler::GetInstanceReference(RootAliasPath rootAliasPath) const
    {
        PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
            AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();

        if (prefabEditorEntityOwnershipInterface)
        {
            return prefabEditorEntityOwnershipInterface->GetInstanceReferenceFromRootAliasPath(rootAliasPath);
        }

        return AZStd::nullopt;
    }
} // namespace AzToolsFramework::Prefab
