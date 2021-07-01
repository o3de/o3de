/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabIntegrationManager.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/ToolsComponents/EditorLayerComponentBus.h>
#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationInterface.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>

namespace AzToolsFramework
{
    namespace Prefab
    {
        
        EditorEntityUiInterface* PrefabIntegrationManager::s_editorEntityUiInterface = nullptr;
        PrefabPublicInterface* PrefabIntegrationManager::s_prefabPublicInterface = nullptr;
        PrefabEditInterface* PrefabIntegrationManager::s_prefabEditInterface = nullptr;
        PrefabLoaderInterface* PrefabIntegrationManager::s_prefabLoaderInterface = nullptr;

        const AZStd::string PrefabIntegrationManager::s_prefabFileExtension = ".prefab";

        void PrefabUserSettings::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<PrefabUserSettings>()
                    ->Version(1)
                    ->Field("m_saveLocation", &PrefabUserSettings::m_saveLocation)
                    ->Field("m_autoNumber", &PrefabUserSettings::m_autoNumber);
            }
        }

        PrefabIntegrationManager::PrefabIntegrationManager()
        {
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

            s_prefabEditInterface = AZ::Interface<PrefabEditInterface>::Get();
            if (s_prefabEditInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get PrefabEditInterface on PrefabIntegrationManager construction.");
                return;
            }

            s_prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
            if (s_prefabLoaderInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get PrefabLoaderInterface on PrefabIntegrationManager construction.");
                return;
            }

            EditorContextMenuBus::Handler::BusConnect();
            PrefabInstanceContainerNotificationBus::Handler::BusConnect();
            AZ::Interface<PrefabIntegrationInterface>::Register(this);
            AssetBrowser::AssetBrowserSourceDropBus::Handler::BusConnect(s_prefabFileExtension);
        }

        PrefabIntegrationManager::~PrefabIntegrationManager()
        {
            AssetBrowser::AssetBrowserSourceDropBus::Handler::BusDisconnect();
            AZ::Interface<PrefabIntegrationInterface>::Unregister(this);
            PrefabInstanceContainerNotificationBus::Handler::BusDisconnect();
            EditorContextMenuBus::Handler::BusDisconnect();
        }

        void PrefabIntegrationManager::Reflect(AZ::ReflectContext* context)
        {
            PrefabUserSettings::Reflect(context);
        }

        int PrefabIntegrationManager::GetMenuPosition() const
        {
            return aznumeric_cast<int>(EditorContextMenuOrdering::MIDDLE);
        }

        AZStd::string PrefabIntegrationManager::GetMenuIdentifier() const
        {
            return "Prefabs";
        }

        void PrefabIntegrationManager::PopulateEditorGlobalContextMenu(QMenu* menu, [[maybe_unused]] const AZ::Vector2& point, [[maybe_unused]] int flags)
        {
            AzToolsFramework::EntityIdList selectedEntities;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

            bool prefabWipFeaturesEnabled = false;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(
                prefabWipFeaturesEnabled, &AzFramework::ApplicationRequests::ArePrefabWipFeaturesEnabled);

            // Create Prefab
            {
                if (!selectedEntities.empty())
                {
                    // Hide if the only selected entity is the Level Container
                    if (selectedEntities.size() > 1 || !s_prefabPublicInterface->IsLevelInstanceContainerEntity(selectedEntities[0]))
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
                        if (!layerInSelection)
                        {
                            QAction* createAction = menu->addAction(QObject::tr("Create Prefab..."));
                            createAction->setToolTip(QObject::tr("Creates a prefab out of the currently selected entities."));

                            QObject::connect(createAction, &QAction::triggered, createAction, [this, selectedEntities] {
                                ContextMenu_CreatePrefab(selectedEntities);
                            });
                        }
                    }
                }
            }

            // Instantiate Prefab
            {
                QAction* instantiateAction = menu->addAction(QObject::tr("Instantiate Prefab..."));
                instantiateAction->setToolTip(QObject::tr("Instantiates a prefab file in the scene."));

                QObject::connect(
                    instantiateAction, &QAction::triggered, instantiateAction, [this] { ContextMenu_InstantiatePrefab(); });
            }

            menu->addSeparator();

            bool itemWasShown = false;

            // Edit/Save Prefab
            {
                if (selectedEntities.size() == 1)
                {
                    AZ::EntityId selectedEntity = selectedEntities[0];

                    if (s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntity))
                    {
                        // Edit Prefab
                        if (prefabWipFeaturesEnabled)
                        {
                            bool beingEdited = s_prefabEditInterface->IsOwningPrefabBeingEdited(selectedEntity);

                            if (!beingEdited)
                            {
                                QAction* editAction = menu->addAction(QObject::tr("Edit Prefab"));
                                editAction->setToolTip(QObject::tr("Edit the prefab in focus mode."));

                                QObject::connect(editAction, &QAction::triggered, editAction, [this, selectedEntity] {
                                    ContextMenu_EditPrefab(selectedEntity);
                                });

                                itemWasShown = true;
                            }
                        }

                        // Save Prefab
                        AZ::IO::Path prefabFilePath = s_prefabPublicInterface->GetOwningInstancePrefabPath(selectedEntity);
                        auto dirtyOutcome = s_prefabPublicInterface->HasUnsavedChanges(prefabFilePath);

                        if (dirtyOutcome.IsSuccess() && dirtyOutcome.GetValue() == true)
                        {
                            QAction* saveAction = menu->addAction(QObject::tr("Save Prefab to file"));
                            saveAction->setToolTip(QObject::tr("Save the changes to the prefab to disk."));

                            QObject::connect(saveAction, &QAction::triggered, saveAction, [this, selectedEntity] {
                                ContextMenu_SavePrefab(selectedEntity);
                            });

                            itemWasShown = true;
                        }
                    }
                }
            }

            if (itemWasShown)
            {
                menu->addSeparator();
            }

            QAction* deleteAction = menu->addAction(QObject::tr("Delete"));
            QObject::connect(deleteAction, &QAction::triggered, deleteAction, [this] { ContextMenu_DeleteSelected(); });
            if (selectedEntities.size() == 0 ||
                (selectedEntities.size() == 1 && s_prefabPublicInterface->IsLevelInstanceContainerEntity(selectedEntities[0])))
            {
                deleteAction->setDisabled(true);
            }

            // Detach Prefab
            if (selectedEntities.size() == 1)
            {
                AZ::EntityId selectedEntity = selectedEntities[0];

                if (s_prefabPublicInterface->IsInstanceContainerEntity(selectedEntity) &&
                    !s_prefabPublicInterface->IsLevelInstanceContainerEntity(selectedEntity))
                {
                    QAction* detachPrefabAction = menu->addAction(QObject::tr("Detach Prefab..."));
                    QObject::connect(
                        detachPrefabAction, &QAction::triggered, detachPrefabAction,
                        [this, selectedEntity]
                        {
                            ContextMenu_DetachPrefab(selectedEntity);
                        });
                }
            }
        }

        void PrefabIntegrationManager::HandleSourceFileType(AZStd::string_view sourceFilePath, AZ::EntityId parentId, AZ::Vector3 position) const
        {
            auto createPrefabOutcome = s_prefabPublicInterface->InstantiatePrefab(sourceFilePath, parentId, position);

            if (!createPrefabOutcome.IsSuccess())
            {
                WarnUserOfError("Prefab Instantiation Error", createPrefabOutcome.GetError());
            }
        }

        void PrefabIntegrationManager::ContextMenu_CreatePrefab(AzToolsFramework::EntityIdList selectedEntities)
        {
            // Save a reference to our currently active window since it will be
            // temporarily null after QFileDialogs close, which we need in order to
            // be able to parent our message dialogs properly
            QWidget* activeWindow = QApplication::activeWindow();
            const AZStd::string prefabFilesPath = "@devassets@/Prefabs";

            // Remove Level entity if it's part of the list
            
            auto levelContainerIter =
                AZStd::find(selectedEntities.begin(), selectedEntities.end(), s_prefabPublicInterface->GetLevelInstanceContainerEntityId());
            if (levelContainerIter != selectedEntities.end())
            {
                selectedEntities.erase(levelContainerIter);
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
                    bool continueCreation = QueryAndPruneMissingExternalReferences(entitiesToIncludeInAsset, allReferencedEntities, useAllReferencedEntities);
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
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(hasCommonRoot,
                        &AzToolsFramework::ToolsApplicationRequests::FindCommonRoot, entitiesToIncludeInAsset, commonRoot, &prefabRootEntities);
                    if (hasCommonRoot && commonRoot.IsValid() && entitiesToIncludeInAsset.find(commonRoot) != entitiesToIncludeInAsset.end())
                    {
                        prefabRootEntities.insert(prefabRootEntities.begin(), commonRoot);
                    }
                }

                GenerateSuggestedFilenameFromEntities(prefabRootEntities, suggestedName);

                if (!QueryUserForPrefabSaveLocation(
                        suggestedName, targetDirectory, AZ_CRC("PrefabUserSettings"), activeWindow, prefabName, prefabFilePath))
                {
                    // User canceled prefab creation, or error prevented continuation.
                    return;
                }
            }

            auto createPrefabOutcome = s_prefabPublicInterface->CreatePrefab(selectedEntities, prefabFilePath.data());

            if (!createPrefabOutcome.IsSuccess())
            {
                WarnUserOfError("Prefab Creation Error", createPrefabOutcome.GetError());
            }
        }

        void PrefabIntegrationManager::ContextMenu_InstantiatePrefab()
        {
            AZStd::string prefabFilePath;
            bool hasUserSelectedValidSourceFile = QueryUserForPrefabFilePath(prefabFilePath);

            if (hasUserSelectedValidSourceFile)
            {
                // Get position (center of viewport). If no viewport is available, (0,0,0) will be used.
                AZ::Vector3 viewportCenterPosition = AZ::Vector3::CreateZero();
                EditorRequestBus::BroadcastResult(viewportCenterPosition, &EditorRequestBus::Events::GetWorldPositionAtViewportCenter);

                // Instantiating from context menu always puts the instance at the root level
                auto createPrefabOutcome = s_prefabPublicInterface->InstantiatePrefab(prefabFilePath, AZ::EntityId(), viewportCenterPosition);

                if (!createPrefabOutcome.IsSuccess())
                {
                    WarnUserOfError("Prefab Instantiation Error",createPrefabOutcome.GetError());
                }
            }
        }

        void PrefabIntegrationManager::ContextMenu_EditPrefab(AZ::EntityId containerEntity)
        {
            s_prefabEditInterface->EditOwningPrefab(containerEntity);
        }

        void PrefabIntegrationManager::ContextMenu_SavePrefab(AZ::EntityId containerEntity)
        {
            auto prefabPath = s_prefabPublicInterface->GetOwningInstancePrefabPath(containerEntity);

            auto savePrefabOutcome = s_prefabPublicInterface->SavePrefab(prefabPath);

            if (!savePrefabOutcome.IsSuccess())
            {
                WarnUserOfError("Prefab Save Error", savePrefabOutcome.GetError());
            }
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
                WarnUserOfError("Delete selected entities error", deleteSelectedResult.GetError());
            }
        }

        void PrefabIntegrationManager::ContextMenu_DetachPrefab(AZ::EntityId containerEntity)
        {
            PrefabOperationResult detachPrefabResult =
                s_prefabPublicInterface->DetachPrefab(containerEntity);

            if (!detachPrefabResult.IsSuccess())
            {
                WarnUserOfError("Detach Prefab error", detachPrefabResult.GetError());
            }
        }

        void PrefabIntegrationManager::GenerateSuggestedFilenameFromEntities(const EntityIdList& entityIds, AZStd::string& outName)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            AZStd::string suggestedName;

            for (const AZ::EntityId& entityId : entityIds)
            {
                if (!AppendEntityToSuggestedFilename(suggestedName, entityId))
                {
                    break;
                }
            }

            if (suggestedName.size() == 0 || AzFramework::StringFunc::Utf8::CheckNonAsciiChar(suggestedName))
            {
                suggestedName = "NewPrefab";
            }

            outName = suggestedName;
        }

        bool PrefabIntegrationManager::AppendEntityToSuggestedFilename(AZStd::string& filename, AZ::EntityId entityId)
        {
            // When naming a prefab after its entities, we stop appending additional names once we've reached this cutoff length
            size_t prefabNameCutoffLength = 32;
            AzToolsFramework::EntityIdSet usedNameEntities;

            if (usedNameEntities.find(entityId) == usedNameEntities.end())
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
                if (entity)
                {
                    AZStd::string entityNameFiltered = entity->GetName();

                    // Convert spaces in entity names to underscores
                    for (size_t i = 0; i < entityNameFiltered.size(); ++i)
                    {
                        char& character = entityNameFiltered.at(i);
                        if (character == ' ')
                        {
                            character = '_';
                        }
                    }

                    filename.append(entityNameFiltered);
                    usedNameEntities.insert(entityId);
                    if (filename.size() > prefabNameCutoffLength)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        bool PrefabIntegrationManager::QueryUserForPrefabSaveLocation(
            const AZStd::string& suggestedName,
            const char* initialTargetDirectory,
            AZ::u32 prefabUserSettingsId,
            QWidget* activeWindow,
            AZStd::string& outPrefabName,
            AZStd::string& outPrefabFilePath
        )
        {
            AZStd::string saveAsInitialSuggestedDirectory;
            if (!GetPrefabSaveLocation(saveAsInitialSuggestedDirectory, prefabUserSettingsId))
            {
                saveAsInitialSuggestedDirectory = initialTargetDirectory;
            }

            AZStd::string saveAsInitialSuggestedFullPath;
            GenerateSuggestedPrefabPath(suggestedName, saveAsInitialSuggestedDirectory, saveAsInitialSuggestedFullPath);

            QString saveAs;
            AZStd::string targetPath;
            QFileInfo prefabSaveFileInfo;
            QString prefabName;
            while (true)
            {
                {
                    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
                    saveAs = QFileDialog::getSaveFileName(nullptr, QString("Save As..."), saveAsInitialSuggestedFullPath.c_str(), QString("Prefabs (*.prefab)"));
                }

                prefabSaveFileInfo = saveAs;
                prefabName = prefabSaveFileInfo.baseName();
                if (saveAs.isEmpty())
                {
                    return false;
                }

                targetPath = saveAs.toUtf8().constData();
                if (AzFramework::StringFunc::Utf8::CheckNonAsciiChar(targetPath))
                {
                    WarnUserOfError(
                        "Prefab Creation Failed.",
                        "Unicode file name is not supported. \r\n"
                        "Please use ASCII characters to name your prefab."
                    );
                    return false;
                }

                PrefabSaveResult saveResult = IsPrefabPathValidForAssets(activeWindow, saveAs, saveAsInitialSuggestedFullPath);
                if (saveResult == PrefabSaveResult::Cancel)
                {
                    // The error was already reported if this failed.
                    return false;
                }
                else if (saveResult == PrefabSaveResult::Continue)
                {
                    // The prefab save name is valid, continue with the save attempt.
                    break;
                }
            }

            // If the prefab already exists, notify the user and bail
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO && fileIO->Exists(targetPath.c_str()))
            {
                const AZStd::string message = AZStd::string::format(
                    "You are attempting to overwrite an existing prefab: \"%s\".\r\n\r\n"
                    "This will damage instances or cascades of this prefab. \r\n\r\n"
                    "Instead, either push entities/fields to the prefab, or save to a different location.",
                    targetPath.c_str());

                WarnUserOfError("Prefab Already Exists", message);
                return false;
            }

            // We prevent users from creating a new prefab with the same relative path that's already
            // been used by an existing prefab in other places (e.g. Gems) because the AssetProcessor
            // generates asset ids based on relative paths. This is unnecessary once AssetProcessor
            // starts to generate UUID to every asset regardless of paths.
            {
                AZStd::string prefabRelativeName;
                bool relativePathFound;
                AssetSystemRequestBus::BroadcastResult(relativePathFound, &AssetSystemRequestBus::Events::GetRelativeProductPathFromFullSourceOrProductPath, targetPath, prefabRelativeName);

                AZ::Data::AssetId prefabAssetId;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(prefabAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, prefabRelativeName.c_str(), AZ::Data::s_invalidAssetType, false);
                if (prefabAssetId.IsValid())
                {
                    const AZStd::string message = AZStd::string::format(
                        "A prefab with the relative path \"%s\" already exists in the Asset Database. \r\n\r\n"
                        "Overriding it will damage instances or cascades of this prefab. \r\n\r\n"
                        "Instead, either push entities/fields to the prefab, or save to a different location.",
                        prefabRelativeName.c_str());

                    WarnUserOfError("Prefab Path Error", message);
                    return false;
                }
            }

            AZStd::string saveDir(prefabSaveFileInfo.absoluteDir().absolutePath().toUtf8().constData());
            SetPrefabSaveLocation(saveDir, prefabUserSettingsId);

            outPrefabName = prefabName.toUtf8().constData();
            outPrefabFilePath = targetPath.c_str();

            return true;
        }

        bool PrefabIntegrationManager::QueryUserForPrefabFilePath(AZStd::string& outPrefabFilePath)
        {
            AssetSelectionModel selection;

            // Note, stringfilter will match every source file CONTAINING ".prefab".
            // If this causes issues, we will need to create a new filter class for regex matching.
            // We'll need to check if the file contents are actually a prefab later in the flow anyways,
            // so this should not be an issue.
            StringFilter* stringFilter = new StringFilter();
            stringFilter->SetName("Prefab");
            stringFilter->SetFilterString(".prefab");
            stringFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
            auto stringFilterPtr = FilterConstType(stringFilter);

            EntryTypeFilter* sourceFilter = new EntryTypeFilter();
            sourceFilter->SetName("Source");
            sourceFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Source);
            sourceFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
            auto sourceFilterPtr = FilterConstType(sourceFilter);

            CompositeFilter* compositeFilter = new CompositeFilter(CompositeFilter::LogicOperatorType::AND);
            compositeFilter->SetName("Prefab");
            compositeFilter->AddFilter(sourceFilterPtr);
            compositeFilter->AddFilter(stringFilterPtr);
            auto compositeFilterPtr = FilterConstType(compositeFilter);

            selection.SetDisplayFilter(compositeFilterPtr);
            selection.SetSelectionFilter(compositeFilterPtr);

            AssetBrowserComponentRequestBus::Broadcast(&AssetBrowserComponentRequests::PickAssets, selection, AzToolsFramework::GetActiveWindow());

            if (!selection.IsValid())
            {
                // User closed the dialog without selecting, just return.
                return false;
            }

            auto source = azrtti_cast<const SourceAssetBrowserEntry*>(selection.GetResult());

            if (source == nullptr)
            {
                AZ_Assert(false, "Prefab - Incorrect entry type selected during prefab instantiation. Expected source.");
                return false;
            }

            outPrefabFilePath = source->GetFullPath();
            return true;
        }

        void PrefabIntegrationManager::WarnUserOfError(AZStd::string_view title, AZStd::string_view message)
        {
            QWidget* activeWindow = QApplication::activeWindow();

            QMessageBox::warning(
                activeWindow,
                QString(title.data()),
                QString(message.data()),
                QMessageBox::Ok,
                QMessageBox::Ok
            );
        }

        PrefabIntegrationManager::PrefabSaveResult PrefabIntegrationManager::IsPrefabPathValidForAssets(QWidget* activeWindow,
            QString prefabPath, AZStd::string& retrySavePath)
        {
            bool assetSetFoldersRetrieved = false;
            AZStd::vector<AZStd::string> assetSafeFolders;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                assetSetFoldersRetrieved,
                &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetSafeFolders,
                assetSafeFolders);

            if (!assetSetFoldersRetrieved)
            {
                // If the asset safe list couldn't be retrieved, don't block the user but warn them.
                AZ_Warning("Prefab", false, "Unable to verify that the prefab file to create is in a valid path.");
            }
            else
            {
                QString cleanSaveAs(QDir::cleanPath(prefabPath));

                bool isPathSafeForAssets = false;
                for (AZStd::string assetSafeFolder : assetSafeFolders)
                {
                    QString cleanAssetSafeFolder(QDir::cleanPath(assetSafeFolder.c_str()));
                    // Compare using clean paths so slash direction does not matter.
                    // Note that this comparison is case sensitive because some file systems
                    // Open 3D Engine supports are case sensitive.
                    if (cleanSaveAs.startsWith(cleanAssetSafeFolder))
                    {
                        isPathSafeForAssets = true;
                        break;
                    }
                }

                if (!isPathSafeForAssets)
                {
                    // Put an error in the console, so the log files have info about this error, or the user can look up the error after dismissing it.
                    AZStd::string errorMessage = "You can only save prefabs to either your game project folder or the Gems folder. Update the location and try again.\n\n"
                        "You can also review and update your save locations in the AssetProcessorPlatformConfig.ini file.";
                    AZ_Error("Prefab", false, errorMessage.c_str());

                    // Display a pop-up, the logs are easy to miss. This will make sure a user who encounters this error immediately knows their prefab save has failed.
                    QMessageBox msgBox(activeWindow);
                    msgBox.setIcon(QMessageBox::Icon::Warning);
                    msgBox.setTextFormat(Qt::RichText);
                    msgBox.setWindowTitle(QObject::tr("Invalid save location"));
                    msgBox.setText(QObject::tr(errorMessage.c_str()));
                    msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Retry);
                    msgBox.setDefaultButton(QMessageBox::Retry);
                    const int response = msgBox.exec();
                    switch (response)
                    {
                        case QMessageBox::Retry:
                            // If the user wants to retry, they probably want to save to a valid location,
                            // so set the suggested save path to a known valid location.
                            if (assetSafeFolders.size() > 0)
                            {
                                retrySavePath = assetSafeFolders[0];
                            }
                            return PrefabSaveResult::Retry;
                        case QMessageBox::Cancel:
                        default:
                            return PrefabSaveResult::Cancel;
                    }
                }
            }
            // Valid prefab save location, continue with the save attempt.
            return PrefabSaveResult::Continue;
        }

        void PrefabIntegrationManager::GenerateSuggestedPrefabPath(const AZStd::string& prefabName, const AZStd::string& targetDirectory, AZStd::string& suggestedFullPath)
        {
            // Generate full suggested path from prefabName - if given NewPrefab as prefabName,
            // NewPrefab_001.prefab would be tried, and if that already existed we would suggest
            // the first unused number value (NewPrefab_002.prefab etc.)
            AZStd::string normalizedTargetDirectory = targetDirectory;
            AZ::StringFunc::Path::Normalize(normalizedTargetDirectory);

            // Convert spaces in entity names to underscores
            AZStd::string prefabNameFiltered = prefabName;
            AZ::StringFunc::Replace(prefabNameFiltered, ' ', '_');

            auto settings = AZ::UserSettings::CreateFind<PrefabUserSettings>(AZ_CRC("PrefabUserSettings"), AZ::UserSettings::CT_LOCAL);
            if (settings->m_autoNumber)
            {
                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

                const AZ::u32 maxPrefabNumber = 1000;
                for (AZ::u32 prefabNumber = 1; prefabNumber < maxPrefabNumber; ++prefabNumber)
                {
                    AZStd::string possiblePath;
                    AZ::StringFunc::Path::Join(
                        normalizedTargetDirectory.c_str(),
                        AZStd::string::format("%s_%3.3u%s", prefabNameFiltered.c_str(), prefabNumber, s_prefabFileExtension.c_str()).c_str(),
                        possiblePath
                    );

                    if (!fileIO || !fileIO->Exists(possiblePath.c_str()))
                    {
                        suggestedFullPath = possiblePath;
                        break;
                    }
                }
            }
            else
            {
                // use the entity name as the file name regardless of it already existing, the OS will ask the user to overwrite the file in that case.
                AZ::StringFunc::Path::Join(
                    normalizedTargetDirectory.c_str(),
                    AZStd::string::format("%s%s", prefabNameFiltered.c_str(), s_prefabFileExtension.c_str()).c_str(),
                    suggestedFullPath
                );
            }
        }

        void PrefabIntegrationManager::SetPrefabSaveLocation(const AZStd::string& path, AZ::u32 settingsId)
        {
            auto settings = AZ::UserSettings::CreateFind<PrefabUserSettings>(settingsId, AZ::UserSettings::CT_LOCAL);
            settings->m_saveLocation = path;
        }

        bool PrefabIntegrationManager::GetPrefabSaveLocation(AZStd::string& path, AZ::u32 settingsId)
        {
            auto settings = AZ::UserSettings::Find<PrefabUserSettings>(settingsId, AZ::UserSettings::CT_LOCAL);
            if (settings)
            {
                path = settings->m_saveLocation;
                return true;
            }

            return false;
        }

        void PrefabIntegrationManager::GatherAllReferencedEntitiesAndCompare(const EntityIdSet& entities,
            EntityIdSet& entitiesAndReferencedEntities, bool& hasExternalReferences)
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

            const AZ::Edit::ElementData* classEditData = classData ? classData->FindElementData(AZ::Edit::ClassElements::EditorData) : nullptr;
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

        void PrefabIntegrationManager::GatherAllReferencedEntities(EntityIdSet& entitiesWithReferences, AZ::SerializeContext& serializeContext)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
                    auto beginCB = [&](void* ptr, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* elementData) -> bool
                    {
                        parentStack.push_back(classData);

                        AZ::u32 sliceFlags = GetSliceFlags(elementData ? elementData->m_editData : nullptr, classData ? classData->m_editData : nullptr);

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
                                AZ::EntityId* entityIdPtr = (elementData->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER) ?
                                    *reinterpret_cast<AZ::EntityId**>(ptr) : reinterpret_cast<AZ::EntityId*>(ptr);
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
                        beginCB,
                        endCB,
                        &serializeContext,
                        AZ::SerializeContext::ENUM_ACCESS_FOR_READ,
                        nullptr
                    );

                    serializeContext.EnumerateInstanceConst(
                        &callContext,
                        entity,
                        azrtti_typeid<AZ::Entity>(),
                        nullptr,
                        nullptr
                    );
                }
            }
        }

        bool PrefabIntegrationManager::QueryAndPruneMissingExternalReferences(EntityIdSet& entities, EntityIdSet& selectedAndReferencedEntities,
            bool& useReferencedEntities, bool defaultMoveExternalRefs)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
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
                    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

                    const AZStd::string message = AZStd::string::format(
                        "Entity references may not be valid if the entity IDs change or if the entities do not exist when the prefab is instantiated.\r\n\r\nSelected Entities\n%s\nReferenced Entities\n%s\n",
                        includedEntities.c_str(),
                        referencedEntities.c_str());

                    QMessageBox msgBox(AzToolsFramework::GetActiveWindow());
                    msgBox.setWindowTitle("External Entity References");
                    msgBox.setText("The prefab contains references to external entities that are not selected.");
                    msgBox.setInformativeText("You can move the referenced entities into this prefab or retain the external references.");
                    QAbstractButton* moveButton = (QAbstractButton*)msgBox.addButton("Move", QMessageBox::YesRole);
                    QAbstractButton* retainButton = (QAbstractButton*)msgBox.addButton("Retain", QMessageBox::NoRole);
                    msgBox.setStandardButtons(QMessageBox::Cancel);
                    msgBox.setDefaultButton(QMessageBox::Yes);
                    msgBox.setDetailedText(message.c_str());
                    const int response = msgBox.exec();

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
            if (s_prefabPublicInterface->IsLevelInstanceContainerEntity(entityId))
            {
                s_editorEntityUiInterface->RegisterEntity(entityId, m_levelRootUiHandler.GetHandlerId());
            }
            else
            {
                s_editorEntityUiInterface->RegisterEntity(entityId, m_prefabUiHandler.GetHandlerId());
            }
        }

        void PrefabIntegrationManager::OnPrefabComponentDeactivate(AZ::EntityId entityId)
        {
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
                WarnUserOfError("Entity Creation Error", createResult.GetError());
                return AZ::EntityId();
            }
        }
    }
}
