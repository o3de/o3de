/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabSaveLoadHandler.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/TemplateInstanceMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Procedural/ProceduralPrefabAsset.h>
#include <AzToolsFramework/UI/Notifications/ToastNotificationsView.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerDragAndDropContext.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <AzQtComponents/DragAndDrop/ViewportDragAndDrop.h>
#include <AzQtComponents/Components/FlowLayout.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDragLeaveEvent>


namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabLoaderInterface* PrefabSaveHandler::s_prefabLoaderInterface = nullptr;
        PrefabPublicInterface* PrefabSaveHandler::s_prefabPublicInterface = nullptr;
        PrefabSystemComponentInterface* PrefabSaveHandler::s_prefabSystemComponentInterface = nullptr;

        const AZStd::string PrefabSaveHandler::s_prefabFileExtension = ".prefab";
        const AZStd::string PrefabSaveHandler::s_procPrefabFileExtension = ".procprefab";

        static const char* const ClosePrefabDialog = "ClosePrefabDialog";
        static const char* const FooterSeparatorLine = "FooterSeparatorLine";
        static const char* const PrefabSavedMessageFrame = "PrefabSavedMessageFrame";
        static const char* const PrefabSavePreferenceHint = "PrefabSavePreferenceHint";
        static const char* const PrefabSaveWarningFrame = "PrefabSaveWarningFrame";
        static const char* const SaveDependentPrefabsCard = "SaveDependentPrefabsCard";
        static const char* const SavePrefabDialog = "SavePrefabDialog";
        static const char* const UnsavedPrefabFileName = "UnsavedPrefabFileName";

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

        PrefabSaveHandler::PrefabSaveHandler()
        {
            s_prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
            if (s_prefabLoaderInterface == nullptr)
            {
                AZ_Assert(false, "PrefabSaveHandler - could not get PrefabLoaderInterface on construction.");
                return;
            }

            s_prefabPublicInterface = AZ::Interface<PrefabPublicInterface>::Get();
            if (s_prefabPublicInterface == nullptr)
            {
                AZ_Assert(false, "PrefabSaveHandler - could not get PrefabPublicInterface on construction.");
                return;
            }
           
            s_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            if (s_prefabSystemComponentInterface == nullptr)
            {
                AZ_Assert(false, "PrefabSaveHandler - could not get PrefabSystemComponentInterface on construction.");
                return;
            }

            m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            if (m_instanceEntityMapperInterface == nullptr)
            {
                AZ_Assert(false, "PrefabSaveHandler - could not get InstanceEntityMapperInterface on construction.");
                return;
            }

            m_templateInstanceMapperInterface = AZ::Interface<TemplateInstanceMapperInterface>::Get();
            if (m_templateInstanceMapperInterface == nullptr)
            {
                AZ_Assert(false, "PrefabSaveHandler - could not get TemplateInstanceMapperInterface on construction.");
                return;
            }
            
            // we care about both outliner and viewport.
            AzQtComponents::DragAndDropEventsBus::Handler::BusConnect(AzQtComponents::DragAndDropContexts::EditorViewport);
            AzQtComponents::DragAndDropItemViewEventsBus::Handler::BusConnect(AzQtComponents::DragAndDropContexts::EntityOutliner);
            AzToolsFramework::AssetSystemBus::Handler::BusConnect();
        }

        PrefabSaveHandler::~PrefabSaveHandler()
        {
            AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();
            AzQtComponents::DragAndDropEventsBus::Handler::BusDisconnect();
            AzQtComponents::DragAndDropItemViewEventsBus::Handler::BusDisconnect();
        }

        bool PrefabSaveHandler::GetPrefabSaveLocation(AZStd::string& path, AZ::u32 settingsId)
        {
            auto settings = AZ::UserSettings::Find<PrefabUserSettings>(settingsId, AZ::UserSettings::CT_LOCAL);
            if (settings)
            {
                path = settings->m_saveLocation;
                return true;
            }

            return false;
        }

        void PrefabSaveHandler::SetPrefabSaveLocation(const AZStd::string& path, AZ::u32 settingsId)
        {
            auto settings = AZ::UserSettings::CreateFind<PrefabUserSettings>(settingsId, AZ::UserSettings::CT_LOCAL);
            settings->m_saveLocation = path;
        }

        // Drag and drop overrides:
        int PrefabSaveHandler::GetDragAndDropEventsPriority() const
        {
            return CommonDragAndDropBusTraits::s_HighPriority; // This class gets ahead of other built in ones
        }

        // viewport dragging:
        void PrefabSaveHandler::DragEnter(QDragEnterEvent* event, [[maybe_unused]] AzQtComponents::DragAndDropContextBase& context)
        {
            if (event->isAccepted())
            {
                return;
            }

            if (CanDragAndDropData(event->mimeData()))
            {
                event->accept();
                event->setDropAction(Qt::DropAction::CopyAction);
            }
        }

        void PrefabSaveHandler::DragMove(QDragMoveEvent* event, [[maybe_unused]] AzQtComponents::DragAndDropContextBase& context)
        {
            if (event->isAccepted())
            {
                return;
            }

            if (CanDragAndDropData(event->mimeData()))
            {
                event->accept();
                event->setDropAction(Qt::DropAction::CopyAction);
            }
        }

        void PrefabSaveHandler::DragLeave(QDragLeaveEvent* event)
        {
            if (event->isAccepted())
            {
                return;
            }
        }

        void PrefabSaveHandler::DoDragAndDropData(
            const AZ::Vector3& instantiateLocation,
            const AZ::EntityId& parentEntity,
            const AZStd::vector<AZStd::string>& prefabsToInstantiate,
            const AZStd::vector<AZStd::string>& prefabsToDetach)
        {
            for (const AZStd::string& entry : prefabsToInstantiate)
            {
                // its a .prefab file.
                auto instantiatePrefabOutcome = s_prefabPublicInterface->InstantiatePrefab(entry, parentEntity, instantiateLocation);

                if (!instantiatePrefabOutcome.IsSuccess())
                {
                    WarningDialog("Prefab Instantiation Error", instantiatePrefabOutcome.GetError());
                }
                else
                {
                    // is it in the "to detach also" list?
                    if (AZStd::find(prefabsToDetach.begin(), prefabsToDetach.end(), entry) != prefabsToDetach.end())
                    {
                        AZ::EntityId instantiatedContainerEntity = instantiatePrefabOutcome.GetValue();
                        auto detachOutcome = s_prefabPublicInterface->DetachPrefab(instantiatedContainerEntity);
                        if (!detachOutcome.IsSuccess())
                        {
                            WarningDialog("Prefab was instantiated but could not detach:", detachOutcome.GetError());
                        }
                    }
                }
            }
        }

        void PrefabSaveHandler::Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context)
        {
            using namespace AzToolsFramework;
            using namespace AzToolsFramework::AssetBrowser;
            using namespace AzQtComponents;

            if (event->isAccepted())
            {
                return;
            }

            if (ViewportDragContext* viewportContext = azrtti_cast<ViewportDragContext*>(&context))
            {
                AZ::Vector3 instantiateLocation = viewportContext->m_hitLocation;
                AZStd::vector<AZStd::string> thingsToInstantiate;
                AZStd::vector<AZStd::string> thingsToDetach;
                if (CanDragAndDropData(event->mimeData(), &thingsToInstantiate, &thingsToDetach))
                {
                    event->accept();
                    event->setDropAction(Qt::DropAction::CopyAction);
                    ScopedUndoBatch undo("Instantiate Prefab");

                    DoDragAndDropData(instantiateLocation, AZ::EntityId(), thingsToInstantiate, thingsToDetach);

                    
                }
            }
        }

        bool PrefabSaveHandler::CanDragAndDropData(
            const QMimeData* mimeData, AZStd::vector<AZStd::string>* prefabsToInstantiate, AZStd::vector<AZStd::string>* prefabsToDetach) const
        {
            using namespace AzToolsFramework;
            using namespace AzToolsFramework::AssetBrowser;
            // this handler cares if the data contains a prefab or proc prefab.
            // it also cares whether the proc prefab was directly selected, or was indirectly selected
            // (indirectly, meaning, a FBX containing the procprefab was dragged into the viewport as
            // opposed to the actual procprefab). indirectly-dragged procprefabs will auto-detach.
            bool foundSomething = false;

            if (!mimeData)
            {
                return false;
            }
                        
            // helper lambda: given a product, evaluate whether it is a proc prefab.
            // if it is, add it to the list of things that should be instantiated.
            // if its not selected, add it to the list of things that should also be detached.
            auto evaluateProduct = [&](const ProductAssetBrowserEntry* entry, bool isSelected)
            {
                AZStd::string extension;
                AZ::StringFunc::Path::GetExtension(entry->GetName().c_str(), extension);
                if (AZ::StringFunc::Equal(PrefabSaveHandler::s_procPrefabFileExtension, extension.c_str()))
                {
                    // its a proc prefab file.
                    if (prefabsToInstantiate)
                    {
                        AZStd::string actualPath = entry->GetFullPath();
                        prefabsToInstantiate->emplace_back(actualPath);
                        if ((prefabsToDetach) && (!isSelected))
                        {
                            // this procprefab should auto detach if it is not directly selected:
                            prefabsToDetach->push_back(actualPath);
                        }
                    }
                    foundSomething = true;
                }
            };

            AZStd::vector<const AssetBrowserEntry*> decoded;
            if (!Utils::FromMimeData(mimeData, decoded))
            {
                return false;
            }

            for (auto entry : decoded)
            {
                if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                {
                    // if you directly selected a .prefab source file, add it to the instantiate list:
                    const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
                    AZStd::string extension;
                    AZ::StringFunc::Path::GetExtension(sourceEntry->GetFullPath().c_str(), extension);
                    if (AZ::StringFunc::Equal(PrefabSaveHandler::s_prefabFileExtension, extension.c_str()))
                    {
                        // its a prefab file.
                        if (prefabsToInstantiate)
                        {
                            prefabsToInstantiate->emplace_back(sourceEntry->GetFullPath());
                        }
                        foundSomething = true;
                    }
                    else
                    {
                        // user is dragging and dropping something that is not a prefab.
                        // but its still valid if one of the products of the thing being dragged
                        // is a procprefab:
                        AZStd::vector<const ProductAssetBrowserEntry*> products;
                        entry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);
                        for (auto product : products)
                        {
                            evaluateProduct(product, false);
                        }
                    }
                }
                if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
                {
                    // we'll get here if the user specifically and intentionally selected a product
                    // in the asset browser instead of a source (ie, selecting an actual model or procprefab
                    // instead of the fbx that produced it).
                    evaluateProduct(azrtti_cast<const ProductAssetBrowserEntry*>(entry), true);
                }
            }

            return foundSomething;
        }

         // Drag and drop overrides:
        int PrefabSaveHandler::GetDragAndDropItemViewEventsPriority() const
        {
            return CommonDragAndDropBusTraits::s_HighPriority; // This class gets ahead of other built in ones.
        }
        // listview/outliner dragging:

        void PrefabSaveHandler::CanDropItemView(bool& accepted, AzQtComponents::DragAndDropContextBase& context)
        {
            if (accepted)
            {
                return;
            }
            using namespace AzToolsFramework;
            if (EntityOutlinerDragAndDropContext* outlinerContext = azrtti_cast<EntityOutlinerDragAndDropContext*>(&context))
            {
                if (CanDragAndDropData(outlinerContext->m_dataBeingDropped))
                {
                    accepted = true;
                }
            }
        }

        void PrefabSaveHandler::DoDropItemView(bool& accepted, AzQtComponents::DragAndDropContextBase& context)
        {
            using namespace AzToolsFramework;
            if (accepted)
            {
                return;
            }

            if (EntityOutlinerDragAndDropContext* outlinerContext = azrtti_cast<EntityOutlinerDragAndDropContext*>(&context))
            {
                AZStd::vector<AZStd::string> thingsToInstantiate;
                AZStd::vector<AZStd::string> thingsToDetach;

                AZ::Vector3 instantiateLocation = AZ::Vector3::CreateZero();
                if (!outlinerContext->m_parentEntity.IsValid())
                {
                    EditorRequestBus::BroadcastResult(instantiateLocation, &EditorRequestBus::Events::GetWorldPositionAtViewportCenter);
                }
               
                if (CanDragAndDropData(outlinerContext->m_dataBeingDropped, &thingsToInstantiate, &thingsToDetach))
                {
                    accepted = true;
                    DoDragAndDropData(instantiateLocation, outlinerContext->m_parentEntity, thingsToInstantiate, thingsToDetach);
                }
            }
        }

        int PrefabSaveHandler::ExecuteClosePrefabDialog(TemplateId templateId)
        {
            if (s_prefabSystemComponentInterface->AreDirtyTemplatesPresent(templateId))
            {
                auto prefabSaveSelectionDialog = ConstructClosePrefabDialog(templateId);

                int prefabSaveSelection = prefabSaveSelectionDialog->exec();

                if (prefabSaveSelection == QDialog::Accepted)
                {
                    SavePrefabsInDialog(prefabSaveSelectionDialog.get());
                }

                return prefabSaveSelection;
            }

            return QDialogButtonBox::DestructiveRole;
        }

        void PrefabSaveHandler::InitializePrefabToastNotificationsView()
        {
            QWidget* mainWindow = nullptr;
            AzToolsFramework::EditorRequestBus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
            AZ_Assert(mainWindow != nullptr, "PrefabSaveHandler - Editor MainWindow is not available.");

            m_prefabToastNotificationsView =
                AZStd::make_unique<AzToolsFramework::ToastNotificationsView>(mainWindow, AZ_CRC_CE("PrefabToastNotificationsView"));

            m_prefabToastNotificationsView->SetRejectDuplicates(false);
            m_prefabToastNotificationsView->SetAnchorPoint(QPoint(1, 1));
            m_prefabToastNotificationsView->SetOffset(QPoint(10, 75));
            m_prefabToastNotificationsView->OnShow();
        }

        void PrefabSaveHandler::ExecuteSavePrefabDialog(AZ::EntityId entityId)
        {
            if (const InstanceOptionalReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId); instance.has_value())
            {
                ExecuteSavePrefabDialog(instance->get().GetTemplateId(), true);
            }
        }

        void PrefabSaveHandler::ExecuteSavePrefabDialog(TemplateId templateId, bool useSaveAllPrefabsPreference)
        {
            if (templateId == Prefab::InvalidTemplateId)
            {
                return;
            }

            auto prefabTemplate = s_prefabSystemComponentInterface->FindTemplate(templateId);

            if (!prefabTemplate.has_value())
            {
                return;
            }

            AZ::IO::Path prefabTemplatePath = prefabTemplate->get().GetFilePath();

            if (s_prefabSystemComponentInterface->IsTemplateDirty(templateId))
            {
                if (s_prefabLoaderInterface->SaveTemplate(templateId) == false)
                {
                    AZ_Error("Prefab", false, "Template '%s' could not be saved successfully.", prefabTemplatePath.c_str());
                    return;
                }
            }

            if (s_prefabSystemComponentInterface->AreDirtyTemplatesPresent(templateId))
            {
                if (useSaveAllPrefabsPreference)
                {
                    SaveAllPrefabsPreference saveAllPrefabsPreference = s_prefabLoaderInterface->GetSaveAllPrefabsPreference();

                    if (saveAllPrefabsPreference == SaveAllPrefabsPreference::SaveAll)
                    {
                        s_prefabSystemComponentInterface->SaveAllDirtyTemplates(templateId);
                        return;
                    }
                    else if (saveAllPrefabsPreference == SaveAllPrefabsPreference::SaveNone)
                    {
                        return;
                    }
                }

                AZStd::unique_ptr<QDialog> savePrefabDialog = ConstructSavePrefabDialog(templateId, useSaveAllPrefabsPreference);
                if (savePrefabDialog)
                {
                    int prefabSaveSelection = savePrefabDialog->exec();

                    if (prefabSaveSelection == QDialog::Accepted)
                    {
                        SavePrefabsInDialog(savePrefabDialog.get());
                    }
                }
            }
        }

        bool PrefabSaveHandler::QueryUserForPrefabFilePath(AZStd::string& outPrefabFilePath)
        {
            AssetSelectionModel selection;

            // Note, string filter will match every source file CONTAINING ".prefab".
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

            AssetBrowserComponentRequestBus::Broadcast(
                &AssetBrowserComponentRequests::PickAssets, selection, AzToolsFramework::GetActiveWindow());

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

        bool PrefabSaveHandler::QueryUserForProceduralPrefabAsset(AZStd::string& outPrefabAssetPath)
        {
            using namespace AzToolsFramework;
            auto selection = AssetBrowser::AssetSelectionModel::AssetTypeSelection(azrtti_typeid<AZ::Prefab::ProceduralPrefabAsset>());
            EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);

            if (!selection.IsValid())
            {
                return false;
            }

            auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
            if (product == nullptr)
            {
                return false;
            }
            outPrefabAssetPath = product->GetRelativePath();
            return true;
        }

        PrefabSaveHandler::PrefabSaveResult PrefabSaveHandler::IsPrefabPathValidForAssets(
            QWidget* activeWindow, QString prefabPath, AZStd::string& retrySavePath)
        {
            bool assetSetFoldersRetrieved = false;
            AZStd::vector<AZStd::string> assetSafeFolders;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                assetSetFoldersRetrieved, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetSafeFolders, assetSafeFolders);

            if (!assetSetFoldersRetrieved)
            {
                // If the asset safe list couldn't be retrieved, don't block the user but warn them.
                AZ_Warning("Prefab", false, "Unable to verify that the prefab file to create is in a valid path.");
            }
            else
            {
                AZ::IO::FixedMaxPath lexicallyNormalPath = AZ::IO::PathView(prefabPath.toUtf8().constData()).LexicallyNormal();

                bool isPathSafeForAssets = false;
                for (const AZStd::string& assetSafeFolder : assetSafeFolders)
                {
                    AZ::IO::PathView assetSafeFolderView(assetSafeFolder);
                    // Check if the prefabPath is relative to the safe asset directory.
                    // The Path classes are being used to make this check case insensitive.
                    if (lexicallyNormalPath.IsRelativeTo(assetSafeFolderView))
                    {
                        isPathSafeForAssets = true;
                        break;
                    }
                }

                if (!isPathSafeForAssets)
                {
                    // Put an error in the console, so the log files have info about this error, or the user can look up the error after
                    // dismissing it.
                    constexpr const char* errorMessage =
                        "You can only save prefabs to either your game project folder or the Gems folder. Update the location and try "
                        "again.\n\n"
                        "You can also review and update your save locations in the Registry/AssetProcessorPlatformConfig.setreg file or "
                        "your Gem's "
                        "Registry/assetprocessor_settings.setreg file.";
                    AZ_Error("Prefab", false, errorMessage);

                    // Display a pop-up, the logs are easy to miss. This will make sure a user who encounters this error immediately knows
                    // their prefab save has failed.
                    QMessageBox msgBox(activeWindow);
                    msgBox.setIcon(QMessageBox::Icon::Warning);
                    msgBox.setTextFormat(Qt::RichText);
                    msgBox.setWindowTitle(QObject::tr("Invalid save location"));
                    msgBox.setText(QObject::tr(errorMessage));
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

        void PrefabSaveHandler::GenerateSuggestedPrefabPath(
            const AZStd::string& prefabName, const AZStd::string& targetDirectory, AZStd::string& suggestedFullPath)
        {
            // Generate full suggested path from prefabName - if given NewPrefab as prefabName,
            // NewPrefab_001.prefab would be tried, and if that already existed we would suggest
            // the first unused number value (NewPrefab_002.prefab etc.)
            AZStd::string normalizedTargetDirectory = targetDirectory;
            AZ::StringFunc::Path::Normalize(normalizedTargetDirectory);

            // Convert spaces in entity names to underscores
            AZStd::string prefabNameFiltered = prefabName;
            AZ::StringFunc::Replace(prefabNameFiltered, ' ', '_');

            auto settings = AZ::UserSettings::CreateFind<PrefabUserSettings>(AZ_CRC_CE("PrefabUserSettings"), AZ::UserSettings::CT_LOCAL);
            if (settings->m_autoNumber)
            {
                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

                const AZ::u32 maxPrefabNumber = 1000;
                for (AZ::u32 prefabNumber = 1; prefabNumber < maxPrefabNumber; ++prefabNumber)
                {
                    AZStd::string possiblePath;
                    AZ::StringFunc::Path::Join(
                        normalizedTargetDirectory.c_str(),
                        AZStd::string::format("%s_%3.3u%s", prefabNameFiltered.c_str(), prefabNumber, s_prefabFileExtension.c_str())
                            .c_str(),
                        possiblePath);

                    if (!fileIO || !fileIO->Exists(possiblePath.c_str()))
                    {
                        suggestedFullPath = possiblePath;
                        break;
                    }
                }
            }
            else
            {
                // use the entity name as the file name regardless of it already existing, the OS will ask the user to overwrite the file in
                // that case.
                AZ::StringFunc::Path::Join(
                    normalizedTargetDirectory.c_str(),
                    AZStd::string::format("%s%s", prefabNameFiltered.c_str(), s_prefabFileExtension.c_str()).c_str(), suggestedFullPath);
            }
        }

        bool PrefabSaveHandler::QueryUserForPrefabSaveLocation(
            const AZStd::string& suggestedName,
            const char* initialTargetDirectory,
            AZ::u32 prefabUserSettingsId,
            QWidget* activeWindow,
            AZStd::string& outPrefabName,
            AZStd::string& outPrefabFilePath)
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
                    AZ_PROFILE_FUNCTION(AzToolsFramework);
                    saveAs = QFileDialog::getSaveFileName(
                        nullptr, QString("Save As..."), saveAsInitialSuggestedFullPath.c_str(), QString("Prefabs (*.prefab)"));
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
                    WarningDialog(
                        "Prefab Creation Failed.",
                        "Unicode file name is not supported. \r\n"
                        "Please use ASCII characters to name your prefab.");
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

                WarningDialog("Prefab Already Exists", message);
                return false;
            }

            // We prevent users from creating a new prefab with the same relative path that's already
            // been used by an existing prefab in other places (e.g. Gems) because the AssetProcessor
            // generates asset ids based on relative paths. This is unnecessary once AssetProcessor
            // starts to generate UUID to every asset regardless of paths.
            {
                AZStd::string prefabRelativeName;
                bool relativePathFound;
                AssetSystemRequestBus::BroadcastResult(
                    relativePathFound, &AssetSystemRequestBus::Events::GetRelativeProductPathFromFullSourceOrProductPath, targetPath,
                    prefabRelativeName);

                AZ::Data::AssetId prefabAssetId;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    prefabAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, prefabRelativeName.c_str(),
                    AZ::Data::s_invalidAssetType, false);
                if (prefabAssetId.IsValid())
                {
                    const AZStd::string message = AZStd::string::format(
                        "A prefab with the relative path \"%s\" already exists in the Asset Database. \r\n\r\n"
                        "Overriding it will damage instances or cascades of this prefab. \r\n\r\n"
                        "Instead, either push entities/fields to the prefab, or save to a different location.",
                        prefabRelativeName.c_str());

                    WarningDialog("Prefab Path Error", message);
                    return false;
                }
            }

            AZStd::string saveDir(prefabSaveFileInfo.absoluteDir().absolutePath().toUtf8().constData());
            SetPrefabSaveLocation(saveDir, prefabUserSettingsId);

            outPrefabName = prefabName.toUtf8().constData();
            outPrefabFilePath = targetPath.c_str();

            return true;
        }

        void PrefabSaveHandler::GenerateSuggestedFilenameFromEntities(const EntityIdList& entityIds, AZStd::string& outName)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

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

        bool PrefabSaveHandler::AppendEntityToSuggestedFilename(AZStd::string& filename, AZ::EntityId entityId)
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

        void PrefabSaveHandler::SavePrefabsInDialog(QDialog* unsavedPrefabsDialog)
        {
            QList<QLabel*> unsavedPrefabFileLabels = unsavedPrefabsDialog->findChildren<QLabel*>(UnsavedPrefabFileName);
            if (unsavedPrefabFileLabels.size() > 0)
            {
                for (const QLabel* unsavedPrefabFileLabel : unsavedPrefabFileLabels)
                {
                    AZStd::string unsavedPrefabFileName = unsavedPrefabFileLabel->property("FilePath").toString().toUtf8().data();
                    AzToolsFramework::Prefab::TemplateId unsavedPrefabTemplateId =
                        s_prefabSystemComponentInterface->GetTemplateIdFromFilePath(unsavedPrefabFileName.data());
                    [[maybe_unused]] bool isTemplateSavedSuccessfully = s_prefabLoaderInterface->SaveTemplate(unsavedPrefabTemplateId);
                    AZ_Error(
                        "Prefab", isTemplateSavedSuccessfully, "Prefab '%s' could not be saved successfully.",
                        unsavedPrefabFileName.c_str());
                }
            }
        }

        AZStd::unique_ptr<QDialog> PrefabSaveHandler::ConstructSavePrefabDialog(TemplateId templateId, bool useSaveAllPrefabsPreference)
        {
            AZStd::unique_ptr<QDialog> savePrefabDialog = AZStd::make_unique<QDialog>(AzToolsFramework::GetActiveWindow());

            savePrefabDialog->setWindowTitle("Unsaved files detected");

            // Main Content section begins.
            savePrefabDialog->setObjectName(SavePrefabDialog);
            QBoxLayout* contentLayout = new QVBoxLayout(savePrefabDialog.get());

            QFrame* prefabSavedMessageFrame = new QFrame(savePrefabDialog.get());
            QHBoxLayout* prefabSavedMessageLayout = new QHBoxLayout(savePrefabDialog.get());
            prefabSavedMessageFrame->setObjectName(PrefabSavedMessageFrame);
            prefabSavedMessageFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

            // Add a checkMark icon next to the level entities saved message.
            QPixmap checkMarkIcon(QString(":/Notifications/checkmark.svg"));
            QLabel* prefabSavedSuccessfullyIconContainer = new QLabel(savePrefabDialog.get());
            prefabSavedSuccessfullyIconContainer->setPixmap(checkMarkIcon);
            prefabSavedSuccessfullyIconContainer->setFixedWidth(checkMarkIcon.width());

            // Add a message that level entities are saved successfully.

            auto prefabTemplate = s_prefabSystemComponentInterface->FindTemplate(templateId);
            AZ::IO::Path prefabTemplatePath = prefabTemplate->get().GetFilePath();
            QLabel* prefabSavedSuccessfullyLabel = new QLabel(
                QString("Prefab '<b>%1</b>' has been saved. Do you want to save the below dependent prefabs too?")
                    .arg(prefabTemplatePath.c_str()),
                savePrefabDialog.get());
            prefabSavedMessageLayout->addWidget(prefabSavedSuccessfullyIconContainer);
            prefabSavedMessageLayout->addWidget(prefabSavedSuccessfullyLabel);
            prefabSavedMessageFrame->setLayout(prefabSavedMessageLayout);
            contentLayout->addWidget(prefabSavedMessageFrame);

            AZStd::unique_ptr<AzQtComponents::Card> unsavedPrefabsContainer = ConstructUnsavedPrefabsCard(templateId);
            contentLayout->addWidget(unsavedPrefabsContainer.release());

            contentLayout->addStretch();

            // Footer section begins.
            QHBoxLayout* footerLayout = new QHBoxLayout(savePrefabDialog.get());

            if (useSaveAllPrefabsPreference)
            {
                QFrame* footerSeparatorLine = new QFrame(savePrefabDialog.get());
                footerSeparatorLine->setObjectName(FooterSeparatorLine);
                footerSeparatorLine->setFrameShape(QFrame::HLine);
                contentLayout->addWidget(footerSeparatorLine);

                QLabel* prefabSavePreferenceHint =
                    new QLabel("You can prevent this from showing in the future by updating your preferences.", savePrefabDialog.get());
                prefabSavePreferenceHint->setToolTip(
                    "Go to 'Edit > Editor Settings > Global Preferences... > Prefab Save Settings' to update your preference");
                prefabSavePreferenceHint->setObjectName(PrefabSavePreferenceHint);
                footerLayout->addWidget(prefabSavePreferenceHint);
            }

            QDialogButtonBox* prefabSaveConfirmationButtons =
                new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::No, savePrefabDialog.get());
            footerLayout->addWidget(prefabSaveConfirmationButtons);
            contentLayout->addLayout(footerLayout);
            connect(prefabSaveConfirmationButtons, &QDialogButtonBox::accepted, savePrefabDialog.get(), &QDialog::accept);
            connect(prefabSaveConfirmationButtons, &QDialogButtonBox::rejected, savePrefabDialog.get(), &QDialog::reject);
            AzQtComponents::StyleManager::setStyleSheet(savePrefabDialog->parentWidget(), QStringLiteral("style:Editor.qss"));

            savePrefabDialog->setLayout(contentLayout);
            return AZStd::move(savePrefabDialog);
        }

        AZStd::shared_ptr<QDialog> PrefabSaveHandler::ConstructClosePrefabDialog(TemplateId templateId)
        {
            AZStd::shared_ptr<QDialog> closePrefabDialog = AZStd::make_shared<QDialog>(AzToolsFramework::GetActiveWindow());
            closePrefabDialog->setWindowTitle("Unsaved files detected");
            AZStd::weak_ptr<QDialog> closePrefabDialogWeakPtr(closePrefabDialog);
            closePrefabDialog->setObjectName(ClosePrefabDialog);

            // Main Content section begins.
            QVBoxLayout* contentLayout = new QVBoxLayout(closePrefabDialog.get());
            QFrame* prefabSaveWarningFrame = new QFrame(closePrefabDialog.get());
            QHBoxLayout* levelEntitiesSaveQuestionLayout = new QHBoxLayout(closePrefabDialog.get());
            prefabSaveWarningFrame->setObjectName(PrefabSaveWarningFrame);

            // Add a warning icon next to save prefab warning.
            prefabSaveWarningFrame->setLayout(levelEntitiesSaveQuestionLayout);
            QPixmap warningIcon(QString(":/Notifications/warning.svg"));
            QLabel* warningIconContainer = new QLabel(closePrefabDialog.get());
            warningIconContainer->setPixmap(warningIcon);
            warningIconContainer->setFixedWidth(warningIcon.width());
            levelEntitiesSaveQuestionLayout->addWidget(warningIconContainer);

            // Ask user if they want to save entities in level.
            QLabel* prefabSaveQuestionLabel = new QLabel("Do you want to save the below unsaved prefabs?", closePrefabDialog.get());
            levelEntitiesSaveQuestionLayout->addWidget(prefabSaveQuestionLabel);
            contentLayout->addWidget(prefabSaveWarningFrame);

            auto templateToSave = s_prefabSystemComponentInterface->FindTemplate(templateId);
            AZ::IO::Path templateToSaveFilePath = templateToSave->get().GetFilePath();
            AZStd::unique_ptr<AzQtComponents::Card> unsavedPrefabsCard = ConstructUnsavedPrefabsCard(templateId);
            contentLayout->addWidget(unsavedPrefabsCard.release());

            contentLayout->addStretch();

            QHBoxLayout* footerLayout = new QHBoxLayout(closePrefabDialog.get());

            QDialogButtonBox* prefabSaveConfirmationButtons = new QDialogButtonBox(
                QDialogButtonBox::Save | QDialogButtonBox::Discard | QDialogButtonBox::Cancel, closePrefabDialog.get());
            footerLayout->addWidget(prefabSaveConfirmationButtons);
            contentLayout->addLayout(footerLayout);
            QObject::connect(prefabSaveConfirmationButtons, &QDialogButtonBox::accepted, closePrefabDialog.get(), &QDialog::accept);
            QObject::connect(prefabSaveConfirmationButtons, &QDialogButtonBox::rejected, closePrefabDialog.get(), &QDialog::reject);
            QObject::connect(
                prefabSaveConfirmationButtons, &QDialogButtonBox::clicked, closePrefabDialog.get(),
                [closePrefabDialogWeakPtr, prefabSaveConfirmationButtons](QAbstractButton* button)
                {
                    int prefabSaveSelection = prefabSaveConfirmationButtons->buttonRole(button);
                    closePrefabDialogWeakPtr.lock()->done(prefabSaveSelection);
                });
            AzQtComponents::StyleManager::setStyleSheet(closePrefabDialog.get(), QStringLiteral("style:Editor.qss"));
            closePrefabDialog->setLayout(contentLayout);
            return closePrefabDialog;
        }

        AZStd::unique_ptr<AzQtComponents::Card> PrefabSaveHandler::ConstructUnsavedPrefabsCard(TemplateId templateId)
        {
            FlowLayout* unsavedPrefabsLayout = new FlowLayout(nullptr);

            AZStd::set<AZ::IO::PathView> dirtyTemplatePaths = s_prefabSystemComponentInterface->GetDirtyTemplatePaths(templateId);

            for (AZ::IO::PathView dirtyTemplatePath : dirtyTemplatePaths)
            {
                QLabel* prefabNameLabel =
                    new QLabel(QString("<u>%1</u>").arg(dirtyTemplatePath.Filename().Native().data()), AzToolsFramework::GetActiveWindow());
                prefabNameLabel->setObjectName(UnsavedPrefabFileName);
                prefabNameLabel->setWordWrap(true);
                prefabNameLabel->setToolTip(dirtyTemplatePath.Native().data());
                prefabNameLabel->setProperty("FilePath", dirtyTemplatePath.Native().data());
                unsavedPrefabsLayout->addWidget(prefabNameLabel);
            }

            AZStd::unique_ptr<AzQtComponents::Card> unsavedPrefabsContainer =
                AZStd::make_unique<AzQtComponents::Card>(AzToolsFramework::GetActiveWindow());
            unsavedPrefabsContainer->setObjectName(SaveDependentPrefabsCard);
            unsavedPrefabsContainer->setTitle("Unsaved Prefabs");
            unsavedPrefabsContainer->header()->setHasContextMenu(false);
            unsavedPrefabsContainer->header()->setIcon(QIcon(QStringLiteral(":/Entity/prefab_edit.svg")));

            QFrame* unsavedPrefabsFrame = new QFrame(unsavedPrefabsContainer.get());
            unsavedPrefabsFrame->setLayout(unsavedPrefabsLayout);
            QScrollArea* unsavedPrefabsScrollArea = new QScrollArea(unsavedPrefabsContainer.get());
            unsavedPrefabsScrollArea->setWidget(unsavedPrefabsFrame);
            unsavedPrefabsScrollArea->setWidgetResizable(true);
            unsavedPrefabsContainer->setContentWidget(unsavedPrefabsScrollArea);

            return AZStd::move(unsavedPrefabsContainer);
        }

        void PrefabSaveHandler::SourceFileRemoved(
            AZStd::string relativePath, AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid sourceUUID)
        {
            // This gets triggered for every source file. We only need source files that are prefabs and are loaded in the current level.
            TemplateId loadedTemplateId = s_prefabSystemComponentInterface->GetTemplateIdFromFilePath(relativePath.c_str());
            if (loadedTemplateId != InvalidTemplateId)
            {
                InstanceSetConstReference instancesMappedToTemplate =
                    m_templateInstanceMapperInterface->FindInstancesOwnedByTemplate(loadedTemplateId);

                if (!instancesMappedToTemplate.has_value() || instancesMappedToTemplate->get().size() == 0)
                {
                    // There are no instances mapping to this template. So it can be removed from memory.
                    s_prefabSystemComponentInterface->RemoveTemplate(loadedTemplateId);
                }
                else
                {
                    m_prefabToastNotificationsView->ShowToastNotification(
                        AzQtComponents::ToastConfiguration{ AzQtComponents::ToastType::Warning, "Missing prefab(s) detected", " See Editor Console for details." });
                    AZ_Warning(
                        "Prefab",
                        false,
                        "Missing Prefab '%s'. %u instance(s) exist in the level. Save the prefab to restore the file, or delete all "
                        "instances to resolve this warning.",
                        relativePath.c_str(),
                        instancesMappedToTemplate->get().size());
                    s_prefabSystemComponentInterface->SetTemplateDirtyFlag(loadedTemplateId, true);
                }
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
