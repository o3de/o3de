/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetEditorWidget.h"

#include <API/ToolsApplicationAPI.h>
#include <API/EditorAssetSystemAPI.h>

#include <AssetEditor/AssetEditorBus.h>
#include <AssetEditor/AssetEditorHeader.h>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <AzToolsFramework/AssetEditor/ui_AssetEditorToolbar.h>
AZ_POP_DISABLE_WARNING
#include <AzToolsFramework/AssetEditor/ui_AssetEditorStatusBar.h>

#include <AssetBrowser/AssetSelectionModel.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/sort.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <SourceControl/SourceControlAPI.h>

#include <UI/PropertyEditor/PropertyRowWidget.hxx>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <QVBoxLayout>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QAction>

namespace AzToolsFramework
{
    namespace AssetEditor
    {
        using AssetCheckoutCallback = AZStd::function<void(bool, const AZStd::string&, const AZStd::string&)>;

        void AssetCheckoutCommon(const AZ::Data::AssetId& id, AZ::Data::Asset<AZ::Data::AssetData> asset, [[maybe_unused]] AZ::SerializeContext* serializeContext, AssetCheckoutCallback assetCheckoutAndSaveCallback)
        {
            AZStd::string assetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, id);
            if (assetPath.empty())
            {
                AZ_Assert(!assetPath.empty(), "A valid path needs to be resolved, if this failed, the provided asset path is not a valid asset.");
                return;
            }

            AZStd::string assetFullPath;
            AssetSystemRequestBus::Broadcast(&AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath, assetPath, assetFullPath);

