/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabIntegrationManager.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <AzToolsFramework/API/EntityPropertyEditorRequestsBus.h>
#include <AzToolsFramework/API/SettingsRegistryUtils.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorActionUpdaterIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorToolBarIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/Prefab/EditorPrefabComponent.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/Procedural/ProceduralPrefabAsset.h>
#include <AzToolsFramework/Prefab/PrefabSettings.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>
#include <AzToolsFramework/UI/Prefab/ActionManagerIdentifiers/PrefabActionUpdaterIdentifiers.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabViewportFocusPathHandler.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Viewport/ActionBus.h>

#include <QApplication>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

namespace AzToolsFramework
{
    namespace Prefab
    {
        AzFramework::EntityContextId PrefabIntegrationManager::s_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

        ContainerEntityInterface* PrefabIntegrationManager::s_containerEntityInterface = nullptr;
        EditorEntityUiInterface* PrefabIntegrationManager::s_editorEntityUiInterface = nullptr;
        PrefabFocusInterface* PrefabIntegrationManager::s_prefabFocusInterface = nullptr;
        PrefabFocusPublicInterface* PrefabIntegrationManager::s_prefabFocusPublicInterface = nullptr;
        PrefabPublicInterface* PrefabIntegrationManager::s_prefabPublicInterface = nullptr;

        PrefabIntegrationManager::PrefabIntegrationManager()
        {
            s_containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();
            if (s_containerEntityInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get ContainerEntityInterface on PrefabIntegrationManager construction.");
                return;
            }

            s_editorEntityUiInterface = AZ::Interface<EditorEntityUiInterface>::Get();
            if (s_editorEntityUiInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get EditorEntityUiInterface on PrefabIntegrationManager construction.");
                return;
            }

            s_prefabPublicInterface = AZ::Interface<PrefabPublicInterface>::Get();
            if (s_prefabPublicInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get PrefabPublicInterface on PrefabIntegrationManager construction.");
                return;
            }

            s_prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            if (s_prefabFocusInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get PrefabFocusInterface on PrefabIntegrationManager construction.");
                return;
            }

            s_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
            if (s_prefabFocusPublicInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get PrefabFocusPublicInterface on PrefabIntegrationManager construction.");
                return;
            }

            m_prefabOverridePublicInterface = AZ::Interface<PrefabOverridePublicInterface>::Get();
            if (m_prefabOverridePublicInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get PrefabOverridePublicInterface on PrefabIntegrationManager construction.");
                return;
            }

            m_readOnlyEntityPublicInterface = AZ::Interface<ReadOnlyEntityPublicInterface>::Get();
            AZ_Assert(
                m_readOnlyEntityPublicInterface,
                "Prefab - could not get ReadOnlyEntityPublicInterface on PrefabIntegrationManager construction.");

            // Get EditorEntityContextId
            EditorEntityContextRequestBus::BroadcastResult(s_editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            // Initialize Editor functionality for the Prefab Focus Handler
            auto prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            prefabFocusInterface->InitializeEditorInterfaces();

            m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
            AZ_Assert(
                m_actionManagerInterface, "Prefab - could not get ActionManagerInterface on PrefabIntegrationManager construction.");

            m_hotKeyManagerInterface = AZ::Interface<HotKeyManagerInterface>::Get();
            AZ_Assert(
                m_hotKeyManagerInterface, "Prefab - could not get HotKeyManagerInterface on PrefabIntegrationManager construction.");

            m_menuManagerInterface = AZ::Interface<MenuManagerInterface>::Get();
            AZ_Assert(
                m_menuManagerInterface, "Prefab - could not get MenuManagerInterface on PrefabIntegrationManager construction.");

            m_toolBarManagerInterface = AZ::Interface<ToolBarManagerInterface>::Get();
            AZ_Assert(
                m_toolBarManagerInterface, "Prefab - could not get ToolBarManagerInterface on PrefabIntegrationManager construction.");

            // Register an updater that will refresh actions when a level is loaded.
            if (m_actionManagerInterface)
            {
                ActionManagerRegistrationNotificationBus::Handler::BusConnect();
            }
            
            PrefabFocusNotificationBus::Handler::BusConnect(s_editorEntityContextId);
            PrefabInstanceContainerNotificationBus::Handler::BusConnect();
            AZ::Interface<PrefabIntegrationInterface>::Register(this);
            EntityPropertyEditorNotificationBus::Handler::BusConnect();
            EditorEntityContextNotificationBus::Handler::BusConnect();
            PrefabPublicNotificationBus::Handler::BusConnect();
        }

        PrefabIntegrationManager::~PrefabIntegrationManager()
        {
            if (m_actionManagerInterface)
            {
                ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
            }

            PrefabPublicNotificationBus::Handler::BusDisconnect();
            EditorEntityContextNotificationBus::Handler::BusDisconnect();
            EntityPropertyEditorNotificationBus::Handler::BusDisconnect();
            AZ::Interface<PrefabIntegrationInterface>::Unregister(this);
            PrefabInstanceContainerNotificationBus::Handler::BusDisconnect();
            PrefabFocusNotificationBus::Handler::BusDisconnect();
        }

        void PrefabIntegrationManager::Reflect(AZ::ReflectContext* context)
        {
            PrefabUserSettings::Reflect(context);
        }

        void PrefabIntegrationManager::OnActionUpdaterRegistrationHook()
        {
            // Update actions whenever a new root prefab is loaded.
            m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier);

            // Update actions whenever component selection in the Inspector changes.
            m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::ComponentSelectionChangedUpdaterIdentifier);

            // Update actions whenever Prefab Focus changes (or is refreshed).
            m_actionManagerInterface->RegisterActionUpdater(PrefabIdentifiers::PrefabFocusChangedUpdaterIdentifier);

            // Update actions whenever the prefab instance propagation process ends.
            m_actionManagerInterface->RegisterActionUpdater(PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier);

            // Update actions whenever a Prefab's unsaved state changes.
            m_actionManagerInterface->RegisterActionUpdater(PrefabIdentifiers::PrefabUnsavedStateChangedUpdaterIdentifier);
        }

