/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetEditorTab.h"
#include "AssetEditorWidget.h"

#include <API/EditorAssetSystemAPI.h>
#include <API/ToolsApplicationAPI.h>

#include <AssetEditor/AssetEditorBus.h>
#include <AssetEditor/AssetEditorHeader.h>

#include <AssetBrowser/AssetSelectionModel.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/sort.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <SourceControl/SourceControlAPI.h>

#include <UI/PropertyEditor/PropertyRowWidget.hxx>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <UI/DocumentPropertyEditor/FilteredDPE.h>

#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QFileInfo::d_ptr': class 'QSharedDataPointer<QFileInfoPrivate>' needs to have
                                                          // dll-interface to be used by clients of class 'QFileInfo'
#include <QFileDialog>
AZ_POP_DISABLE_WARNING
#include <QAction>

namespace AzToolsFramework
{
    AZ_CVAR(
        bool,
        ed_enableDPEAssetEditor,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
        "If set, enables experimental Document Property Editor for the AssetEditor");

    namespace AssetEditor
    {
        // Amount to add on to the save confirm dialog text width to account for the icon etc when padding.
        static constexpr int SaveConfirmDialogPadding = 120;

        using AssetCheckoutCallback = AZStd::function<void(bool, const AZStd::string&, const AZStd::string&)>;

        void AssetCheckoutCommon(
            const AZ::Data::AssetId& id,
            AZ::Data::Asset<AZ::Data::AssetData> asset,
            [[maybe_unused]] AZ::SerializeContext* serializeContext,
            AssetCheckoutCallback assetCheckoutAndSaveCallback)
        {
            AZStd::string assetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, id);
            if (assetPath.empty())
            {
                AZ_Assert(
                    !assetPath.empty(), "A valid path needs to be resolved, if this failed, the provided asset path is not a valid asset.");
                return;
            }

            AZStd::string assetFullPath;
            AssetSystemRequestBus::Broadcast(
                &AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath, assetPath, assetFullPath);

