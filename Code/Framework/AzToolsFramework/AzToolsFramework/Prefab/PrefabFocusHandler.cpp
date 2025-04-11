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
        [[maybe_unused]] AzFramework::EntityContextId entityContextId)
    {
        // If only one instance is in the hierarchy, this operation is invalid
        if (m_rootAliasFocusPathLength <= 1)
        {
            return AZ::Failure(AZStd::string(
                "Prefab Focus Handler: Could not complete FocusOnParentOfFocusedPrefab operation while focusing on the root."));
        }

        RootAliasPath parentPath = m_rootAliasFocusPath;
        parentPath.RemoveFilename();

        // Retrieve parent of currently focused prefab.
        InstanceOptionalReference parentInstance = GetInstanceReference(parentPath);

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

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPathIndex([[maybe_unused]] AzFramework::EntityContextId entityContextId, int index)
    {
        if (index < 0 || index >= m_rootAliasFocusPathLength)
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: Invalid index on FocusOnPathIndex."));
        }

        int i = 0;
        RootAliasPath indexedPath;
        for (const auto& pathElement : m_rootAliasFocusPath)
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
        InstanceOptionalReference focusedInstance;

        if (!entityId.IsValid())
        {
            PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();

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

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPrefabInstance(InstanceOptionalReference focusedInstance)
    {
        if (!focusedInstance.has_value())
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: invalid instance to focus on."));
        }

        // Close all container entities in the old path.
        SetInstanceContainersOpenState(m_rootAliasFocusPath, false);

        if (IsOutlinerOverrideManagementEnabled())
        {
            // Always close all nested instances in the old focus subtree.
            SetInstanceContainersOpenStateOfAllDescendantContainers(GetInstanceReference(m_rootAliasFocusPath), false);
        }

        const RootAliasPath previousContainerRootAliasPath = m_rootAliasFocusPath;
        const InstanceOptionalReference previousFocusedInstance = GetInstanceReference(previousContainerRootAliasPath);
        m_rootAliasFocusPath = focusedInstance->get().GetAbsoluteInstanceAliasPath();
        m_rootAliasFocusPathLength = aznumeric_cast<int>(AZStd::distance(m_rootAliasFocusPath.begin(), m_rootAliasFocusPath.end()));

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
        RefreshInstanceFocusPath();

        // Open all container entities in the new path.
        SetInstanceContainersOpenState(m_rootAliasFocusPath, true);

        if (IsOutlinerOverrideManagementEnabled())
        {
            // Set open state on all nested instances in the new focus subtree based on edit scope.
            SetInstanceContainersOpenStateOfAllDescendantContainers(GetInstanceReference(m_rootAliasFocusPath), true);
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
        InstanceOptionalReference instance = GetInstanceReference(m_rootAliasFocusPath);

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
        return GetInstanceReference(m_rootAliasFocusPath);
    }

    bool PrefabFocusHandler::IsFocusedPrefabInstanceReadOnly([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        InstanceOptionalReference instance = GetInstanceReference(m_rootAliasFocusPath);

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
        InstanceOptionalReference focusedInstance = GetInstanceReference(m_rootAliasFocusPath);
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
        if (const InstanceOptionalReference instance = GetInstanceReference(m_rootAliasFocusPath); instance.has_value())
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

        return (instance->get().GetAbsoluteInstanceAliasPath() == m_rootAliasFocusPath);
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
            if (instance->get().GetAbsoluteInstanceAliasPath() == m_rootAliasFocusPath)
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
        // Focus on the root prefab (AZ::EntityId() will default to it)
        FocusOnPrefabInstanceOwningEntityId(AZ::EntityId());
    }

    void PrefabFocusHandler::OnEntityInfoUpdatedName(AZ::EntityId entityId, [[maybe_unused]]const AZStd::string& name)
    {
        PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
            AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();

        if (prefabEditorEntityOwnershipInterface)
        {
            // Determine if the entityId is the container for any of the instances in the vector.
            bool match = prefabEditorEntityOwnershipInterface->GetInstancesInRootAliasPath(
                m_rootAliasFocusPath,
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
                RefreshInstanceFocusPath();
                PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusRefreshed);
            }
        }
    }

    void PrefabFocusHandler::OnPrefabInstancePropagationEnd()
    {
        // Refresh the path and notify changes in case propagation updated any container names.
        RefreshInstanceFocusPath();
        PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusRefreshed);

        if (IsOutlinerOverrideManagementEnabled())
        {
            SwitchToEditScope();
        }
    }

    void PrefabFocusHandler::OnPrefabTemplateDirtyFlagUpdated(TemplateId templateId, [[maybe_unused]] bool status)
    {
        PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
            AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();

        if (prefabEditorEntityOwnershipInterface)
        {
            // Determine if the templateId matches any of the instances in the vector.
            bool match = prefabEditorEntityOwnershipInterface->GetInstancesInRootAliasPath(
                m_rootAliasFocusPath,
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
                // Refresh the path and notify changes.
                RefreshInstanceFocusPath();
                PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusRefreshed);
            }
        }
    }

    void PrefabFocusHandler::RefreshInstanceFocusPath()
    {
        m_filenameFocusPath.clear();
        
        PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
            AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
        PrefabSystemComponentInterface* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

        if (prefabEditorEntityOwnershipInterface && prefabSystemComponentInterface)
        {
            int i = 0;

            prefabEditorEntityOwnershipInterface->GetInstancesInRootAliasPath(
                m_rootAliasFocusPath,
                [&](const Prefab::InstanceOptionalReference instance)
                {
                    if (instance.has_value())
                    {
                        AZStd::string prefabName;

                        if (i == m_rootAliasFocusPathLength - 1)
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

                        m_filenameFocusPath.Append(prefabName);
                    }

                    ++i;
                    return false;
                }
            );
        }
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
        auto focusInstance = GetInstanceReference(m_rootAliasFocusPath);

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
