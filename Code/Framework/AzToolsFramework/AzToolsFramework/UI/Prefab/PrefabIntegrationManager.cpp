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
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
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
#include <AzToolsFramework/ToolsComponents/EditorLayerComponentBus.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabViewportFocusPathHandler.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Viewport/ActionBus.h>

#include <QApplication>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

static constexpr AZStd::string_view LevelLoadedUpdaterIdentifier = "o3de.updater.onLevelLoaded";
static constexpr AZStd::string_view PrefabFocusChangedUpdaterIdentifier = "o3de.updater.onPrefabFocusChanged";

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

            if (IsNewActionManagerEnabled())
            {
                m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
                AZ_Assert(
                    m_actionManagerInterface, "Prefab - could not get m_actionManagerInterface on PrefabIntegrationManager construction.");

                m_toolBarManagerInterface = AZ::Interface<ToolBarManagerInterface>::Get();
                AZ_Assert(
                    m_toolBarManagerInterface, "Prefab - could not get m_toolBarManagerInterface on PrefabIntegrationManager construction.");

                // Register an updater that will refresh actions when a level is loaded.
                if (m_actionManagerInterface)
                {
                    ActionManagerRegistrationNotificationBus::Handler::BusConnect();
                }
            }
            
            EditorContextMenuBus::Handler::BusConnect();
            EditorEventsBus::Handler::BusConnect();
            PrefabFocusNotificationBus::Handler::BusConnect(s_editorEntityContextId);
            PrefabInstanceContainerNotificationBus::Handler::BusConnect();
            AZ::Interface<PrefabIntegrationInterface>::Register(this);
            EditorEntityContextNotificationBus::Handler::BusConnect();
            PrefabPublicNotificationBus::Handler::BusConnect();

            InitializeShortcuts();
        }

        PrefabIntegrationManager::~PrefabIntegrationManager()
        {
            UninitializeShortcuts();

            if (m_actionManagerInterface)
            {
                ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
            }

            PrefabPublicNotificationBus::Handler::BusDisconnect();
            EditorEntityContextNotificationBus::Handler::BusDisconnect();
            AZ::Interface<PrefabIntegrationInterface>::Unregister(this);
            PrefabInstanceContainerNotificationBus::Handler::BusDisconnect();
            PrefabFocusNotificationBus::Handler::BusDisconnect();
            EditorEventsBus::Handler::BusDisconnect();
            EditorContextMenuBus::Handler::BusDisconnect();
        }

        void PrefabIntegrationManager::Reflect(AZ::ReflectContext* context)
        {
            PrefabUserSettings::Reflect(context);
        }

        void PrefabIntegrationManager::InitializeShortcuts()
        {
            // Open/Edit Prefab (+)
            // We also support = to enable easier editing on compact US keyboards.
            {
                m_actions.emplace_back(AZStd::make_unique<QAction>(nullptr));

                m_actions.back()->setShortcuts({ QKeySequence(Qt::Key_Plus), QKeySequence(Qt::Key_Equal) });
                m_actions.back()->setText("Open/Edit Prefab");
                m_actions.back()->setStatusTip("Edit the prefab in focus mode.");

                QObject::connect(
                    m_actions.back().get(), &QAction::triggered, m_actions.back().get(),
                    [this]
                    {
                        AzToolsFramework::EntityIdList selectedEntities;
                        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

                        if (selectedEntities.size() != 1)
                        {
                            return;
                        }

                        AZ::EntityId selectedEntity = selectedEntities[0];

                        if (!s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntity))
                        {
                            return;
                        }

                        if (!s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntity))
                        {
                            ContextMenu_EditPrefab(selectedEntity);
                        }
                    });

                EditorActionRequestBus::Broadcast(
                    &EditorActionRequests::AddActionViaBusCrc, AZ_CRC_CE("org.o3de.action.editortransform.prefabopen"),
                    m_actions.back().get());
            }

            // Close Prefab (-)
            {
                m_actions.emplace_back(AZStd::make_unique<QAction>(nullptr));

                m_actions.back()->setShortcuts({ QKeySequence(Qt::Key_Minus) });
                m_actions.back()->setText("Close Prefab");
                m_actions.back()->setStatusTip("Close focus mode for this prefab and move one level up.");

                QObject::connect(
                    m_actions.back().get(), &QAction::triggered, m_actions.back().get(),
                    [this]
                    {
                        ContextMenu_ClosePrefab();
                    });

                EditorActionRequestBus::Broadcast(
                    &EditorActionRequests::AddActionViaBusCrc, AZ_CRC_CE("org.o3de.action.editortransform.prefabclose"),
                    m_actions.back().get());
            }
        }

        void PrefabIntegrationManager::UninitializeShortcuts()
        {
            m_actions.clear();
        }

        void PrefabIntegrationManager::OnActionUpdaterRegistrationHook()
        {
            // Update actions whenever a new root prefab is loaded.
            m_actionManagerInterface->RegisterActionUpdater(LevelLoadedUpdaterIdentifier);

            // Update actions whenever Prefab Focus changes (or is refreshed).
            m_actionManagerInterface->RegisterActionUpdater(PrefabFocusChangedUpdaterIdentifier);
        }

        void PrefabIntegrationManager::OnActionRegistrationHook()
        {
            {
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Focus up one level";
                actionProperties.m_description = "Move the Prefab Focus up one level.";
                actionProperties.m_category = "Prefabs";
                actionProperties.m_iconPath = ":/Breadcrumb/img/UI20/Breadcrumb/arrow_left-default.svg";

                m_actionManagerInterface->RegisterAction(
                    "o3de.context.editor.mainwindow",
                    "o3de.action.prefabs.upOneLevel",
                    actionProperties,
                    [prefabFocusPublicInterface = s_prefabFocusPublicInterface, editorEntityContextId = s_editorEntityContextId]()
                    {
                        prefabFocusPublicInterface->FocusOnParentOfFocusedPrefab(editorEntityContextId);
                    }
                );

                m_actionManagerInterface->InstallEnabledStateCallback(
                    "o3de.action.prefabs.upOneLevel",
                    [prefabFocusPublicInterface = s_prefabFocusPublicInterface, editorEntityContextId = s_editorEntityContextId]() -> bool
                    {
                        return prefabFocusPublicInterface->GetPrefabFocusPathLength(editorEntityContextId) > 1;
                    }
                );

                m_actionManagerInterface->AddActionToUpdater(PrefabFocusChangedUpdaterIdentifier, "o3de.action.prefabs.upOneLevel");
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

        void PrefabIntegrationManager::OnToolBarBindingHook()
        {
            // Populate Viewport top menu with Prefab actions and widgets
            m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.viewport.top", "o3de.action.prefabs.upOneLevel", 100);
            m_toolBarManagerInterface->AddWidgetToToolBar("o3de.toolbar.viewport.top", "o3de.widgetAction.prefab.focusPath", 200);
        }

        int PrefabIntegrationManager::GetMenuPosition() const
        {
            return aznumeric_cast<int>(EditorContextMenuOrdering::MIDDLE);
        }

        AZStd::string PrefabIntegrationManager::GetMenuIdentifier() const
        {
            return "Prefabs";
        }

        void PrefabIntegrationManager::PopulateEditorGlobalContextMenu(
            QMenu* menu, [[maybe_unused]] const AZStd::optional<AzFramework::ScreenPoint>& point, [[maybe_unused]] int flags)
        {
            AzToolsFramework::EntityIdList selectedEntities;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

            bool prefabWipFeaturesEnabled = false;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(
                prefabWipFeaturesEnabled, &AzFramework::ApplicationRequests::ArePrefabWipFeaturesEnabled);

            bool readOnlyEntityInSelection = false;
            for (const auto& entityId : selectedEntities)
            {
                if (m_readOnlyEntityPublicInterface->IsReadOnly(entityId))
                {
                    readOnlyEntityInSelection = true;
                    break;
                }
            }

            bool onlySelectedEntityIsFocusedPrefabContainer = false;
            bool onlySelectedEntityIsClosedPrefabContainer = false;

            if (selectedEntities.size() == 1)
            {
                AZ::EntityId selectedEntity = selectedEntities.front();

                if (s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntity))
                {
                    if (s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntity))
                    {
                        onlySelectedEntityIsFocusedPrefabContainer = true;
                    }
                    else
                    {
                        onlySelectedEntityIsClosedPrefabContainer = true;
                    }
                }
            }

            // Edit/Inspect/Close Prefab
            {
                if (selectedEntities.size() == 1)
                {
                    AZ::EntityId selectedEntity = selectedEntities[0];

                    if (s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntity))
                    {
                        if (!s_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(selectedEntity))
                        {
                            if (s_prefabPublicInterface->IsOwnedByProceduralPrefabInstance(selectedEntity))
                            {
                                // Inspect Prefab
                                QAction* editAction = menu->addAction(QObject::tr("Inspect Procedural Prefab"));
                                editAction->setShortcut(QKeySequence(Qt::Key_Plus));
                                editAction->setToolTip(QObject::tr("See the procedural prefab contents in focus mode."));

                                QObject::connect(
                                    editAction, &QAction::triggered, editAction,
                                    [this, selectedEntity]
                                    {
                                        ContextMenu_EditPrefab(selectedEntity);
                                    }
                                );
                            }
                            else
                            {
                                // Edit Prefab
                                QAction* editAction = menu->addAction(QObject::tr("Open/Edit Prefab"));
                                editAction->setShortcut(QKeySequence(Qt::Key_Plus));
                                editAction->setToolTip(QObject::tr("Edit the prefab in focus mode."));

                                QObject::connect(
                                    editAction, &QAction::triggered, editAction,
                                    [this, selectedEntity]
                                    {
                                        ContextMenu_EditPrefab(selectedEntity);
                                    }
                                );
                            }

                            if (IsPrefabOverridesUxEnabled())
                            {
                                if (!s_containerEntityInterface->IsContainerOpen(selectedEntity))
                                {
                                    // Open Prefab Instance
                                    QAction* overrideAction = menu->addAction(QObject::tr("Override Prefab Instance"));
                                    overrideAction->setToolTip(QObject::tr("Open the prefab instance to apply overrides."));

                                    QObject::connect(
                                        overrideAction, &QAction::triggered, overrideAction,
                                        [this, selectedEntity]
                                    {
                                        ContextMenu_OpenPrefabInstance(selectedEntity);
                                    }
                                    );
                                }
                                else
                                {
                                    // Close Prefab
                                    QAction* closeAction = menu->addAction(QObject::tr("Close Prefab Instance"));
                                    closeAction->setToolTip(QObject::tr("Close this prefab instance."));

                                    QObject::connect(
                                        closeAction, &QAction::triggered, closeAction,
                                        [this, selectedEntity]
                                    {
                                        ContextMenu_ClosePrefabInstance(selectedEntity);
                                    }
                                    );
                                }
                            }                           
                        }
                        else if (selectedEntity != s_prefabPublicInterface->GetLevelInstanceContainerEntityId())
                        {
                            // Close Prefab
                            QAction* closeAction = menu->addAction(QObject::tr("Close Prefab"));
                            closeAction->setShortcut(QKeySequence(Qt::Key_Minus));
                            closeAction->setToolTip(QObject::tr("Close focus mode for this prefab and move one level up."));

                            QObject::connect(
                                closeAction, &QAction::triggered, closeAction,
                                [this]
                                {
                                    ContextMenu_ClosePrefab();
                                }
                            );
                        }

                        menu->addSeparator();
                    }
                }
            }

            bool itemWasShown = false;

            // Create Prefab
            {
                if (!selectedEntities.empty())
                {
                    // Hide if the only selected entity is the Focused Instance Container
                    if (!onlySelectedEntityIsFocusedPrefabContainer)
                    {
                        bool layerInSelection = false;

                        for (AZ::EntityId entityId : selectedEntities)
                        {
                            if (!layerInSelection)
                            {
                                AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                                    layerInSelection, entityId,
                                    &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

                                if (layerInSelection)
                                {
                                    break;
                                }
                            }
                        }

                        // Layers can't be in prefabs.
                        // Also don't allow to create a prefab if any of the selected entities are read-only
                        if (!layerInSelection && !readOnlyEntityInSelection)
                        {
                            QAction* createAction = menu->addAction(QObject::tr("Create Prefab..."));
                            createAction->setToolTip(QObject::tr("Creates a prefab out of the currently selected entities."));

                            QObject::connect(
                                createAction, &QAction::triggered, createAction,
                                [this, selectedEntities]
                                {
                                    ContextMenu_CreatePrefab(selectedEntities);
                                }
                            );

                            itemWasShown = true;
                        }
                    }
                }
            }

            // Detach Prefab
            if (onlySelectedEntityIsClosedPrefabContainer)
            {
                AZ::EntityId selectedEntityId = selectedEntities.front();

                QAction* detachPrefabAction = menu->addAction(QObject::tr("Detach Prefab..."));
                QObject::connect(
                    detachPrefabAction, &QAction::triggered, detachPrefabAction,
                    [this, selectedEntityId]
                    {
                        ContextMenu_DetachPrefab(selectedEntityId);
                    }
                );
            }

            // Instantiate Prefab
            if (selectedEntities.size() == 0 ||
                selectedEntities.size() == 1 && !readOnlyEntityInSelection && !onlySelectedEntityIsClosedPrefabContainer)
            {
                QAction* instantiateAction = menu->addAction(QObject::tr("Instantiate Prefab..."));
                instantiateAction->setToolTip(QObject::tr("Instantiates a prefab file in the scene."));

                QObject::connect(
                    instantiateAction, &QAction::triggered, instantiateAction,
                    [this]
                    {
                        ContextMenu_InstantiatePrefab();
                    }
                );

                // Instantiate Procedural Prefab
                if (AZ::Prefab::ProceduralPrefabAsset::UseProceduralPrefabs())
                {
                    QAction* action = menu->addAction(QObject::tr("Instantiate Procedural Prefab..."));
                    action->setToolTip(QObject::tr("Instantiates a procedural prefab file in a prefab."));

                    QObject::connect(
                        action, &QAction::triggered, action,
                        [this]
                        {
                            ContextMenu_InstantiateProceduralPrefab();
                        }
                    );
                }

                itemWasShown = true;
            }

            if (itemWasShown)
            {
                menu->addSeparator();
            }

            // Save Prefab
            {
                if (selectedEntities.size() == 1)
                {
                    AZ::EntityId selectedEntity = selectedEntities[0];

                    if (s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntity))
                    {
                        // Save Prefab
                        AZ::IO::Path prefabFilePath = s_prefabPublicInterface->GetOwningInstancePrefabPath(selectedEntity);
                        auto dirtyOutcome = s_prefabPublicInterface->HasUnsavedChanges(prefabFilePath);

                        if (dirtyOutcome.IsSuccess() && dirtyOutcome.GetValue() == true)
                        {
                            QAction* saveAction = menu->addAction(QObject::tr("Save Prefab to file"));
                            saveAction->setToolTip(QObject::tr("Save the changes to the prefab to disk."));

                            QObject::connect(
                                saveAction, &QAction::triggered, saveAction,
                                [this, selectedEntity]
                                {
                                    ContextMenu_SavePrefab(selectedEntity);
                                }
                            );

                            menu->addSeparator();
                        }
                    }
                }
            }

            if (!selectedEntities.empty())
            {
                // Don't allow duplication if any of the selected entities are direct descendants of a read-only entity
                bool selectionContainsDescendantOfReadOnlyEntity = false;
                for (const auto& entityId : selectedEntities)
                {
                    AZ::EntityId parentEntityId;
                    AZ::TransformBus::EventResult(parentEntityId, entityId, &AZ::TransformBus::Events::GetParentId);

                    if (parentEntityId.IsValid() && m_readOnlyEntityPublicInterface->IsReadOnly(parentEntityId))
                    {
                        selectionContainsDescendantOfReadOnlyEntity = true;
                        break;
                    }
                }

                if (!selectionContainsDescendantOfReadOnlyEntity)
                {
                    QAction* duplicateAction = menu->addAction(QObject::tr("Duplicate"));
                    duplicateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
                    QObject::connect(
                        duplicateAction, &QAction::triggered, duplicateAction,
                        [this]
                        {
                            ContextMenu_Duplicate();
                        }
                    );
                }
            }

            if (!selectedEntities.empty() &&
                (selectedEntities.size() != 1 ||
                 selectedEntities[0] != s_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(s_editorEntityContextId)) &&
                !readOnlyEntityInSelection)
            {
                QAction* deleteAction = menu->addAction(QObject::tr("Delete"));
                deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));
                QObject::connect(
                    deleteAction, &QAction::triggered, deleteAction,
                    [this]
                    {
                        ContextMenu_DeleteSelected();
                    }
                );
            }

            // Revert Overrides
            {
                if (IsPrefabOverridesUxEnabled() && selectedEntities.size() == 1)
                {
                    AZ::EntityId selectedEntity = selectedEntities[0];
                    if (!s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntity) &&
                        m_prefabOverridePublicInterface->AreOverridesPresent(selectedEntity))
                    {
                        QAction* revertAction = menu->addAction(QObject::tr("Revert Overrides"));
                        QObject::connect(
                            revertAction,
                            &QAction::triggered,
                            revertAction,
                            [this, selectedEntity]
                            {
                                ContextMenu_RevertOverrides(selectedEntity);
                            });

                        menu->addSeparator();
                    }
                }
            }

            menu->addSeparator();
        }

        void PrefabIntegrationManager::OnEscape()
        {
            s_prefabFocusPublicInterface->FocusOnOwningPrefab(AZ::EntityId());
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
                        suggestedName, targetDirectory, AZ_CRC("PrefabUserSettings"), activeWindow, prefabName, prefabFilePath))
                {
                    // User canceled prefab creation, or error prevented continuation.
                    return;
                }
            }

            auto createPrefabOutcome = s_prefabPublicInterface->CreatePrefabInDisk(selectedEntities, prefabFilePath.data());

            if (!createPrefabOutcome.IsSuccess())
            {
                WarningDialog("Prefab Creation Error", createPrefabOutcome.GetError());
            }
        }

        void PrefabIntegrationManager::ContextMenu_InstantiatePrefab()
        {
            AZStd::string prefabFilePath;
            bool hasUserSelectedValidSourceFile = PrefabSaveHandler::QueryUserForPrefabFilePath(prefabFilePath);

            if (hasUserSelectedValidSourceFile)
            {
                AZ::EntityId parentId;
                AZ::Vector3 position = AZ::Vector3::CreateZero();

                EntityIdList selectedEntities;
                ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);
                // if one entity is selected, instantiate prefab as its child and place it at same position as parent
                if (selectedEntities.size() == 1)
                {
                    parentId = selectedEntities.front();
                    AZ::TransformBus::EventResult(position, parentId, &AZ::TransformInterface::GetWorldTranslation);
                }
                // otherwise instantiate it at root level and center of viewport
                else
                {
                    EditorRequestBus::BroadcastResult(position, &EditorRequestBus::Events::GetWorldPositionAtViewportCenter);
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
            PrefabOperationResult detachPrefabResult = s_prefabPublicInterface->DetachPrefab(containerEntity);

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
                m_actionManagerInterface->TriggerActionUpdater(LevelLoadedUpdaterIdentifier);
            }
        }

        void PrefabIntegrationManager::OnPrefabFocusChanged(
            [[maybe_unused]] AZ::EntityId previousContainerEntityId, [[maybe_unused]] AZ::EntityId newContainerEntityId)
        {
            if (m_actionManagerInterface)
            {
                m_actionManagerInterface->TriggerActionUpdater(PrefabFocusChangedUpdaterIdentifier);
            }
        }

        void PrefabIntegrationManager::OnPrefabFocusRefreshed()
        {
            if (m_actionManagerInterface)
            {
                m_actionManagerInterface->TriggerActionUpdater(PrefabFocusChangedUpdaterIdentifier);
            }
        }

    } // namespace Prefab

} // namespace AzToolsFramework