            if (!assetFullPath.empty())
            {
                using SCCommandBus = SourceControlCommandBus;
                SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, assetFullPath.c_str(), true,
                    [id, asset, assetFullPath, assetCheckoutAndSaveCallback](bool /*success*/, const SourceControlFileInfo& info)
                    {
                        if (!info.IsReadOnly())
                        {
                            AZ::Outcome<bool, AZStd::string> outcome = AZ::Success(true);
                            AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::EventResult(outcome, id, &AzToolsFramework::AssetEditor::AssetEditorValidationRequests::IsAssetDataValid, asset);
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
                            AZStd::string error = AZStd::string::format("Could not check out asset file: %s.", assetFullPath.c_str());
                            assetCheckoutAndSaveCallback(false, error, assetFullPath);
                        }
                    }
                    );
            }
            else
            {
                AZStd::string error = AZStd::string::format("Could not resolve path name for asset {%s}.", id.ToString<AZStd::string>().c_str());
                assetCheckoutAndSaveCallback(false, error, AZStd::string{});
            }
        }

        namespace Status
        {
            static QString assetCreated = QStringLiteral("Asset created!");
            static QString assetSaved = QStringLiteral("Asset saved!");
            static QString assetSaving = QStringLiteral("Asset saving!");
            static QString assetLoaded = QStringLiteral("Asset loaded!");
            static QString unableToSave = QStringLiteral("Failed to save asset due to validation error, check the log!");
            static QString emptyString = QStringLiteral("");
        }

        //////////////////////////////////
        // AssetEditorWidgetUserSettings
        //////////////////////////////////

        void AssetEditorWidgetUserSettings::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetEditorWidgetUserSettings>()
                    ->Version(0)
                    ->Field("m_showPreviewMessage", &AssetEditorWidgetUserSettings::m_lastSavePath)
                    ->Field("m_snapDistance", &AssetEditorWidgetUserSettings::m_recentPaths)
                    ;
            }
        }

        AssetEditorWidgetUserSettings::AssetEditorWidgetUserSettings()
        {
            char assetRoot[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@projectroot@", assetRoot, AZ_MAX_PATH_LEN);

            m_lastSavePath = assetRoot;
        }

        void AssetEditorWidgetUserSettings::AddRecentPath(const AZStd::string& recentPath)
        {
            AZStd::string lowerCasePath = recentPath;
            AZStd::to_lower(lowerCasePath.begin(), lowerCasePath.end());

            auto item = AZStd::find(m_recentPaths.begin(), m_recentPaths.end(), lowerCasePath);
            if (item != m_recentPaths.end())
            {
                m_recentPaths.erase(item);
            }

            m_recentPaths.emplace(m_recentPaths.begin(), lowerCasePath);

            while (m_recentPaths.size() > 10)
            {
                m_recentPaths.pop_back();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetEditorWidget
        //////////////////////////////////////////////////////////////////////////

        const AZ::Crc32 k_assetEditorWidgetSettings = AZ_CRC("AssetEditorSettings", 0xfe740dee);

        AssetEditorWidget::AssetEditorWidget(QWidget* parent)
            : QWidget(parent)
            , m_statusBar(new Ui::AssetEditorStatusBar())
            , m_dirty(true)
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            setObjectName("AssetEditorWidget");
            m_propertyEditor = new ReflectedPropertyEditor(this);
            m_propertyEditor->setObjectName("AssetEditorWidgetPropertyEditor");
            m_propertyEditor->Setup(m_serializeContext, this, true, 250);
            m_propertyEditor->show();

            QVBoxLayout* mainLayout = new QVBoxLayout();
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(0);

            m_header = new Ui::AssetEditorHeader(this);
            mainLayout->addWidget(m_header);
            m_header->show();

            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            mainLayout->addWidget(m_propertyEditor);

            QWidget* statusBarWidget = new QWidget(this);
            statusBarWidget->setObjectName("AssetEditorStatusBar");
            m_statusBar->setupUi(statusBarWidget);
            m_statusBar->textEdit->setText(Status::emptyString);
            m_statusBar->textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);

            mainLayout->addWidget(statusBarWidget);

            PopulateGenericAssetTypes();

            QMenuBar* mainMenu = new QMenuBar();
            QMenu* fileMenu = mainMenu->addMenu(tr("&File"));
            // Add Create New Asset menu and populate it with all asset types that have GenericAssetHandler
            QMenu* newAssetMenu = fileMenu->addMenu(tr("&New"));

            for (const auto& assetType : m_genericAssetTypes)
            {
                QString assetTypeName;
                AZ::AssetTypeInfoBus::EventResult(assetTypeName, assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

                if (!assetTypeName.isEmpty())
                {
                    QAction* newAssetAction = newAssetMenu->addAction(assetTypeName);
                    connect(newAssetAction, &QAction::triggered, this, [assetType, this]() { CreateAssetImpl(assetType); });
                }
            }

            QAction* openAssetAction = fileMenu->addAction("&Open");
            connect(openAssetAction, &QAction::triggered, this, &AssetEditorWidget::OpenAssetWithDialog);

            m_recentFileMenu = fileMenu->addMenu("Open Recent");

            m_saveAssetAction = fileMenu->addAction("&Save");
            m_saveAssetAction->setShortcut(QKeySequence::Save);
            connect(m_saveAssetAction, &QAction::triggered, this, &AssetEditorWidget::SaveAsset);

            m_saveAsAssetAction = fileMenu->addAction("&Save As");
            m_saveAsAssetAction->setShortcut(QKeySequence::SaveAs);
            connect(m_saveAsAssetAction, &QAction::triggered, this, &AssetEditorWidget::SaveAssetAs);

            // "Save" and "Save As..." actions are disabled by default,
            // and they are activated when an asset is created/open
            m_saveAssetAction->setEnabled(false);
            m_saveAsAssetAction->setEnabled(false);

            QMenu* viewMenu = mainMenu->addMenu(tr("&View"));

            QAction* expandAll = viewMenu->addAction("&Expand All");
            connect(expandAll, &QAction::triggered, this, &AssetEditorWidget::ExpandAll);

            QAction* collapseAll = viewMenu->addAction("&Collapse All");
            connect(collapseAll, &QAction::triggered, this, &AssetEditorWidget::CollapseAll);

            mainLayout->setMenuBar(mainMenu);

            setLayout(mainLayout);

            AzFramework::AssetCatalogEventBus::Handler::BusConnect();

            m_userSettings = AZ::UserSettings::CreateFind<AssetEditorWidgetUserSettings>(k_assetEditorWidgetSettings, AZ::UserSettings::CT_LOCAL);

            UpdateRecentFileListState();

            QObject::connect(m_recentFileMenu, &QMenu::aboutToShow, this, &AssetEditorWidget::PopulateRecentMenu);
        }

        AssetEditorWidget::~AssetEditorWidget()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
        }

        void AssetEditorWidget::CreateAsset(AZ::Data::AssetType assetType)
        {
            auto typeIter = AZStd::find_if(m_genericAssetTypes.begin(), m_genericAssetTypes.end(), [assetType](const AZ::Data::AssetType& testType) { return assetType == testType; });

            if (typeIter != m_genericAssetTypes.end())
            {
                CreateAssetImpl(assetType);
            }
            else
            {
                AZ_Assert(false, "The AssetEditorWidget only supports Generic Asset Types, make sure your type has a handler that implements GenericAssetHandler");
            }
        }

        void AssetEditorWidget::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_dirty = false;

            AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

            // Clone the asset
            AZ::Data::AssetId newAssetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
            m_inMemoryAsset = AZ::Data::AssetManager::Instance().CreateAsset(newAssetId, asset.GetType(), m_inMemoryAsset.GetAutoLoadBehavior());
            auto serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
            serializeContext->CloneObjectInplace((*m_inMemoryAsset.GetData()), asset.GetData());

            UpdatePropertyEditor(m_inMemoryAsset);

            SetupHeader();

            if (!m_sourceAssetId.IsValid())
            {
                SetStatusText(Status::assetCreated);
            }
            else
            {
                SetStatusText(Status::assetLoaded);
            }

            UpdateMenusOnAssetOpen();
        }

        void AssetEditorWidget::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            // Keep a reference to the reloaded asset data
            OnAssetReady(asset);
        }

        void AssetEditorWidget::UpdatePropertyEditor(AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            m_propertyEditor->ClearInstances();

            AZ::Crc32 saveStateKey;
            saveStateKey.Add(&asset.GetId(), sizeof(AZ::Data::AssetId));

            m_propertyEditor->SetSavedStateKey(saveStateKey);
            m_propertyEditor->AddInstance(asset.Get(), asset.GetType(), nullptr);

            m_propertyEditor->InvalidateAll();
            m_propertyEditor->setEnabled(true);

        }

        void AssetEditorWidget::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_dirty = false;
            m_saveAssetAction->setEnabled(false);
            m_propertyEditor->ClearInstances();

            if (AZ::Data::AssetBus::MultiHandler::BusIsConnectedId(asset.GetId()))
            {
                AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
            }
            QString errString = tr("Failed to load %1!").arg(asset.GetHint().c_str());
            AZ_Error("Asset Editor", false, errString.toUtf8());
            QMessageBox::warning(this, tr("Error!"), errString);
        }

        bool AssetEditorWidget::TrySave(const AZStd::function<void()>& savedCallback)
        {
            if (!m_sourceAssetId.IsValid() || !m_dirty)
            {
                return false;
            }

            const int result = QMessageBox::question(this,
                    tr("Save Changes?"),
                    tr("Changes have been made to the asset during this session. Would you like to save prior to closing?"),
                    QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);

            if (result == QMessageBox::No)
            {
                return false;
            }
            if (result == QMessageBox::Yes)
            {
                if (savedCallback)
                {
                    auto conn = AZStd::make_shared<QMetaObject::Connection>();
                    *conn = connect(this, &AssetEditorWidget::OnAssetSavedSignal, this, [conn, savedCallback]()
                            {
                                disconnect(*conn);
                                savedCallback();
                            });
                }
                SaveAsset();
            }
            return true;
        }

        bool AssetEditorWidget::WaitingToSave() const
        {
            return m_waitingToSave;
        }

        void AssetEditorWidget::SetCloseAfterSave()
        {
            m_closeAfterSave = true;
        }

        bool AssetEditorWidget::SaveAsDialog(AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            AZStd::vector<AZStd::string> assetTypeExtensions;
            AZ::AssetTypeInfoBus::Event(asset.GetType(), &AZ::AssetTypeInfo::GetAssetTypeExtensions, assetTypeExtensions);
            QString filter;
            if (assetTypeExtensions.empty())
            {
                filter = tr("All Files (*.*)");
            }
            else
            {
                QString assetTypeName;
                AZ::AssetTypeInfoBus::EventResult(assetTypeName, asset.GetType(), &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
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
                        filter.append(" ");
                    }
                }
                filter.append(")");
            }

            const QString saveAs = AzQtComponents::FileDialog::GetSaveFileName(AzToolsFramework::GetActiveWindow(), tr("Save As..."), m_userSettings->m_lastSavePath.c_str(), filter);

            return SaveImpl(asset, saveAs);
        }

        bool AssetEditorWidget::SaveAssetToPath(AZStd::string_view assetPath)
        {
            return SaveImpl(m_inMemoryAsset, assetPath.data());
        }

        bool AssetEditorWidget::SaveImpl(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const QString& saveAsPath)
        {
            if (!saveAsPath.isEmpty())
            {
                AZStd::string targetFilePath(saveAsPath.toUtf8().constData());

                m_propertyEditor->ForceQueuedInvalidation();

                AZStd::vector<char> byteBuffer;
                AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

                auto assetHandler = const_cast<AZ::Data::AssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(asset.GetType()));
                if (assetHandler->SaveAssetData(asset, &byteStream))
                {
                    AZ::IO::FileIOStream fileStream(targetFilePath.c_str(), AZ::IO::OpenMode::ModeWrite);
                    if (!fileStream.IsOpen())
                    {
                        AZ_Warning("Asset Editor Widget", false, "Could not open file for writing - invalid path (%s).", targetFilePath.c_str());
                        return false;
                    }

                    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePathKeepCase, targetFilePath);
                    m_expectedAddedAssetPath = targetFilePath;

                    SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestEdit, targetFilePath.c_str(), true, [](bool, const SourceControlFileInfo&) {});

                    fileStream.Write(byteBuffer.size(), byteBuffer.data());

                    AZStd::string watchFolder;
                    AZ::Data::AssetInfo assetInfo;
                    bool sourceInfoFound{};
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, targetFilePath.c_str(), assetInfo, watchFolder);

                    if (sourceInfoFound)
                    {
                        m_sourceAssetId = assetInfo.m_assetId;
                    }

                    AZStd::string fileName = targetFilePath;

                    if (AzFramework::StringFunc::Path::Normalize(fileName))
                    {
                        AzFramework::StringFunc::Path::StripPath(fileName);
                        m_currentAsset = fileName.c_str();

                        m_userSettings->m_lastSavePath = targetFilePath;
                        AZStd::size_t findIndex = m_userSettings->m_lastSavePath.find(fileName.c_str());

                        if (findIndex != AZStd::string::npos)
                        {
                            m_userSettings->m_lastSavePath = m_userSettings->m_lastSavePath.substr(0, findIndex);
                        }
                    }

                    m_dirty = false;

                    AddRecentPath(targetFilePath);

                    SetStatusText(Status::assetCreated);

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

        void AssetEditorWidget::OpenAsset(const AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            // If we're already editing this asset, don't do anything.
            if (m_sourceAssetId == asset.GetId())
            {
                return;
            }

            // If an unsaved asset is open, ask.
            if (TrySave([this]() { OpenAssetWithDialog(); }))
            {
                return;
            }

            AZ::Data::AssetInfo typeInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(typeInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, asset.GetId());

            // Check if there is an asset to open
            if (typeInfo.m_relativePath.empty())
            {
                return;
            }

            bool hasResult = false;
            AZStd::string fullPath;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(hasResult, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath, typeInfo.m_relativePath, fullPath);

            AZStd::string fileName = typeInfo.m_relativePath;

            if (AzFramework::StringFunc::Path::Normalize(fileName))
            {
                AzFramework::StringFunc::Path::StripPath(fileName);
                m_currentAsset = fileName.c_str();
            }

            AddRecentPath(fullPath.c_str());

            m_sourceAssetId = asset.GetId();

            LoadAsset(asset.GetId(), asset.GetType());
        }

        void AssetEditorWidget::OpenAssetWithDialog()
        {
            // If an unsaved asset is open, ask.
            if (TrySave([this]() { OpenAssetWithDialog(); }))
            {
                return;
            }

            AssetSelectionModel selection = AssetSelectionModel::AssetTypesSelection(m_genericAssetTypes);
            EditorRequests::Bus::Broadcast(&EditorRequests::BrowseForAssets, selection);
            if (selection.IsValid())
            {
                auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
                m_currentAsset = product->GetName().c_str();

                AddRecentPath(product->GetFullPath());

                LoadAsset(product->GetAssetId(), product->GetAssetType());
                m_sourceAssetId = product->GetAssetId();
            }
        }

        void AssetEditorWidget::OpenAssetFromPath(const AZStd::string& assetPath)
        {
            bool hasResult = false;
            AZ::Data::AssetInfo assetInfo;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(hasResult, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, assetPath.c_str(), assetInfo, watchFolder);

            if (hasResult)
            {
                AZStd::string fileName = assetPath;

                if (AzFramework::StringFunc::Path::Normalize(fileName))
                {
                    AzFramework::StringFunc::Path::StripPath(fileName);
                    m_currentAsset = fileName.c_str();
                }

                AZ::Data::AssetInfo typeInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(typeInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetInfo.m_assetId);

                LoadAsset(assetInfo.m_assetId, typeInfo.m_assetType);

                m_sourceAssetId = assetInfo.m_assetId;
            }
        }

        void AssetEditorWidget::LoadAsset(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType)
        {
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
                m_propertyEditor->setEnabled(false);
            }
        }

        void AssetEditorWidget::GenerateSaveDataSnapshot()
        {
            //Generate data now so that if there's a wait for the source control system, the data saved will be as it was when the save was called.
            AZStd::vector<AZ::u8> newSaveData;

            // Make a stream and save a snapshot of the asset to it.
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > dstByteStream(&newSaveData);

            AssetEditorValidationRequestBus::Event(m_sourceAssetId, &AssetEditorValidationRequests::PreAssetSave, m_inMemoryAsset);

            if (AZ::Utils::SaveObjectToStream(
                    dstByteStream, AZ::DataStream::ST_XML, m_inMemoryAsset.Get(), m_inMemoryAsset.Get()->RTTI_GetType(),
                    m_serializeContext))
            {
                AZStd::swap(newSaveData, m_saveData);
            }
        }

        void AssetEditorWidget::SaveAsset()
        {
            if (!m_sourceAssetId.IsValid())
            {
                SaveAsDialog(m_inMemoryAsset);
            }
            else if (m_dirty)
            {
                SetStatusText(Status::assetSaving);

                GenerateSaveDataSnapshot();

                // Clear the dirty flag now so that attempting to close the window during a save doesn't ask the user to save again, 
                // unless there are further changes.
                m_dirty = false;
                m_saveAssetAction->setEnabled(false);
                m_saveAsAssetAction->setEnabled(false);

                if (WaitingToSave())
                {
                    // Don't need to do the save, as we've just overwritten the data that the previously queued save will write out.
                    return;
                }
                m_waitingToSave = true;
                AssetCheckoutCommon(m_sourceAssetId, m_inMemoryAsset, m_serializeContext,
                [this](bool checkoutSuccess, const AZStd::string& error, const AZStd::string& assetFullPath)
                {
                    AZStd::string saveError = error;
                    bool saveSuccessful = false;

                    if (checkoutSuccess)
                    {
                        saveError = AZStd::string::format("Could not write asset file: %s.", assetFullPath.c_str());
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
                        m_propertyEditor->QueueInvalidation(Refresh_AttributesAndValues);
                        SetStatusText(Status::assetSaved);

                        if (m_closeAfterSave)
                        {
                            m_closeAfterSave = false;
                            parentWidget()->parentWidget()->close();
                        }
                    }
                    else
                    {
                        // Don't want to dirty the asset here, since we are setting ourselves into a weird state since we were unable to save.
                        m_dirty = true;
                        m_saveAssetAction->setEnabled(false);
                        m_saveAsAssetAction->setEnabled(false);

                        SetStatusText(Status::unableToSave);
                        Q_EMIT OnAssetSaveFailedSignal(saveError);
                    }

                    m_waitingToSave = false;
                }
                );
            }
        }

        void AssetEditorWidget::SaveAssetAs()
        {
            SaveAsDialog(m_inMemoryAsset);
        }

        void AssetEditorWidget::OnNewAsset()
        {
            // This only works if we've already made a new asset or opened an asset of a given type
            // We use that knowledge to create another asset of the same type.
            if (!m_inMemoryAsset.GetType().IsNull())
            {
                CreateAssetImpl(m_inMemoryAsset.GetType());
            }
        }

        void AssetEditorWidget::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
            if (assetInfo.m_assetType == m_inMemoryAsset.GetType()
                && strstr(m_expectedAddedAssetPath.c_str(), assetInfo.m_relativePath.c_str()) != nullptr)
            {
                m_expectedAddedAssetPath.clear();
                m_recentlyAddedAssetPath = assetInfo.m_relativePath;

                LoadAsset(assetId, assetInfo.m_assetType);

                m_sourceAssetId = assetId;
            }
        }

        void AssetEditorWidget::OnCatalogAssetRemoved(const AZ::Data::AssetId& /*assetId*/, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType == m_inMemoryAsset.GetType()
                && assetInfo.m_relativePath.compare(m_recentlyAddedAssetPath) == 0)
            {
                m_recentlyAddedAssetPath.clear();

                // The file was removed due to errors, but the user needs to be 
                // given a chance to fix the errors and try again.
                m_sourceAssetId.SetInvalid();
                m_currentAsset = "New Asset";
                m_propertyEditor->setEnabled(true);

                DirtyAsset();
            }
        }

        void AssetEditorWidget::PopulateGenericAssetTypes()
        {
            auto enumerateCallback =
                [this](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& /*typeId*/) -> bool
                {
                    AZ::Data::AssetType assetType = classData->m_typeId;

                    // make sure this is not base class                    
                    if (assetType == AZ::AzTypeInfo<AZ::Data::AssetData>::Uuid()
                        || AZ::FindAttribute(AZ::Edit::Attributes::EnableForAssetEditor, classData->m_attributes) == nullptr)
                    {
                        return true;
                    }

                    // narrow down all reflected AssetTypeInfos to those that have a GenericAssetHandler
                    AZ::Data::AssetManager& manager = AZ::Data::AssetManager::Instance();
                    if (const AZ::Data::AssetHandler* assetHandler = manager.GetHandler(classData->m_typeId))
                    {
                        if (!azrtti_istypeof<AzFramework::GenericAssetHandlerBase>(assetHandler))
                        {
                            // its not the generic asset handler.
                            return true;
                        }
                    }

                    m_genericAssetTypes.push_back(assetType);
                    return true;
                };

            m_serializeContext->EnumerateDerived(
                enumerateCallback,
                AZ::Uuid::CreateNull(),
                AZ::AzTypeInfo<AZ::Data::AssetData>::Uuid()
                );

            AZStd::sort(m_genericAssetTypes.begin(), m_genericAssetTypes.end(), [&](const AZ::Data::AssetType& lhsAssetType, const AZ::Data::AssetType& rhsAssetType)
                {
                    AZStd::string lhsAssetTypeName = "";
                    AZ::AssetTypeInfoBus::EventResult(lhsAssetTypeName, lhsAssetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
                    AZStd::string rhsAssetTypeName = "";
                    AZ::AssetTypeInfoBus::EventResult(rhsAssetTypeName, rhsAssetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
                    return lhsAssetTypeName < rhsAssetTypeName;
                });
        }

        void AssetEditorWidget::CreateAssetImpl(AZ::Data::AssetType assetType)
        {
            // If an unsaved asset is open, ask.
            if (TrySave([this, assetType]() { CreateAssetImpl(assetType); }))
            {
                return;
            }

            if (m_inMemoryAsset)
            {
                AZ::Data::AssetBus::MultiHandler::BusDisconnect(m_inMemoryAsset.GetId());
                m_inMemoryAsset.Release();
            }

            m_sourceAssetId.SetInvalid();

            AZ::Data::AssetId newAssetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
            m_inMemoryAsset = AZ::Data::AssetManager::Instance().CreateAsset(newAssetId, assetType, AZ::Data::AssetLoadBehavior::Default);

            DirtyAsset();

            m_propertyEditor->ClearInstances();
            m_currentAsset = "New Asset";

            UpdatePropertyEditor(m_inMemoryAsset);

            ExpandAll();
            SetStatusText(Status::assetCreated);
            SetupHeader();
        }

        void AssetEditorWidget::AfterPropertyModified(InstanceDataNode* /*node*/)
        {
            DirtyAsset();
        }

        void AssetEditorWidget::RequestPropertyContextMenu(InstanceDataNode* node, const QPoint& point)
        {
            if (node && node->IsDifferentVersusComparison())
            {
                QMenu menu;
                QAction* resetAction = menu.addAction(tr("Reset value"));
                connect(resetAction, &QAction::triggered, this,
                    [this, node]()
                    {
                        const InstanceDataNode* orig = node->GetComparisonNode();
                        if (orig)
                        {
                            InstanceDataHierarchy::CopyInstanceData(orig, node, m_serializeContext);

                            PropertyRowWidget* widget = m_propertyEditor->GetWidgetFromNode(node);
                            if (widget)
                            {
                                widget->DoPropertyNotify();
                            }

                            m_propertyEditor->QueueInvalidation(Refresh_Values);
                        }
                    }
                    );
                menu.exec(point);
            }
        }

        void AssetEditorWidget::BeforePropertyModified(InstanceDataNode* node)
        {
            AssetEditorValidationRequestBus::Event(m_inMemoryAsset.Get()->GetId(), &AssetEditorValidationRequests::BeforePropertyEdit, node, m_inMemoryAsset);
        }

        void AssetEditorWidget::ExpandAll()
        {
            m_propertyEditor->ExpandAll();
        }

        void AssetEditorWidget::CollapseAll()
        {
            m_propertyEditor->CollapseAll();
        }

        void AssetEditorWidget::DirtyAsset()
        {
            m_dirty = true;
            m_saveAssetAction->setEnabled(true);
            m_saveAsAssetAction->setEnabled(true);

            SetStatusText(Status::emptyString);
        }

        void AssetEditorWidget::OnSystemTick()
        {
            if (!m_queuedAssetStatus.isEmpty())
            {
                ApplyStatusText();
            }
        }

        void AssetEditorWidget::ApplyStatusText()
        {
            QString statusString;

            if (m_dirty)
            {
                statusString = QString("%1*");
            }
            else
            {
                statusString = QString("%1");
            }

            statusString = statusString.arg(m_currentAsset);

            if (!m_queuedAssetStatus.isEmpty())
            {
                statusString.append(" - ");
                statusString.append(m_queuedAssetStatus);
            }

            m_statusBar->textEdit->setText(statusString);

            m_queuedAssetStatus.clear();

            AZ::SystemTickBus::Handler::BusDisconnect();

        }

        void AssetEditorWidget::SetupHeader()
        {            
            QString nameString = QString("%1").arg(m_currentAsset);

            m_header->setName(nameString);

            AZStd::string assetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_sourceAssetId);

            // Add the asset location to the right of the header. Location is relative to the Project directory.
            QString pathAndAsset = assetPath.c_str();
            int seperatorPos = pathAndAsset.lastIndexOf('/');
            if (seperatorPos < 0)
            {
                m_header->setLocation("");
            }
            else
            {
                m_header->setLocation(pathAndAsset.left(seperatorPos));
            }
            
            m_header->setIcon(QIcon(QStringLiteral(":/AssetEditor/default_document.svg")));
            m_header->show();
        }

        void AssetEditorWidget::SetStatusText(const QString& assetStatus)
        {
            m_queuedAssetStatus = assetStatus;
            AZ::SystemTickBus::Handler::BusConnect();
        }

        void AssetEditorWidget::AddRecentPath(const AZStd::string& recentPath)
        {
            m_userSettings->AddRecentPath(recentPath);
            UpdateRecentFileListState();
        }

        void AssetEditorWidget::PopulateRecentMenu()
        {
            m_recentFileMenu->clear();

            if (m_userSettings->m_recentPaths.empty())
            {
                m_recentFileMenu->setEnabled(false);
            }
            else
            {
                m_recentFileMenu->setEnabled(true);

                for (const AZStd::string& recentFile : m_userSettings->m_recentPaths)
                {
                    bool hasResult = false;
                    AZStd::string relativePath;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(hasResult, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, recentFile, relativePath);

                    if (hasResult)
                    {
                        QAction* action = m_recentFileMenu->addAction(relativePath.c_str());
                        connect(action, &QAction::triggered, this, [recentFile, this]() { this->OpenAssetFromPath(recentFile); });
                    }
                }
            }
        }

        void AssetEditorWidget::UpdateMenusOnAssetOpen()
        {
            // Activate "Save" and "Save As..." actions
            m_saveAssetAction->setEnabled(true);
            m_saveAsAssetAction->setEnabled(true);
        }

        void AssetEditorWidget::UpdateRecentFileListState()
        {
            if (m_recentFileMenu)
            {
                if (!m_userSettings || m_userSettings->m_recentPaths.empty())
                {
                    m_recentFileMenu->setEnabled(false);
                }
                else
                {
                    m_recentFileMenu->setEnabled(true);
                }
            }
        }

    } // namespace AssetEditor
} // namespace AzToolsFramework

#include "AssetEditor/moc_AssetEditorWidget.cpp"