        void PrefabIntegrationManager::OnActionRegistrationHook()
        {
            // Open/Edit Prefab
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.edit";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Open/Edit Prefab";
                actionProperties.m_description = "Edit the prefab in focus mode.";
                actionProperties.m_category = "Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        if (selectedEntities.size() == 1 &&
                            s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                            !s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()) &&
                            !s_prefabPublicInterface->IsOwnedByProceduralPrefabInstance(selectedEntities.front()))
                        {
                            ContextMenu_EditPrefab(selectedEntities.front());
                        }
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [prefabFocusPublicInterface = s_prefabFocusPublicInterface, prefabPublicInterface = s_prefabPublicInterface]() -> bool
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        return  selectedEntities.size() == 1 &&
                            prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                            !prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()) &&
                            !prefabPublicInterface->IsOwnedByProceduralPrefabInstance(selectedEntities.front());
                    }
                );

                // Trigger update whenever entity selection changes.
                m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

                m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "+");
            }

            // Inspect Procedural Prefab
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.procedural.inspect";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Inspect Procedural Prefab";
                actionProperties.m_description = "See the procedural prefab contents in focus mode.";
                actionProperties.m_category = "Procedural Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        if (selectedEntities.size() == 1 &&
                            s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                            !s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()) &&
                            s_prefabPublicInterface->IsOwnedByProceduralPrefabInstance(selectedEntities.front()))
                        {
                            ContextMenu_EditPrefab(selectedEntities.front());
                        }
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [prefabFocusPublicInterface = s_prefabFocusPublicInterface, prefabPublicInterface = s_prefabPublicInterface]() -> bool
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        return  selectedEntities.size() == 1 &&
                                prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                                !prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()) &&
                                prefabPublicInterface->IsOwnedByProceduralPrefabInstance(selectedEntities.front());
                    }
                );

                // Trigger update whenever entity selection changes.
                m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

                m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "+");
            }

            // Close Prefab
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.close";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Close Prefab";
                actionProperties.m_description = "Close focus mode for this prefab and move one level up.";
                actionProperties.m_category = "Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        if (selectedEntities.size() == 1 && s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                            s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()) &&
                            selectedEntities.front() != s_prefabPublicInterface->GetLevelInstanceContainerEntityId())
                        {
                            ContextMenu_ClosePrefab();
                        }
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    []() -> bool
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        return selectedEntities.size() == 1 &&
                            s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                            s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()) &&
                            selectedEntities.front() != s_prefabPublicInterface->GetLevelInstanceContainerEntityId();
                    }
                );

                // Trigger update whenever entity selection changes.
                m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

                m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "-");
            }

            if (IsOutlinerOverrideManagementEnabled())
            {
                // Open Prefab Instance
                {
                    AZStd::string actionIdentifier = "o3de.action.prefabs.openInstance";
                    AzToolsFramework::ActionProperties actionProperties;
                    actionProperties.m_name = "Open Prefab Instance";
                    actionProperties.m_description = "Open the prefab instance to apply overrides.";
                    actionProperties.m_category = "Prefabs";

                    m_actionManagerInterface->RegisterAction(
                        EditorIdentifiers::MainWindowActionContextIdentifier,
                        actionIdentifier,
                        actionProperties,
                        [this]()
                        {
                            AzToolsFramework::EntityIdList selectedEntities;
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                            if (selectedEntities.size() == 1 &&
                                s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                                !s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()) &&
                                !s_containerEntityInterface->IsContainerOpen(selectedEntities.front()))
                            {
                                ContextMenu_OpenPrefabInstance(selectedEntities.front());
                            }
                        }
                    );

                    m_actionManagerInterface->InstallEnabledStateCallback(
                        actionIdentifier,
                        [prefabFocusPublicInterface = s_prefabFocusPublicInterface,
                         prefabPublicInterface = s_prefabPublicInterface]() -> bool
                        {
                            AzToolsFramework::EntityIdList selectedEntities;
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                            return selectedEntities.size() == 1 &&
                                prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                                !prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()) &&
                                !s_containerEntityInterface->IsContainerOpen(selectedEntities.front());
                        }
                    );
                    
                    // Trigger update whenever entity selection or container entity states change.
                    m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
                    m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::ContainerEntityStatesChangedUpdaterIdentifier, actionIdentifier);

                    // This action is only accessible outside of Component Modes
                    m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
                }

                // Close Prefab Instance
                {
                    AZStd::string actionIdentifier = "o3de.action.prefabs.closeInstance";
                    AzToolsFramework::ActionProperties actionProperties;
                    actionProperties.m_name = "Close Prefab Instance";
                    actionProperties.m_description = "Close this prefab instance.";
                    actionProperties.m_category = "Prefabs";

                    m_actionManagerInterface->RegisterAction(
                        EditorIdentifiers::MainWindowActionContextIdentifier,
                        actionIdentifier,
                        actionProperties,
                        [this]()
                        {
                            AzToolsFramework::EntityIdList selectedEntities;
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                            if (selectedEntities.size() == 1 &&
                                s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                                !s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()) &&
                                s_containerEntityInterface->IsContainerOpen(selectedEntities.front()))
                            {
                                ContextMenu_ClosePrefabInstance(selectedEntities.front());
                            }
                        }
                    );

                    m_actionManagerInterface->InstallEnabledStateCallback(
                        actionIdentifier,
                        [prefabFocusPublicInterface = s_prefabFocusPublicInterface,
                         prefabPublicInterface = s_prefabPublicInterface]() -> bool
                        {
                            AzToolsFramework::EntityIdList selectedEntities;
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                            return selectedEntities.size() == 1 &&
                                prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                                !prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()) &&
                                s_containerEntityInterface->IsContainerOpen(selectedEntities.front());
                        }
                    );

                    // Trigger update whenever entity selection or container entity states change.
                    m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
                    m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::ContainerEntityStatesChangedUpdaterIdentifier, actionIdentifier);

                    // This action is only accessible outside of Component Modes
                    m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
                }

            }

            // Create Prefab
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.create";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Create Prefab...";
                actionProperties.m_description = "Creates a prefab out of the currently selected entities.";
                actionProperties.m_category = "Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        if (CanCreatePrefabWithCurrentSelection(selectedEntities))
                        {
                            ContextMenu_CreatePrefab(selectedEntities);
                        }
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [this]() -> bool
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        return CanCreatePrefabWithCurrentSelection(selectedEntities);
                    }
                );

                // Trigger update whenever entity selection changes, and after prefab instance propagation.
                m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
                m_actionManagerInterface->AddActionToUpdater(PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier, actionIdentifier);

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
            }

            // Detach Prefab
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.detach";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Detach Prefab";
                actionProperties.m_description = "Selected prefab is detached from its prefab file and becomes normal entities instead. "
                                                 "You can change whether or not this action keeps the container entity in the editor settings panel.";
                actionProperties.m_category = "Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        if (CanDetachPrefabWithCurrentSelection(selectedEntities))
                        {
                            ContextMenu_DetachPrefab(selectedEntities.front());
                        }
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [this]() -> bool
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        return CanDetachPrefabWithCurrentSelection(selectedEntities);
                    }
                );

                // Trigger update whenever entity selection changes.
                m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
            }

            // Instantiate Prefab
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.instantiate";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Instantiate Prefab...";
                actionProperties.m_description = "Instantiates a prefab file in the scene.";
                actionProperties.m_category = "Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        if (CanInstantiatePrefabWithCurrentSelection(selectedEntities))
                        {
                            ContextMenu_InstantiatePrefab();
                        }
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [this]() -> bool
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        return CanInstantiatePrefabWithCurrentSelection(selectedEntities);
                    }
                );

                // Trigger update whenever entity selection changes, and after prefab instance propagation.
                m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
                m_actionManagerInterface->AddActionToUpdater(
                    PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier, actionIdentifier);

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
            }

            // Instantiate Procedural Prefab
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.procedural.instantiate";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Instantiate Procedural Prefab...";
                actionProperties.m_description = "Instantiates a procedural prefab file in a prefab.";
                actionProperties.m_category = "Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        if (CanInstantiatePrefabWithCurrentSelection(selectedEntities))
                        {
                            ContextMenu_InstantiateProceduralPrefab();
                        }
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [this]() -> bool
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        return CanInstantiatePrefabWithCurrentSelection(selectedEntities);
                    }
                );

                // Trigger update whenever entity selection changes, and after prefab instance propagation.
                m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
                m_actionManagerInterface->AddActionToUpdater(
                    PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier, actionIdentifier);

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
            }

            // Save Prefab to file
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.save";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Save Prefab to file";
                actionProperties.m_description = "Save the changes to the prefab to disk.";
                actionProperties.m_category = "Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        if (CanSaveUnsavedPrefabChangedInCurrentSelection(selectedEntities))
                        {
                            ContextMenu_SavePrefab(selectedEntities.front());
                        }
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [this]() -> bool
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                        return CanSaveUnsavedPrefabChangedInCurrentSelection(selectedEntities);
                    }
                );

                m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
                m_actionManagerInterface->AddActionToUpdater(PrefabIdentifiers::PrefabUnsavedStateChangedUpdaterIdentifier, actionIdentifier);

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
            }

            // Focus on Level Prefab
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.focusOnLevel";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Focus on top level";
                actionProperties.m_description = "Move the Prefab Focus to the top level.";
                actionProperties.m_category = "Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [prefabFocusPublicInterface = s_prefabFocusPublicInterface]()
                    {
                        prefabFocusPublicInterface->FocusOnOwningPrefab(AZ::EntityId());
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [prefabFocusPublicInterface = s_prefabFocusPublicInterface, editorEntityContextId = s_editorEntityContextId]() -> bool
                    {
                        return prefabFocusPublicInterface->GetPrefabFocusPathLength(editorEntityContextId) > 1;
                    }
                );

                m_actionManagerInterface->AddActionToUpdater(PrefabIdentifiers::PrefabFocusChangedUpdaterIdentifier, actionIdentifier);

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

                m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Esc");
            }

            // Focus one level up
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.focusUpOneLevel";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Focus up one level";
                actionProperties.m_description = "Move the Prefab Focus up one level.";
                actionProperties.m_category = "Prefabs";
                actionProperties.m_iconPath = ":/Breadcrumb/img/UI20/Breadcrumb/arrow_left-default.svg";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [prefabFocusPublicInterface = s_prefabFocusPublicInterface, editorEntityContextId = s_editorEntityContextId]()
                    {
                        prefabFocusPublicInterface->FocusOnParentOfFocusedPrefab(editorEntityContextId);
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [prefabFocusPublicInterface = s_prefabFocusPublicInterface, editorEntityContextId = s_editorEntityContextId]() -> bool
                    {
                        return !ComponentModeFramework::InComponentMode() &&
                            prefabFocusPublicInterface->GetPrefabFocusPathLength(editorEntityContextId) > 1;
                    }
                );

                m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::ComponentModeChangedUpdaterIdentifier, actionIdentifier);
                m_actionManagerInterface->AddActionToUpdater(PrefabIdentifiers::PrefabFocusChangedUpdaterIdentifier, actionIdentifier);
            }

            if (IsOutlinerOverrideManagementEnabled())
            {
                // Revert overrides
                {
                    AZStd::string actionIdentifier = "o3de.action.prefabs.revertInstanceOverrides";
                    AzToolsFramework::ActionProperties actionProperties;
                    actionProperties.m_name = "Revert overrides";
                    actionProperties.m_description = "Revert all overrides on this entity.";
                    actionProperties.m_category = "Prefabs";

                    m_actionManagerInterface->RegisterAction(
                        EditorIdentifiers::MainWindowActionContextIdentifier,
                        actionIdentifier,
                        actionProperties,
                        [this]()
                        {
                            AzToolsFramework::EntityIdList selectedEntities;
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                            if (selectedEntities.size() != 1)
                            {
                                return;
                            }

                            AZ::EntityId selectedEntityId = selectedEntities.front();

                            if (!s_prefabPublicInterface->IsOwnedByPrefabInstance(selectedEntityId))
                            {
                                return;
                            }

                            if (!s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntityId) &&
                                m_prefabOverridePublicInterface->AreOverridesPresent(selectedEntityId) &&
                                m_prefabOverridePublicInterface->GetEntityOverrideType(selectedEntityId) != OverrideType::AddEntity)
                            {
                                ContextMenu_RevertOverrides(selectedEntityId);
                            }
                        }
                    );

                    m_actionManagerInterface->InstallEnabledStateCallback(
                        actionIdentifier,
                        [prefabPublicInterface = s_prefabPublicInterface,
                         prefabOverridePublicInterface = m_prefabOverridePublicInterface]() -> bool
                        {
                            AzToolsFramework::EntityIdList selectedEntities;
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                            if (selectedEntities.size() != 1)
                            {
                                return false;
                            }

                            AZ::EntityId selectedEntityId = selectedEntities.front();

                            // Returns false if the selected entity is not owned by a prefab instance. This could happen when the callback is
                            // triggered after entity selection changes, but prefab propagation does not finish yet to create/reload the entity.
                            if (!prefabPublicInterface->IsOwnedByPrefabInstance(selectedEntityId))
                            {
                                return false;
                            }

                            return !prefabPublicInterface->IsInstanceContainerEntity(selectedEntityId) &&
                                prefabOverridePublicInterface->AreOverridesPresent(selectedEntityId) &&
                                prefabOverridePublicInterface->GetEntityOverrideType(selectedEntityId) != OverrideType::AddEntity;
                        }
                    );

                    // Refresh this action whenever instance propagation ends, as that could have changed overrideson the current selection.
                    m_actionManagerInterface->AddActionToUpdater(
                        EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
                    m_actionManagerInterface->AddActionToUpdater(
                        PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier, actionIdentifier);
                }
            }

            // Revert overrides on Component
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.revertComponentOverrides";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Revert overrides";
                actionProperties.m_description = "Revert all overrides on this component.";
                actionProperties.m_category = "Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    []()
                    {
                        if (auto prefabOverridePublicInterface =
                                AZ::Interface<AzToolsFramework::Prefab::PrefabOverridePublicInterface>::Get())
                        {
                            EntityPropertyEditorRequestBus::Broadcast(
                                &EntityPropertyEditorRequestBus::Events::VisitComponentEditors,
                                [prefabOverridePublicInterface](const AzToolsFramework::ComponentEditor* componentEditor)
                                {
                                    if (componentEditor->IsSelected())
                                    {
                                        auto components = componentEditor->GetComponents();

                                        if (components.size() > 1)
                                        {
                                            // We do not support multi-editing with overrides.
                                            return false;
                                        }

                                        const auto& component = components.front();
                                        prefabOverridePublicInterface->RevertComponentOverrides(
                                            AZ::EntityComponentIdPair(component->GetEntityId(), component->GetId()));
                                    }

                                    return true;
                                }
                            );
                        }
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [prefabOverridePublicInterface = m_prefabOverridePublicInterface]() -> bool
                    {
                        // Retrieve selected component in the Inspector.
                        AZStd::unordered_set<AZ::EntityComponentIdPair> selectedEntityComponentIds;
                        EntityPropertyEditorRequestBus::Broadcast(
                            &EntityPropertyEditorRequests::GetSelectedComponents, selectedEntityComponentIds);

                        if (selectedEntityComponentIds.size() != 1)
                        {
                            // Override handling doesn't support multi-editing.
                            return false;
                        }

                        const auto& selectedEntityComponentId = *selectedEntityComponentIds.begin();
                        return prefabOverridePublicInterface->AreComponentOverridesPresent(selectedEntityComponentId);
                    }
                );

                // Refresh this action whenever instance propagation ends, as that could have changed overrides on the current selection.
                m_actionManagerInterface->AddActionToUpdater(
                    EditorIdentifiers::ComponentSelectionChangedUpdaterIdentifier, actionIdentifier);
                m_actionManagerInterface->AddActionToUpdater(
                    EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
                m_actionManagerInterface->AddActionToUpdater(
                    PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier, actionIdentifier);
            }

            // Apply overrides on Component
            {
                AZStd::string actionIdentifier = "o3de.action.prefabs.applyComponentOverrides";
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Apply overrides";
                actionProperties.m_description = "Apply all overrides on this component.";
                actionProperties.m_category = "Prefabs";

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    []()
                    {
                        if (auto prefabOverridePublicInterface =
                                AZ::Interface<AzToolsFramework::Prefab::PrefabOverridePublicInterface>::Get())
                        {
                            EntityPropertyEditorRequestBus::Broadcast(
                                &EntityPropertyEditorRequestBus::Events::VisitComponentEditors,
                                [prefabOverridePublicInterface](const AzToolsFramework::ComponentEditor* componentEditor)
                                {
                                    if (componentEditor->IsSelected())
                                    {
                                        auto components = componentEditor->GetComponents();

                                        if (components.size() > 1)
                                        {
                                            // We do not support multi-editing with overrides.
                                            return false;
                                        }

                                        const auto& component = components.front();
                                        prefabOverridePublicInterface->ApplyComponentOverrides(
                                                AZ::EntityComponentIdPair(component->GetEntityId(), component->GetId()));
                                    }

                                    return true;
                                }
                            );
                        }
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    actionIdentifier,
                    [prefabOverridePublicInterface = m_prefabOverridePublicInterface]() -> bool
                    {
                        // Retrieve selected component in the Inspector.
                        AZStd::unordered_set<AZ::EntityComponentIdPair> selectedEntityComponentIds;
                        EntityPropertyEditorRequestBus::Broadcast(
                            &EntityPropertyEditorRequests::GetSelectedComponents, selectedEntityComponentIds);

                        if (selectedEntityComponentIds.size() != 1)
                        {
                            // Override handling doesn't support multi-editing.
                            return false;
                        }

                        const auto& selectedEntityComponentId = *selectedEntityComponentIds.begin();
                        return prefabOverridePublicInterface->AreComponentOverridesPresent(selectedEntityComponentId);
                    }
                );

                // Refresh this action whenever instance propagation ends, as that could have changed overrides on the current selection.
                m_actionManagerInterface->AddActionToUpdater(
                    EditorIdentifiers::ComponentSelectionChangedUpdaterIdentifier, actionIdentifier);
                m_actionManagerInterface->AddActionToUpdater(
                    EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
                m_actionManagerInterface->AddActionToUpdater(
                    PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier, actionIdentifier);
            }
        }

        void PrefabIntegrationManager::OnWidgetActionRegistrationHook()
        {
            // Prefab Focus Path Widget
            {
                AzToolsFramework::WidgetActionProperties widgetActionProperties;
                widgetActionProperties.m_name = "Prefab Focus Path";
                widgetActionProperties.m_category = "Prefabs";

                auto outcome = m_actionManagerInterface->RegisterWidgetAction(
                    "o3de.widgetAction.prefab.focusPath",
                    widgetActionProperties,
                    []() -> QWidget*
                    {
                        return new PrefabFocusPathWidget();
                    }
                );
            }
        }

        void PrefabIntegrationManager::OnMenuRegistrationHook()
        {
            {
                AzToolsFramework::MenuProperties menuProperties;
                menuProperties.m_name = "Inspector Entity Component Context Menu";
                m_menuManagerInterface->RegisterMenu(EditorIdentifiers::InspectorEntityComponentContextMenuIdentifier, menuProperties);
            }

            {
                AzToolsFramework::MenuProperties menuProperties;
                menuProperties.m_name = "Inspector Entity Property Context Menu";
                m_menuManagerInterface->RegisterMenu(EditorIdentifiers::InspectorEntityComponentContextMenuIdentifier, menuProperties);
            }
        }

        void PrefabIntegrationManager::OnMenuBindingHook()
        {
            // Entity Outliner Context Menu
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.edit", 10500);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.procedural.inspect", 10600);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.close", 10700);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.openInstance", 10800);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.closeInstance", 10900);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.create", 20100);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.detach", 20200);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.detach_and_keep_container_entities", 20210);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.instantiate", 20300);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.procedural.instantiate", 20400);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.save", 30100);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.prefabs.revertInstanceOverrides", 30200);

            // Viewport Context Menu
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.edit", 10500);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.procedural.inspect", 10600);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.close", 10700);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.openInstance", 10800);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.closeInstance", 10900);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.create", 20100);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.detach", 20200);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.detach_and_keep_container_entities", 20210);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.instantiate", 20300);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.procedural.instantiate", 20400);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.save", 30100);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.prefabs.revertInstanceOverrides", 30200);

            // Inspector Entity Component Context Menu
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::InspectorEntityComponentContextMenuIdentifier, "o3de.action.prefabs.applyComponentOverrides", 10000);   
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::InspectorEntityComponentContextMenuIdentifier, "o3de.action.prefabs.revertComponentOverrides", 10100);   
        }

        void PrefabIntegrationManager::OnToolBarBindingHook()
        {
            // Populate Viewport top toolbar with Prefab actions and widgets
            m_toolBarManagerInterface->AddActionToToolBar(EditorIdentifiers::ViewportTopToolBarIdentifier, "o3de.action.prefabs.focusUpOneLevel", 100);
            m_toolBarManagerInterface->AddWidgetToToolBar(EditorIdentifiers::ViewportTopToolBarIdentifier, "o3de.widgetAction.prefab.focusPath", 200);
        }

        void PrefabIntegrationManager::OnPostActionManagerRegistrationHook()
        {
            // It should not be possible to Delete the container entity of the focused instance.
            m_actionManagerInterface->InstallEnabledStateCallback(
                "o3de.action.edit.delete",
                []()
                {
                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    // If only one entity is selected, don't show the option if it's the container entity of the focused instance.
                    if (selectedEntities.size() == 1 &&
                        s_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(s_editorEntityContextId) == selectedEntities.front())
                    {
                        return false;
                    }

                    // If multiple entities are selected, they should all be owned by the same instance.
                    if (!s_prefabPublicInterface->EntitiesBelongToSameInstance(selectedEntities))
                    {
                        return false;
                    }
                    
                    return true;
                }
            );

            m_actionManagerInterface->InstallEnabledStateCallback(
                "o3de.action.edit.duplicate",
                []()
                {
                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    if (s_prefabPublicInterface->EntitiesBelongToSameInstance(selectedEntities))
                    {
                        AZ::EntityId entityToCheck = selectedEntities[0];

                        // If it is a container entity, then check its parent entity's owning instance instead.
                        if (s_prefabPublicInterface->IsInstanceContainerEntity(entityToCheck))
                        {
                            AZ::TransformBus::EventResult(entityToCheck, entityToCheck, &AZ::TransformBus::Events::GetParentId);
                        }

                        // Do not show the option when it is not a prefab edit.
                        return s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityToCheck);
                    }

                    return false;
                }
            );

            // Update the duplicate action after Prefab instance propagation.
            m_actionManagerInterface->AddActionToUpdater(
                PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier, "o3de.action.edit.duplicate");

            // Update the move up/move down actions after Prefab instance propagation.
            m_actionManagerInterface->AddActionToUpdater(
                PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier, "o3de.action.entitySorting.moveUp");
            m_actionManagerInterface->AddActionToUpdater(
                PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier, "o3de.action.entitySorting.moveDown");
        }

        void PrefabIntegrationManager::OnStartPlayInEditorBegin()
        {
            // Focus on the root prefab (AZ::EntityId() will default to it)
            s_prefabFocusPublicInterface->FocusOnOwningPrefab(AZ::EntityId());
        }

        void PrefabIntegrationManager::OnStopPlayInEditor()
        {
            // Refresh all containers when leaving Game Mode to ensure everything is synced.
            QTimer::singleShot(
                0,
                [&]()
                {
                    s_containerEntityInterface->RefreshAllContainerEntities(s_editorEntityContextId);
                }
            );
        }

        
        void PrefabIntegrationManager::OnComponentSelectionChanged(EntityPropertyEditor*, const AZStd::unordered_set<AZ::EntityComponentIdPair>&)
        {
            if (m_actionManagerInterface)
            {
                m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::ComponentSelectionChangedUpdaterIdentifier);
            }
        }

        bool PrefabIntegrationManager::CanCreatePrefabWithCurrentSelection(const AzToolsFramework::EntityIdList& selectedEntities)
        {
            if (selectedEntities.empty())
            {
                // Can't create an empty prefab.
                return false;
            }

            if (selectedEntities.size() == 1 && s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()))
            {
                // Can't create if the only selected entity is the Focused Instance Container.
                return false;
            }

            // Check all entities
            for (const auto& entityId : selectedEntities)
            {
                if (m_readOnlyEntityPublicInterface->IsReadOnly(entityId))
                {
                    // Can't create a prefab with read-only entities in it.
                    return false;
                }
            }

            if (!s_prefabPublicInterface->EntitiesBelongToSameInstance(selectedEntities))
            {
                // All selected entities must belong to the same instance to create a prefab.
                return false;
            }

            AZ::EntityId entityToCheck = selectedEntities[0];

            // If it is a container entity, then check its parent entity's owning instance instead.
            if (s_prefabPublicInterface->IsInstanceContainerEntity(entityToCheck))
            {
                AZ::TransformBus::EventResult(entityToCheck, entityToCheck, &AZ::TransformBus::Events::GetParentId);
            }

            // Do not show the option when it is not a prefab edit.
            if (!s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityToCheck))
            {
                return false;
            }

            return true;
        };

        bool PrefabIntegrationManager::CanDetachPrefabWithCurrentSelection(const AzToolsFramework::EntityIdList& selectedEntities)
        {
            if (selectedEntities.size() == 1 &&
                s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntities.front()) &&
                !s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntities.front()))
            {
                AZ::EntityId parentEntityId;
                AZ::TransformBus::EventResult(parentEntityId, selectedEntities.front(), &AZ::TransformBus::Events::GetParentId);

                // Do not show the option when it is not a prefab edit.
                if (s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(parentEntityId))
                {
                    return true;
                }
            }

            return false;
        };

        bool PrefabIntegrationManager::CanInstantiatePrefabWithCurrentSelection(const AzToolsFramework::EntityIdList& selectedEntities)
        {
            if (selectedEntities.empty())
            {
                // Can always instantiate with no selection.
                // New instance will be parented to the container entity of the currently focused prefab.
                return true;
            }

            if (selectedEntities.size() > 1)
            {
                // Can't instantiate a prefab instance with multiple parents.
                return false;
            }

            AZ::EntityId selectedEntityId = selectedEntities.front();

            if (m_readOnlyEntityPublicInterface->IsReadOnly(selectedEntityId))
            {
                // Can't instantiate as a child of a read-only entity.
                return false;
            }

            if (s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntityId) ||
                !s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntityId))
            {
                // Can't instantiate under prefab that is not in focus.
                return false;
            }

            return true;
        }

        bool PrefabIntegrationManager::CanSaveUnsavedPrefabChangedInCurrentSelection(const AzToolsFramework::EntityIdList& selectedEntities)
        {
            if (selectedEntities.size() != 1)
            {
                // This operation only supports one selection at a time.
                return false;
            }

            AZ::EntityId selectedEntityId = selectedEntities.front();

            if (!s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntityId))
            {
                // Entity needs to be a prefab instance container.
                return false;
            }

            AZ::IO::Path prefabFilePath = s_prefabPublicInterface->GetOwningInstancePrefabPath(selectedEntityId);
            auto dirtyOutcome = s_prefabPublicInterface->HasUnsavedChanges(prefabFilePath);

            if (!dirtyOutcome.IsSuccess() || dirtyOutcome.GetValue() == false)
            {
                // No change to save.
                return false;
            }

            return true;
        }

        void PrefabIntegrationManager::ContextMenu_CreatePrefab(AzToolsFramework::EntityIdList selectedEntities)
        {
            // Save a reference to our currently active window since it will be
            // temporarily null after QFileDialogs close, which we need in order to
            // be able to parent our message dialogs properly
            QWidget* activeWindow = QApplication::activeWindow();
            const AZStd::string prefabFilesPath = "@projectroot@/Prefabs";

            // Remove focused instance container entity if it's part of the list
            auto focusedContainerIter = AZStd::find(
                selectedEntities.begin(), selectedEntities.end(),
                s_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(s_editorEntityContextId));
            if (focusedContainerIter != selectedEntities.end())
            {
                selectedEntities.erase(focusedContainerIter);
            }

            if (selectedEntities.empty())
            {
                return;
            }

            // Set default folder for prefabs
            AZ::IO::FileIOBase* fileIoBaseInstance = AZ::IO::FileIOBase::GetInstance();

            if (fileIoBaseInstance == nullptr)
            {
                AZ_Assert(false, "Prefab - could not find FileIoBaseInstance on CreatePrefab.");
                return;
            }

            if (!fileIoBaseInstance->Exists(prefabFilesPath.c_str()))
            {
                fileIoBaseInstance->CreatePath(prefabFilesPath.c_str());
            }

            char targetDirectory[AZ_MAX_PATH_LEN] = { 0 };
            fileIoBaseInstance->ResolvePath(prefabFilesPath.c_str(), targetDirectory, AZ_MAX_PATH_LEN);

            AzToolsFramework::EntityIdSet entitiesToIncludeInAsset(selectedEntities.begin(), selectedEntities.end());
            {
                AzToolsFramework::EntityIdSet allReferencedEntities;
                bool hasExternalReferences = false;
                GatherAllReferencedEntitiesAndCompare(entitiesToIncludeInAsset, allReferencedEntities, hasExternalReferences);

                if (hasExternalReferences)
                {
                    bool useAllReferencedEntities = false;
                    bool continueCreation =
                        QueryAndPruneMissingExternalReferences(entitiesToIncludeInAsset, allReferencedEntities, useAllReferencedEntities);
                    if (!continueCreation)
                    {
                        // User canceled the operation
                        return;
                    }

                    if (useAllReferencedEntities)
                    {
                        entitiesToIncludeInAsset = allReferencedEntities;
                    }
                }
            }

            // Determine prefab asset file name/path - come up with default suggested name, ask user
            AZStd::string prefabName;
            AZStd::string prefabFilePath;
            {
                AZStd::string suggestedName;
                AzToolsFramework::EntityIdList prefabRootEntities;
                {
                    AZ::EntityId commonRoot;
                    bool hasCommonRoot = false;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        hasCommonRoot, &AzToolsFramework::ToolsApplicationRequests::FindCommonRoot, entitiesToIncludeInAsset, commonRoot,
                        &prefabRootEntities);
                    if (hasCommonRoot && commonRoot.IsValid() &&
                        entitiesToIncludeInAsset.find(commonRoot) != entitiesToIncludeInAsset.end())
                    {
                        prefabRootEntities.insert(prefabRootEntities.begin(), commonRoot);
                    }
                }

                PrefabSaveHandler::GenerateSuggestedFilenameFromEntities(prefabRootEntities, suggestedName);

                if (!PrefabSaveHandler::QueryUserForPrefabSaveLocation(
                        suggestedName, targetDirectory, AZ_CRC_CE("PrefabUserSettings"), activeWindow, prefabName, prefabFilePath))
                {
                    // User canceled prefab creation, or error prevented continuation.
                    return;
                }
            }

            auto createPrefabOutcome = s_prefabPublicInterface->CreatePrefabAndSaveToDisk(selectedEntities, prefabFilePath.data());

            if (!createPrefabOutcome.IsSuccess())
            {
                WarningDialog("Prefab Creation Error", createPrefabOutcome.GetError());
            }
        }

        void PrefabIntegrationManager::ContextMenu_InstantiatePrefab()
        {
            EntityIdList selectedEntities;
            ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);

            // Can't instantiate as child of multiple entities.
            if (selectedEntities.size() > 1)
            {
                return;
            }

            // Can't instantiate under a closed container or a read-only entity.
            if (selectedEntities.size() == 1)
            {
                AZ::EntityId selectedEntityId = selectedEntities.front();
                bool selectedEntityIsReadOnly = m_readOnlyEntityPublicInterface->IsReadOnly(selectedEntityId);
                auto containerEntityInterface = AZ::Interface<AzToolsFramework::ContainerEntityInterface>::Get();

                if (!containerEntityInterface || !containerEntityInterface->IsContainerOpen(selectedEntityId) || selectedEntityIsReadOnly)
                {
                    return;
                }
            }

            // Retrieve the cursor position before querying for file path so it is retrieved correctly.
            AZ::Vector3 position = AZ::Vector3::CreateZero();
            EditorRequestBus::BroadcastResult(position, &EditorRequestBus::Events::GetWorldPositionAtViewportInteraction);

            // Query for the prefab to instantiate.
            AZStd::string prefabFilePath;
            if (PrefabSaveHandler::QueryUserForPrefabFilePath(prefabFilePath))
            {
                AZ::EntityId parentId = AZ::EntityId();

                // When a single entity is selected, instance is created as its child.
                if (selectedEntities.size() == 1)
                {
                    // Set the selected entity as the parent.
                    parentId = selectedEntities.front();
                }

                auto createPrefabOutcome = s_prefabPublicInterface->InstantiatePrefab(prefabFilePath, parentId, position);
                if (!createPrefabOutcome.IsSuccess())
                {
                    WarningDialog("Prefab Instantiation Error", createPrefabOutcome.GetError());
                }
            }
        }

        void PrefabIntegrationManager::ContextMenu_InstantiateProceduralPrefab()
        {
            AZStd::string prefabAssetPath;
            bool hasUserForProceduralPrefabAsset = PrefabSaveHandler::QueryUserForProceduralPrefabAsset(prefabAssetPath);

            if (hasUserForProceduralPrefabAsset)
            {
                AZ::EntityId parentId;
                AZ::Vector3 position = AZ::Vector3::CreateZero();

                EntityIdList selectedEntities;
                ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);
                if (selectedEntities.size() == 1)
                {
                    parentId = selectedEntities.front();
                    AZ::TransformBus::EventResult(position, parentId, &AZ::TransformInterface::GetWorldTranslation);
                }
                else
                {
                    EditorRequestBus::BroadcastResult(position, &EditorRequestBus::Events::GetWorldPositionAtViewportCenter);
                }

                auto createPrefabOutcome = s_prefabPublicInterface->InstantiatePrefab(prefabAssetPath, parentId, position);
                if (!createPrefabOutcome.IsSuccess())
                {
                    WarningDialog("Procedural Prefab Instantiation Error", createPrefabOutcome.GetError());
                }
            }
        }

        void PrefabIntegrationManager::ContextMenu_ClosePrefab()
        {
            s_prefabFocusPublicInterface->FocusOnParentOfFocusedPrefab(s_editorEntityContextId);
        }

        void PrefabIntegrationManager::ContextMenu_EditPrefab(AZ::EntityId containerEntity)
        {
            s_prefabFocusPublicInterface->FocusOnOwningPrefab(containerEntity);
        }

        void PrefabIntegrationManager::ContextMenu_SavePrefab(AZ::EntityId containerEntity)
        {
            m_prefabSaveHandler.ExecuteSavePrefabDialog(containerEntity);
        }

        void PrefabIntegrationManager::ContextMenu_ClosePrefabInstance(AZ::EntityId containerEntity)
        {
            s_prefabFocusPublicInterface->SetOwningPrefabInstanceOpenState(containerEntity, false);
        }

        void PrefabIntegrationManager::ContextMenu_OpenPrefabInstance(AZ::EntityId containerEntity)
        {
            s_prefabFocusPublicInterface->SetOwningPrefabInstanceOpenState(containerEntity, true);
        }

        void PrefabIntegrationManager::ContextMenu_Duplicate()
        {
            bool handled = true;
            AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::CloneSelection, handled);
        }

        void PrefabIntegrationManager::ContextMenu_DeleteSelected()
        {
            AzToolsFramework::EntityIdList selectedEntityIds;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
            PrefabOperationResult deleteSelectedResult =
                s_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance(selectedEntityIds);
            if (!deleteSelectedResult.IsSuccess())
            {
                WarningDialog("Delete selected entities error", deleteSelectedResult.GetError());
            }
        }

        void PrefabIntegrationManager::ContextMenu_DetachPrefab(AZ::EntityId containerEntity)
        {
            bool shouldRemoveContainer = AzToolsFramework::GetRegistry(Settings::DetachPrefabRemovesContainerName, Settings::DetachPrefabRemovesContainerDefault);

            PrefabOperationResult detachPrefabResult = shouldRemoveContainer ? 
                       s_prefabPublicInterface->DetachPrefabAndRemoveContainerEntity(containerEntity)
                    : s_prefabPublicInterface->DetachPrefab(containerEntity);

            if (!detachPrefabResult.IsSuccess())
            {
                WarningDialog("Detach Prefab error", detachPrefabResult.GetError());
            }
        }

        void PrefabIntegrationManager::ContextMenu_RevertOverrides(AZ::EntityId entityId)
        {
            m_prefabOverridePublicInterface->RevertOverrides(entityId);
        }

        void PrefabIntegrationManager::GatherAllReferencedEntitiesAndCompare(
            const EntityIdSet& entities, EntityIdSet& entitiesAndReferencedEntities, bool& hasExternalReferences)
        {
            AZ::SerializeContext* serializeContext;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            entitiesAndReferencedEntities.clear();
            entitiesAndReferencedEntities = entities;
            GatherAllReferencedEntities(entitiesAndReferencedEntities, *serializeContext);

            // NOTE: that AZStd::unordered_set equality operator only returns true if they are in the same order
            // (which appears to deviate from the standard). So we have to do the comparison ourselves.
            hasExternalReferences = (entitiesAndReferencedEntities.size() > entities.size());
            if (!hasExternalReferences)
            {
                for (AZ::EntityId id : entitiesAndReferencedEntities)
                {
                    if (entities.find(id) == entities.end())
                    {
                        hasExternalReferences = true;
                        break;
                    }
                }
            }
        }

        AZ::u32 PrefabIntegrationManager::GetSliceFlags(const AZ::Edit::ElementData* editData, const AZ::Edit::ClassData* classData)
        {
            AZ::u32 sliceFlags = 0;

            if (editData)
            {
                AZ::Edit::Attribute* slicePushAttribute = editData->FindAttribute(AZ::Edit::Attributes::SliceFlags);
                if (slicePushAttribute)
                {
                    AZ::u32 elementSliceFlags = 0;
                    AzToolsFramework::PropertyAttributeReader reader(nullptr, slicePushAttribute);
                    reader.Read<AZ::u32>(elementSliceFlags);
                    sliceFlags |= elementSliceFlags;
                }
            }

            const AZ::Edit::ElementData* classEditData =
                classData ? classData->FindElementData(AZ::Edit::ClassElements::EditorData) : nullptr;
            if (classEditData)
            {
                AZ::Edit::Attribute* slicePushAttribute = classEditData->FindAttribute(AZ::Edit::Attributes::SliceFlags);
                if (slicePushAttribute)
                {
                    AZ::u32 classSliceFlags = 0;
                    AzToolsFramework::PropertyAttributeReader reader(nullptr, slicePushAttribute);
                    reader.Read<AZ::u32>(classSliceFlags);
                    sliceFlags |= classSliceFlags;
                }
            }

            return sliceFlags;
        }

        void PrefabIntegrationManager::GatherAllReferencedEntities(
            EntityIdSet& entitiesWithReferences, AZ::SerializeContext& serializeContext)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZStd::vector<AZ::EntityId> floodQueue;
            floodQueue.reserve(entitiesWithReferences.size());

            // Seed with all provided entity Ids
            for (const AZ::EntityId& entityId : entitiesWithReferences)
            {
                floodQueue.push_back(entityId);
            }

            // Flood-fill via outgoing entity references and gather all unique visited entities.
            while (!floodQueue.empty())
            {
                const AZ::EntityId id = floodQueue.back();
                floodQueue.pop_back();

                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, id);

                if (entity)
                {
                    AZStd::vector<const AZ::SerializeContext::ClassData*> parentStack;
                    parentStack.reserve(30);
                    auto beginCB = [&](void* ptr, const AZ::SerializeContext::ClassData* classData,
                                       const AZ::SerializeContext::ClassElement* elementData) -> bool
                    {
                        parentStack.push_back(classData);

                        AZ::u32 sliceFlags =
                            GetSliceFlags(elementData ? elementData->m_editData : nullptr, classData ? classData->m_editData : nullptr);

                        // Skip any class or element marked as don't gather references
                        if (0 != (sliceFlags & AZ::Edit::SliceFlags::DontGatherReference))
                        {
                            return false;
                        }

                        if (classData->m_typeId == AZ::SerializeTypeInfo<AZ::EntityId>::GetUuid())
                        {
                            if (!parentStack.empty() && parentStack.back()->m_typeId == AZ::SerializeTypeInfo<AZ::Entity>::GetUuid())
                            {
                                // Ignore the entity's actual Id field. We're only looking for references.
                            }
                            else
                            {
                                AZ::EntityId* entityIdPtr = (elementData->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
                                    ? *reinterpret_cast<AZ::EntityId**>(ptr)
                                    : reinterpret_cast<AZ::EntityId*>(ptr);
                                if (entityIdPtr)
                                {
                                    const AZ::EntityId id = *entityIdPtr;
                                    if (id.IsValid())
                                    {
                                        if (entitiesWithReferences.insert(id).second)
                                        {
                                            floodQueue.push_back(id);
                                        }
                                    }
                                }
                            }
                        }

                        // Keep recursing.
                        return true;
                    };

                    auto endCB = [&]() -> bool
                    {
                        parentStack.pop_back();
                        return true;
                    };

                    AZ::SerializeContext::EnumerateInstanceCallContext callContext(
                        beginCB, endCB, &serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ, nullptr);

                    serializeContext.EnumerateInstanceConst(&callContext, entity, azrtti_typeid<AZ::Entity>(), nullptr, nullptr);
                }
            }
        }

        bool PrefabIntegrationManager::QueryAndPruneMissingExternalReferences(
            EntityIdSet& entities, EntityIdSet& selectedAndReferencedEntities, bool& useReferencedEntities, bool defaultMoveExternalRefs)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            useReferencedEntities = false;

            AZStd::string includedEntities;
            AZStd::string referencedEntities;
            AzToolsFramework::EntityIdList missingEntityIds;
            for (const AZ::EntityId& id : selectedAndReferencedEntities)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, id);
                if (entity)
                {
                    if (entities.find(id) != entities.end())
                    {
                        includedEntities.append("    ");
                        includedEntities.append(entity->GetName());
                        includedEntities.append("\r\n");
                    }
                    else
                    {
                        referencedEntities.append("    ");
                        referencedEntities.append(entity->GetName());
                        referencedEntities.append("\r\n");
                    }
                }
                else
                {
                    missingEntityIds.push_back(id);
                }
            }

            if (!referencedEntities.empty())
            {
                if (!defaultMoveExternalRefs)
                {
                    AZ_PROFILE_FUNCTION(AzToolsFramework);

                    const AZStd::string message = AZStd::string::format(
                        "Entity references may not be valid if the entity IDs change or if the entities do not exist when the prefab is "
                        "instantiated.\r\n\r\nSelected Entities\n%s\nReferenced Entities\n%s\n",
                        includedEntities.c_str(), referencedEntities.c_str());

                    QMessageBox msgBox(AzToolsFramework::GetActiveWindow());
                    msgBox.setWindowTitle("External Entity References");
                    msgBox.setText("The prefab contains references to external entities that are not selected.");
                    msgBox.setInformativeText("You can move the referenced entities into this prefab or retain the external references.");
                    QAbstractButton* moveButton = (QAbstractButton*)msgBox.addButton("Move", QMessageBox::YesRole);
                    QAbstractButton* retainButton = (QAbstractButton*)msgBox.addButton("Retain", QMessageBox::NoRole);
                    msgBox.setStandardButtons(QMessageBox::Cancel);
                    msgBox.setDefaultButton(QMessageBox::Yes);
                    msgBox.setDetailedText(message.c_str());
                    msgBox.exec();

                    if (msgBox.clickedButton() == moveButton)
                    {
                        useReferencedEntities = true;
                    }
                    else if (msgBox.clickedButton() != retainButton)
                    {
                        return false;
                    }
                }
                else
                {
                    useReferencedEntities = true;
                }
            }

            for (const AZ::EntityId& missingEntityId : missingEntityIds)
            {
                entities.erase(missingEntityId);
                selectedAndReferencedEntities.erase(missingEntityId);
            }
            return true;
        }

        void PrefabIntegrationManager::OnPrefabComponentActivate(AZ::EntityId entityId)
        {
            // Register entity to appropriate UI Handler for UI overrides
            if (s_prefabPublicInterface->IsLevelInstanceContainerEntity(entityId))
            {
                s_editorEntityUiInterface->RegisterEntity(entityId, m_levelRootUiHandler.GetHandlerId());
            }
            else
            {
                if (s_prefabPublicInterface->IsOwnedByProceduralPrefabInstance(entityId))
                {
                    s_editorEntityUiInterface->RegisterEntity(entityId, m_proceduralPrefabUiHandler.GetHandlerId());
                }
                else
                {
                    s_editorEntityUiInterface->RegisterEntity(entityId, m_prefabUiHandler.GetHandlerId());
                }

                // Register entity as a container
                s_containerEntityInterface->RegisterEntityAsContainer(entityId);
            }
        }

        void PrefabIntegrationManager::OnPrefabComponentDeactivate(AZ::EntityId entityId)
        {
            if (!s_prefabPublicInterface->IsLevelInstanceContainerEntity(entityId))
            {
                // Unregister entity as a container
                s_containerEntityInterface->UnregisterEntityAsContainer(entityId);
            }

            // Unregister entity from UI Handler
            s_editorEntityUiInterface->UnregisterEntity(entityId);
        }

        AZ::EntityId PrefabIntegrationManager::CreateNewEntityAtPosition(const AZ::Vector3& position, AZ::EntityId parentId)
        {
            Prefab::PrefabPublicInterface* prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
            auto createResult = prefabPublicInterface->CreateEntity(parentId, position);
            if (createResult.IsSuccess())
            {
                return createResult.GetValue();
            }
            else
            {
                WarningDialog("Entity Creation Error", createResult.GetError());
                return AZ::EntityId();
            }
        }

        int PrefabIntegrationManager::HandleRootPrefabClosure(TemplateId templateId)
        {
            return m_prefabSaveHandler.ExecuteClosePrefabDialog(templateId);
        }

        void PrefabIntegrationManager::SaveCurrentPrefab()
        {
            if (s_prefabFocusInterface)
            {
                TemplateId currentTemplateId = s_prefabFocusInterface->GetFocusedPrefabTemplateId(s_editorEntityContextId);
                m_prefabSaveHandler.ExecuteSavePrefabDialog(currentTemplateId, true);
            }
        }

        void PrefabIntegrationManager::OnRootPrefabInstanceLoaded()
        {
            if (m_actionManagerInterface)
            {
                m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier);
            }

            // Lazily initialize the PrefabToastNotificationsView so that the main window is ready to show the prefab toasts.
            m_prefabSaveHandler.InitializePrefabToastNotificationsView();
        }

        void PrefabIntegrationManager::OnPrefabTemplateDirtyFlagUpdated(
            [[maybe_unused]] TemplateId templateId, [[maybe_unused]] bool status)
        {
            if (m_actionManagerInterface)
            {
                m_actionManagerInterface->TriggerActionUpdater(PrefabIdentifiers::PrefabUnsavedStateChangedUpdaterIdentifier);
            }
        }

        void PrefabIntegrationManager::OnPrefabInstancePropagationEnd()
        {
            if (m_actionManagerInterface)
            {
                m_actionManagerInterface->TriggerActionUpdater(PrefabIdentifiers::PrefabInstancePropagationEndUpdaterIdentifier);
            }
        }

        void PrefabIntegrationManager::OnPrefabFocusChanged(
            [[maybe_unused]] AZ::EntityId previousContainerEntityId, [[maybe_unused]] AZ::EntityId newContainerEntityId)
        {
            if (m_actionManagerInterface)
            {
                m_actionManagerInterface->TriggerActionUpdater(PrefabIdentifiers::PrefabFocusChangedUpdaterIdentifier);
            }
        }

        void PrefabIntegrationManager::OnPrefabFocusRefreshed()
        {
            if (m_actionManagerInterface)
            {
                m_actionManagerInterface->TriggerActionUpdater(PrefabIdentifiers::PrefabFocusChangedUpdaterIdentifier);
            }
        }

    } // namespace Prefab

} // namespace AzToolsFramework
