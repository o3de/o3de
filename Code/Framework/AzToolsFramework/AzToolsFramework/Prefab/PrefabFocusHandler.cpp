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
#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>
#include <AzToolsFramework/Prefab/PrefabFocusUndo.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework::Prefab
{
    PrefabFocusHandler::PrefabFocusHandler()
    {
        m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
        AZ_Assert(
            m_instanceEntityMapperInterface,
            "Prefab - PrefabFocusHandler - "
            "Instance Entity Mapper Interface could not be found. "
            "Check that it is being correctly initialized.");

        EditorEntityInfoNotificationBus::Handler::BusConnect();
        EditorEntityContextNotificationBus::Handler::BusConnect();
        PrefabPublicNotificationBus::Handler::BusConnect();
        AZ::Interface<PrefabFocusInterface>::Register(this);
        AZ::Interface<PrefabFocusPublicInterface>::Register(this);
        PrefabFocusPublicRequestBus::Handler::BusConnect();
    }

    PrefabFocusHandler::~PrefabFocusHandler()
    {
        PrefabFocusPublicRequestBus::Handler::BusDisconnect();
        AZ::Interface<PrefabFocusPublicInterface>::Unregister(this);
        AZ::Interface<PrefabFocusInterface>::Unregister(this);
        PrefabPublicNotificationBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();
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

        m_readOnlyEntityQueryInterface = AZ::Interface<ReadOnlyEntityQueryInterface>::Get();
        AZ_Assert(
            m_readOnlyEntityQueryInterface,
            "Prefab - PrefabFocusHandler - "
            "ReadOnly Entity Query Interface could not be found. "
            "Check that it is being correctly initialized.");
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

        const RootAliasPath previousContainerRootAliasPath = m_rootAliasFocusPath;
        const InstanceOptionalReference previousFocusedInstance = GetInstanceReference(previousContainerRootAliasPath);

        m_rootAliasFocusPath = focusedInstance->get().GetAbsoluteInstanceAliasPath();
        m_focusedTemplateId = focusedInstance->get().GetTemplateId();
        m_rootAliasFocusPathLength = aznumeric_cast<int>(AZStd::distance(m_rootAliasFocusPath.begin(), m_rootAliasFocusPath.end()));

        // Focus on the descendants of the container entity in the Editor, if the interface is initialized.
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
            EntityIdList containerEntities;

            if (previousFocusedInstance.has_value())
            {
                containerEntities.push_back(previousFocusedInstance->get().GetContainerEntityId());
            }
            containerEntities.push_back(focusedInstance->get().GetContainerEntityId());

            m_readOnlyEntityQueryInterface->RefreshReadOnlyState(containerEntities);
        }

        // Refresh path variables.
        RefreshInstanceFocusPath();

        // Open all container entities in the new path.
        SetInstanceContainersOpenState(m_rootAliasFocusPath, true);

        PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusChanged);

        return AZ::Success();
    }
    
    TemplateId PrefabFocusHandler::GetFocusedPrefabTemplateId([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return m_focusedTemplateId;
    }

    InstanceOptionalReference PrefabFocusHandler::GetFocusedPrefabInstance(
        [[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return GetInstanceReference(m_rootAliasFocusPath);
    }

    AZ::EntityId PrefabFocusHandler::GetFocusedPrefabContainerEntityId([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
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

        const InstanceOptionalConstReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
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

        InstanceOptionalConstReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
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

    void PrefabFocusHandler::OnContextReset()
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
                PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusChanged);
            }
        }
    }

    void PrefabFocusHandler::OnPrefabInstancePropagationEnd()
    {
        // Refresh the path and notify changes in case propagation updated any container names.
        RefreshInstanceFocusPath();
        PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusChanged);
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
                PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusChanged);
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