            if (!assetFullPath.empty())
            {
                using SCCommandBus = SourceControlCommandBus;
                bool opComplete = false;
                SCCommandBus::Broadcast(
                    &SCCommandBus::Events::RequestEdit,
                    assetFullPath.c_str(),
                    true,
                    [id, asset, assetFullPath, assetCheckoutAndSaveCallback, &opComplete](
                        bool /*success*/, const SourceControlFileInfo& info)
                    {
                        if (!info.IsReadOnly())
                        {
                            AZ::Outcome<bool, AZStd::string> outcome = AZ::Success(true);
                            AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::EventResult(
                                outcome, id, &AzToolsFramework::AssetEditor::AssetEditorValidationRequests::IsAssetDataValid, asset);
                            if (!outcome.IsSuccess())
                            {
                                assetCheckoutAndSaveCallback(false, outcome.GetError(), assetFullPath);
                            }
                            else
                            {
                                AssetEditorValidationRequestBus::Event(id, &AssetEditorValidationRequests::PreAssetSave, asset);
                                assetCheckoutAndSaveCallback(true, "", assetFullPath);
                            }
                        }
                        else
                        {
                            AZStd::string error =
                                AZStd::move(AZStd::string::format("Could not check out asset file: %s.", assetFullPath.c_str()));
                            assetCheckoutAndSaveCallback(false, error, assetFullPath);
                        }
                        opComplete = true;
                    });

                // Broadcast is asynchronous above, (it queues an operation and returns immediately),
                // but hooks into local variables and other lambdas that might be destroyed if we exit scope here.
                // This operation also happens while the dialog is closing, and WILL close, destroying
                // internal buffers like the save data itself, before this async operation proceeds and calls the callback.
                // For these reasons we MUST block here, before proceeding.
                //
                // The above broadcast API also ALWAYS calls the callback, even if it failed or is unavailable, so we can bank on
                // opComplete becoming true.  It also always calls it from the Tickbus, which is always the main GUI thread.

                while (!opComplete)
                {
                    // note that the above operation could conceivably pop up a dialog that asks the user for permission
                    // to edit a file or to show a password dialog or accept a certificate or any number of source control
                    // things.  If it does, it will run its OWN modal event loop inside this event loop, which will accept
                    // user input for itself.  However, we should forbid user input events here, so that we don't end up in a
                    // double close situation.
                    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 16);
                    AZ::TickBus::ExecuteQueuedEvents();
                }
            }
            else
            {
                AZStd::string error =
                    AZStd::move(AZStd::string::format("Could not resolve path name for asset {%s}.", id.ToString<AZStd::string>().c_str()));
                assetCheckoutAndSaveCallback(false, error, AZStd::string{});
            }
        }

        namespace Status
        {
            static QString assetCreated = QObject::tr("Asset created!");
            static QString assetSaved = QObject::tr("Asset saved!");
            static QString assetSaving = QObject::tr("Asset saving!");
            static QString assetLoaded = QObject::tr("Asset loaded!");
            static QString unableToSave = QObject::tr("Failed to save asset due to validation error, check the log!");
            static QString emptyString = QObject::tr("");
        } // namespace Status

        namespace TextStrings
        {
            static QString assetLocation = QObject::tr("Asset location: ");
            static QString unsaved = QObject::tr("Unsaved");
        } // namespace TextStrings

        //////////////////////////////////////////////////////////////////////////
        // AssetEditorTab
        //////////////////////////////////////////////////////////////////////////

        AssetEditorTab::AssetEditorTab(QWidget* parent)
            : QWidget(parent)
            , m_dirty(true)
            , m_assetObserverToken(AZ::Uuid::CreateNull())
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            setObjectName("AssetEditorTab");

            QWidget* propertyEditor = nullptr;

            // use the DPE version of the AssetEditor if ed_enableDPEAssetEditor is enabled
            if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
            {
                console->GetCvarValue("ed_enableDPEAssetEditor", m_useDPE);
            }

            if (!m_useDPE)
            {
                m_propertyEditor = new ReflectedPropertyEditor(this);
                m_propertyEditor->Setup(m_serializeContext, this, true, 250);
                propertyEditor = m_propertyEditor;
            }
            else
            {
                m_adapter = AZStd::make_shared<AZ::DocumentPropertyEditor::ReflectionAdapter>();
                m_filteredWidget = new FilteredDPE(this);
                m_dpe = m_filteredWidget->GetDPE();
                propertyEditor = m_filteredWidget;
                m_propertyChangeHandler = AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeEvent::Handler(
                    [this](const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& changeInfo)
                    {
                        this->OnDocumentPropertyChanged(changeInfo);
                    });
                m_adapter->ConnectPropertyChangeHandler(m_propertyChangeHandler);
            }

            propertyEditor->setObjectName("AssetEditorWidgetPropertyEditor");
            propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            propertyEditor->show();

            QVBoxLayout* mainLayout = new QVBoxLayout();
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(0);

            m_header = new Ui::AssetEditorHeader(this);
            mainLayout->addWidget(m_header);
            m_header->show();

            propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            mainLayout->addWidget(propertyEditor);

            setLayout(mainLayout);

            AzFramework::AssetCatalogEventBus::Handler::BusConnect();

            m_parentEditorWidget = qobject_cast<AssetEditorWidget*>(parent->parentWidget());

            m_warningIcon = QIcon(":/Cards/img/UI20/Cards/warning.svg");
        }

        AssetEditorTab::~AssetEditorTab()
        {
        }

        bool AssetEditorTab::UserRefusedSave()
        {
            return m_userRefusedSave;
        }

        void AssetEditorTab::ClearUserRefusedSaveFlag()
        {
            m_userRefusedSave = false;
        }

        bool AssetEditorTab::TrySaveAsset(const AZStd::function<void()>& savedCallback)
        {
            if (WaitingToSave())
            {
                // Re call SaveAsset directly to ensure the save data is up to date.
                SaveAssetImpl(savedCallback);
                return true;
            }

            if (!m_dirty)
            {
                if (savedCallback)
                {
                    savedCallback();
                }
                return true;
            }

            m_userRefusedSave = false;

            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Save Changes?"));
            QString msgText(tr("Would you like to save the changes to %1 prior to closing?").arg(m_currentAsset));
            msgBox.setText(msgText);
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Yes);
            msgBox.setIconPixmap(m_warningIcon.pixmap(48, 48));

            // Add padding to stretch the dialog to a single line.
            const int txtWidth = msgBox.fontMetrics().horizontalAdvance(msgText);
            QSpacerItem* horizontalSpacer =
                new QSpacerItem(txtWidth + SaveConfirmDialogPadding, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            QGridLayout* layout = (QGridLayout*)msgBox.layout();
            layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());

            const int result = msgBox.exec();

            if (result == QMessageBox::No)
            {
                m_userRefusedSave = true;
                if (savedCallback)
                {
                    savedCallback();
                }
                return true;
            }
            if (result == QMessageBox::Yes)
            {
                SaveAssetImpl(savedCallback);
                return true;
            }

            // If the save was cancelled, return false to stop any further actions.
            return false;
        }

        void AssetEditorTab::SaveAssetImpl(const AZStd::function<void()>& savedCallback)
        {
            if (!m_sourceAssetId.IsValid())
            {
                SaveAsDialog();

                if (savedCallback)
                {
                    savedCallback();
                }
            }
            else if (m_dirty)
            {
                SetStatusText(Status::assetSaving);

                GenerateSaveDataSnapshot();

                // Clear the dirty flag now so that attempting to close the window during a save doesn't ask the user to save again,
                // unless there are further changes.
                m_dirty = false;
                m_parentEditorWidget->UpdateSaveMenuActionsStatus();

                if (WaitingToSave())
                {
                    // Don't need to do the save, as we've just overwritten the data that the previously queued save will write out.
                    return;
                }
                m_waitingToSave = true;
                AssetCheckoutCommon(
                    m_sourceAssetId,
                    m_inMemoryAsset,
                    m_serializeContext,
                    [this, savedCallback](bool checkoutSuccess, const AZStd::string& error, const AZStd::string& assetFullPath)
                    {
                        AZStd::string saveError = error;
                        bool saveSuccessful = false;

                        if (checkoutSuccess)
                        {
                            saveError = AZStd::move(AZStd::string::format("Could not write asset file: %s.", assetFullPath.c_str()));
                            if (!m_saveData.empty())
                            {
                                if (AZ::Utils::SaveStreamToFile(assetFullPath, m_saveData))
                                {
                                    m_saveData.clear();
                                    saveSuccessful = true;
                                }
                            }
                        }

                        if (saveSuccessful)
                        {
                            Q_EMIT OnAssetSavedSignal();
                                            
                            m_parentEditorWidget->AddRecentPath(assetFullPath);
                            if (!m_useDPE)
                            {
                                m_propertyEditor->QueueInvalidation(Refresh_AttributesAndValues);
                            }
                            SetStatusText(Status::assetSaved);

                            if (savedCallback)
                            {
                                savedCallback();
                            }
                        }
                        else
                        {
                            m_dirty = true;

                            m_parentEditorWidget->UpdateSaveMenuActionsStatus();
                            SetStatusText(Status::unableToSave);
                            Q_EMIT OnAssetSaveFailedSignal(saveError);
                        }

                        m_waitingToSave = false;
                    });
            }

            m_parentEditorWidget->UpdateTabTitle(this);
        }

        void AssetEditorTab::SaveAsset()
        {
            if (!m_sourceAssetId.IsValid())
            {
                SaveAsDialog();
                return;
            }

            if (!m_dirty)
            {
                return;
            }
            
            SaveAssetImpl(nullptr);
        }

        bool AssetEditorTab::SaveImpl(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const QString& saveAsPath)
        {
            if (!saveAsPath.isEmpty())
            {
                AZStd::string targetFilePath(saveAsPath.toUtf8().constData());

                if (!m_useDPE)
                {
                    m_propertyEditor->ForceQueuedInvalidation();
                }

                AZStd::vector<char> byteBuffer;
                AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

                auto assetHandler = const_cast<AZ::Data::AssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(asset.GetType()));
                if (assetHandler->SaveAssetData(asset, &byteStream))
                {
                    AZ::IO::FileIOStream fileStream(targetFilePath.c_str(), AZ::IO::OpenMode::ModeWrite);
                    if (!fileStream.IsOpen())
                    {
                        AZ_Warning(
                            "Asset Editor Widget", false, "Could not open file for writing - invalid path (%s).", targetFilePath.c_str());
                        return false;
                    }

                    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePathKeepCase, targetFilePath);
                    m_expectedAddedAssetPath = targetFilePath;
                    AZStd::to_lower(m_expectedAddedAssetPath.begin(), m_expectedAddedAssetPath.end());

                    SourceControlCommandBus::Broadcast(
                        &SourceControlCommandBus::Events::RequestEdit,
                        targetFilePath.c_str(),
                        true,
                        [](bool, const SourceControlFileInfo&)
                        {
                        });

                    fileStream.Write(byteBuffer.size(), byteBuffer.data());

                    AZStd::string watchFolder;
                    AZ::Data::AssetInfo assetInfo;
                    bool sourceInfoFound{};
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                        sourceInfoFound,
                        &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
                        targetFilePath.c_str(),
                        assetInfo,
                        watchFolder);

                    if (sourceInfoFound)
                    {
                        m_sourceAssetId = assetInfo.m_assetId;
                    }

                    AZStd::string fileName = targetFilePath;

                    if (AzFramework::StringFunc::Path::Normalize(fileName))
                    {
                        AzFramework::StringFunc::Path::StripPath(fileName);
                        m_currentAsset = fileName.c_str();

                        m_parentEditorWidget->SetLastSavePath(targetFilePath);
                        AZStd::size_t findIndex = targetFilePath.find(fileName.c_str());

                        if (findIndex != AZStd::string::npos)
                        {
                            AZStd::string savePath = m_parentEditorWidget->GetLastSavePath().toUtf8().data();
                            m_parentEditorWidget->SetLastSavePath(savePath.substr(0, findIndex));
                        }
                    }

                    m_dirty = false;
                    m_parentEditorWidget->AddRecentPath(targetFilePath);
                    m_parentEditorWidget->UpdateTabTitle(this);
                    SetStatusText(Status::assetCreated);

                    if (m_closeTabAfterSave)
                    {
                        m_parentEditorWidget->CloseTab(this);
                    }

                    if (m_closeParentAfterSave)
                    {
                        m_parentEditorWidget->CloseTabAndContainerIfEmpty(this);
                    }

                    return true;
                }
                else
                {
                    SetStatusText(Status::unableToSave);
                    return false;
                }
            }

            return false;
        }

        void AssetEditorTab::CreateAsset(AZ::Data::AssetType assetType, const QString& assetName, const AZ::Uuid& observerToken)
        {
            m_assetObserverToken = observerToken;

            if (m_parentEditorWidget->IsValidAssetType(assetType))
            {
                CreateAssetImpl(assetType, assetName);
            }
            else
            {
                AZ_Assert(
                    false,
                    "The AssetEditorTab only supports Generic Asset Types, make sure your type has a handler that implements "
                    "GenericAssetHandler");
            }
        }

        void AssetEditorTab::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_dirty = false;

            AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

            // Clone the asset
            AZ::Data::AssetId newAssetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
            m_inMemoryAsset = AZ::Data::AssetManager::Instance().CreateAsset(newAssetId, asset.GetType());
            auto serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
            serializeContext->CloneObjectInplace((*m_inMemoryAsset.GetData()), asset.GetData());

            // Make sure the saved state key is reset since this could be a file opened with the same property editor intance
            m_savedStateKey = AZ::Crc32(&asset.GetId(), sizeof(AZ::Data::AssetId));

            UpdatePropertyEditor(m_inMemoryAsset);

            QString assetStatus = Status::assetLoaded;

            if (!m_sourceAssetId.IsValid())
            {
                SetStatusText(Status::assetCreated);
            }

            SetupHeader();
            m_parentEditorWidget->SetCurrentTab(this);
            m_parentEditorWidget->UpdateSaveMenuActionsStatus();
        }

        void AssetEditorTab::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            // Keep a reference to the reloaded asset data
            OnAssetReady(asset);
        }

        void AssetEditorTab::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_dirty = false;
            //m_saveAssetAction->setEnabled(false);

            if (!m_useDPE)
            {
                m_propertyEditor->ClearInstances();
            }

            if (AZ::Data::AssetBus::MultiHandler::BusIsConnectedId(asset.GetId()))
            {
                AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
            }
            QString errString = tr("Failed to load %1!").arg(asset.GetHint().c_str());
            AZ_Error("Asset Editor", false, errString.toUtf8());
            QMessageBox::warning(this, tr("Error!"), errString);
        }


        void AssetEditorTab::UpdatePropertyEditor(AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {           
            if (m_useDPE)
            {
                m_adapter->SetValue(asset.Get(), asset.GetType());
                m_dpe->SetAdapter(m_adapter);
                m_dpe->setEnabled(true);

                m_dpe->SetSavedStateKey(m_savedStateKey, "AssetEditor");
            }
            else
            {
                m_propertyEditor->ClearInstances();
                m_propertyEditor->SetSavedStateKey(m_savedStateKey);
                m_propertyEditor->AddInstance(asset.Get(), asset.GetType(), nullptr);

                m_propertyEditor->InvalidateAll();
                m_propertyEditor->setEnabled(true);
            }
        }

        bool AssetEditorTab::IsDirty() const
        {
            return m_dirty;
        }

        bool AssetEditorTab::WaitingToSave() const
        {
            return m_waitingToSave;
        }

        const QString& AssetEditorTab::GetAssetName()
        {
            return m_currentAsset;
        }

        bool AssetEditorTab::SaveAsDialog()
        {
            AZStd::vector<AZStd::string> assetTypeExtensions;
            AZ::AssetTypeInfoBus::Event(m_inMemoryAsset.GetType(), &AZ::AssetTypeInfo::GetAssetTypeExtensions, assetTypeExtensions);
            QString filter;
            if (assetTypeExtensions.empty())
            {
                filter = tr("All Files (*.*)");
            }
            else
            {
                QString assetTypeName;
                AZ::AssetTypeInfoBus::EventResult(assetTypeName, m_inMemoryAsset.GetType(), &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
                filter.append(assetTypeName);
                filter.append(" (");
                for (size_t i = 0, n = assetTypeExtensions.size(); i < n; ++i)
                {
                    const char* ext = assetTypeExtensions[i].c_str();
                    if (ext[0] == '.')
                    {
                        ++ext;
                    }

                    filter.append("*.");
                    filter.append(ext);
                    if (i < n - 1)
                    {
                        filter.append(", ");
                    }
                }
                filter.append(")");
            }

            const QString saveAs = AzQtComponents::FileDialog::GetSaveFileName(
                AzToolsFramework::GetActiveWindow(), tr("Save As..."), m_parentEditorWidget->GetLastSavePath(), filter);

            return SaveImpl(m_inMemoryAsset, saveAs);
        }

        bool AssetEditorTab::SaveAssetToPath(AZStd::string_view assetPath)
        {
            return SaveImpl(m_inMemoryAsset, assetPath.data());
        }

        void AssetEditorTab::LoadAsset(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType, const QString& assetName)
        {
            m_sourceAssetId = assetId;
            m_currentAsset = assetName;

            auto asset = AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::Default);

            asset.BlockUntilLoadComplete();
            
            if (asset.IsReady())
            {
                OnAssetReady(asset);
            }
            else
            {
                if (m_inMemoryAsset)
                {
                    AZ::Data::AssetBus::MultiHandler::BusDisconnect(m_inMemoryAsset.GetId());
                }

                AZ::Data::AssetBus::MultiHandler::BusConnect(asset.GetId());

                // Need to disable editing until OnAssetReady.
                if (!m_useDPE)
                {
                    m_propertyEditor->setEnabled(false);
                }
                else
                {
                    m_filteredWidget->setEnabled(false);
                }
            }
        }

        const AZ::Data::AssetId& AssetEditorTab::GetAssetId() const
        {
            return m_sourceAssetId;
        }

        void AssetEditorTab::GenerateSaveDataSnapshot()
        {
            // Generate data now so that if there's a wait for the source control system, the data saved will be as it was when the save was
            // called.
            AZStd::vector<AZ::u8> newSaveData;

            // Make a stream and save a snapshot of the asset to it.
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> dstByteStream(&newSaveData);

            AssetEditorValidationRequestBus::Event(m_sourceAssetId, &AssetEditorValidationRequests::PreAssetSave, m_inMemoryAsset);

            if (AZ::Utils::SaveObjectToStream(
                    dstByteStream,
                    AZ::DataStream::ST_XML,
                    m_inMemoryAsset.Get(),
                    m_inMemoryAsset.Get()->RTTI_GetType(),
                    m_serializeContext))
            {
                AZStd::swap(newSaveData, m_saveData);
            }
        }

        void AssetEditorTab::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
            if (assetInfo.m_assetType == m_inMemoryAsset.GetType() &&
                strstr(m_expectedAddedAssetPath.c_str(), assetInfo.m_relativePath.c_str()) != nullptr)
            {
                m_expectedAddedAssetPath.clear();
                m_recentlyAddedAssetPath = assetInfo.m_relativePath;

                LoadAsset(assetId, assetInfo.m_assetType, m_currentAsset);

                m_sourceAssetId = assetId;

                AssetEditorNotificationsBus::Event(m_assetObserverToken, &AssetEditorNotifications::OnAssetCreated, assetId);
            }
        }

        void AssetEditorTab::OnCatalogAssetRemoved(const AZ::Data::AssetId& /*assetId*/, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType == m_inMemoryAsset.GetType() && assetInfo.m_relativePath.compare(m_recentlyAddedAssetPath) == 0)
            {
                m_recentlyAddedAssetPath.clear();

                // The file was removed due to errors, but the user needs to be
                // given a chance to fix the errors and try again.
                m_sourceAssetId.SetInvalid();
                m_currentAsset = "New Asset";
                
                if (!m_useDPE)
                {
                    m_propertyEditor->setEnabled(true);
                }
                else
                {
                    m_dpe->setEnabled(true);
                }

                DirtyAsset();
            }
        }

        void AssetEditorTab::CreateAssetImpl(AZ::Data::AssetType assetType, const QString& assetName)
        {
            m_sourceAssetId.SetInvalid();

            AZ::Data::AssetId newAssetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
            m_inMemoryAsset = AZ::Data::AssetManager::Instance().CreateAsset(newAssetId, assetType);

            DirtyAsset();

            if (!m_useDPE)
            {
                m_propertyEditor->ClearInstances();
            }
            m_currentAsset = assetName;

            // Make sure the saved state key is reset since this could be a file opened with the same property editor isntance
            m_savedStateKey = AZ::Crc32(&newAssetId, sizeof(AZ::Data::AssetId));

            UpdatePropertyEditor(m_inMemoryAsset);

            ExpandAll();
            SetStatusText(Status::assetCreated);
            SetupHeader();

            m_parentEditorWidget->UpdateTabTitle(this);
            m_parentEditorWidget->SetCurrentTab(this);
        }

        void AssetEditorTab::AfterPropertyModified(InstanceDataNode* /*node*/)
        {
            DirtyAsset();
        }
        
        void AssetEditorTab::OnDocumentPropertyChanged(const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& changeInfo)
        {
            if (changeInfo.changeType == AZ::DocumentPropertyEditor::Nodes::ValueChangeType::FinishedEdit)
            {
                DirtyAsset();
            }
        }

        void AssetEditorTab::RequestPropertyContextMenu(InstanceDataNode* node, const QPoint& point)
        {
            if (node && node->IsDifferentVersusComparison())
            {
                QMenu menu;
                QAction* resetAction = menu.addAction(tr("Reset value"));
                connect(
                    resetAction,
                    &QAction::triggered,
                    this,
                    [this, node]()
                    {
                        const InstanceDataNode* orig = node->GetComparisonNode();
                        if (orig)
                        {
                            InstanceDataHierarchy::CopyInstanceData(orig, node, m_serializeContext);

                            if (!m_useDPE)
                            {
                                PropertyRowWidget* widget = m_propertyEditor->GetWidgetFromNode(node);
                               if (widget)
                               {
                                   widget->DoPropertyNotify();
                               }

                               m_propertyEditor->QueueInvalidation(Refresh_Values);
                            }
                        }
                    });
                menu.exec(point);
            }
        }

        void AssetEditorTab::BeforePropertyModified(InstanceDataNode* node)
        {
            AssetEditorValidationRequestBus::Event(
                m_inMemoryAsset.Get()->GetId(), &AssetEditorValidationRequests::BeforePropertyEdit, node, m_inMemoryAsset);
        }

        void AssetEditorTab::ExpandAll()
        {
            if (!m_useDPE)
            {
                m_propertyEditor->ExpandAll();
            }
            else
            {
                m_dpe->ExpandAll();
            }
        }

        void AssetEditorTab::CollapseAll()
        {
            if (!m_useDPE)
            {
                m_propertyEditor->CollapseAll();
            }
            else
            {
                m_dpe->CollapseAll();
            }
        }

        void AssetEditorTab::DirtyAsset()
        {
            m_dirty = true;

            m_parentEditorWidget->UpdateTabTitle(this);
            m_parentEditorWidget->UpdateSaveMenuActionsStatus();

            SetStatusText(Status::emptyString);
        }

        void AssetEditorTab::ApplyStatusText(QString newText)
        {
            QString statusString =  QStringLiteral("%1").arg(m_currentAsset);

            if (!newText.isEmpty())
            {
                statusString.append(" - ");
                statusString.append(newText);
            }

            m_parentEditorWidget->SetStatusText(statusString);
        }

        void AssetEditorTab::SetupHeader()
        {
            AZStd::string assetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_sourceAssetId);

            QString headerText = QString("<b>%1</b>").arg(TextStrings::assetLocation);
            if (!assetPath.empty())
            {
                // Add the asset location to the right of the header. Location is relative to the Project directory.
                QString pathAndAsset = assetPath.c_str();
                int seperatorPos = pathAndAsset.lastIndexOf('/');
                if (seperatorPos >= 0)
                {
                    headerText.append(pathAndAsset.left(seperatorPos));
                }
            }
            else
            {
                headerText.append(TextStrings::unsaved);
            }

            m_header->setName(headerText);

            m_header->setIcon(QIcon(QStringLiteral(":/TreeView/open_small.svg")));
            m_header->show();
        }

        void AssetEditorTab::SetStatusText(const QString& assetStatus)
        {
            QMetaObject::invokeMethod(this, "ApplyStatusText", Qt::QueuedConnection, Q_ARG(QString, assetStatus));
        }

    } // namespace AssetEditor
} // namespace AzToolsFramework

#include <AssetEditor/moc_AssetEditorTab.cpp>
