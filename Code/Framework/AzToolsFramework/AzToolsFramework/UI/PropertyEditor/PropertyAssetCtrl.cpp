/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "PropertyAssetCtrl.hxx"

#include "PropertyQTConstants.h"

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtGui/QPixmapCache>
#include <QtGui/QMouseEvent>
#include <QtCore/QMimeData>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QClipboard>
#include <QListView>
#include <QTableView>
#include <QHeaderView>
#include <QPainter>
#include <QPixmap>
#include <QByteArray>
#include <QDataStream>
AZ_POP_DISABLE_WARNING

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzToolsFramework/ToolsComponents/EditorAssetMimeDataContainer.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/Logging/GenericLogPanel.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Thumbnails/ThumbnailWidget.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>

#include <UI/PropertyEditor/Model/AssetCompleterModel.h>
#include <UI/PropertyEditor/View/AssetCompleterListView.h>
#include <UI/PropertyEditor/ThumbnailPropertyCtrl.h>

namespace AzToolsFramework
{
    /* ----- PropertyAssetCtrl ----- */

    PropertyAssetCtrl::PropertyAssetCtrl(QWidget* pParent, QString optionalValidDragDropExtensions)
        : QWidget(pParent)
        , m_optionalValidDragDropExtensions(optionalValidDragDropExtensions)
    {
        QHBoxLayout* pLayout = new QHBoxLayout();
        m_browseEdit = new AzQtComponents::BrowseEdit(this);
        m_browseEdit->lineEdit()->setFocusPolicy(Qt::StrongFocus);
        m_browseEdit->lineEdit()->installEventFilter(this);
        m_browseEdit->setClearButtonEnabled(true);
        m_browseEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QToolButton* clearButton = AzQtComponents::LineEdit::getClearButton(m_browseEdit->lineEdit());
        assert(clearButton);
        connect(clearButton, &QToolButton::clicked, this, &PropertyAssetCtrl::OnClearButtonClicked);

        connect(m_browseEdit->lineEdit(), &QLineEdit::textEdited, this, &PropertyAssetCtrl::OnTextChange);
        connect(m_browseEdit->lineEdit(), &QLineEdit::returnPressed, this, &PropertyAssetCtrl::OnReturnPressed);

        connect(m_browseEdit, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &PropertyAssetCtrl::PopupAssetPicker);

        pLayout->setContentsMargins(0, 0, 0, 0);
        pLayout->setSpacing(2);

        m_browseEdit->lineEdit()->setAcceptDrops(false);
        setAcceptDrops(true);

        m_thumbnail = new ThumbnailPropertyCtrl(this);
        m_thumbnail->setFixedSize(QSize(40, 24));
        m_thumbnail->setVisible(false);

        connect(m_thumbnail, &ThumbnailPropertyCtrl::clicked, this, &PropertyAssetCtrl::OnThumbnailClicked);

        m_editButton = new QToolButton(this);
        m_editButton->setAutoRaise(true);
        m_editButton->setIcon(QIcon(":/stylesheet/img/UI20/open-in-internal-app.svg"));
        m_editButton->setToolTip("Edit asset");
        SetEditButtonVisible(false);

        connect(m_editButton, &QToolButton::clicked, this, &PropertyAssetCtrl::OnEditButtonClicked);

        pLayout->addWidget(m_thumbnail);
        pLayout->addWidget(m_browseEdit);
        pLayout->addWidget(m_editButton);

        setLayout(pLayout);

        setFocusProxy(m_browseEdit->lineEdit());
        setFocusPolicy(m_browseEdit->lineEdit()->focusPolicy());

        m_currentAssetType = AZ::Data::s_invalidAssetType;

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(ShowContextMenu(const QPoint&)));
    }

    void PropertyAssetCtrl::ConfigureAutocompleter()
    {
        if (m_completerIsConfigured)
        {
            return;
        }
        m_completerIsConfigured = true;

        m_model = aznew AssetCompleterModel(this);

        m_completer = new QCompleter(m_model, this);
        m_completer->setMaxVisibleItems(20);
        m_completer->setCompletionColumn(0);
        m_completer->setCompletionRole(Qt::DisplayRole);
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setFilterMode(Qt::MatchContains);

        connect(m_completer->completionModel(), &QAbstractItemModel::modelReset, this, &PropertyAssetCtrl::OnCompletionModelReset);
        connect(m_completer, static_cast<void (QCompleter::*)(const QModelIndex& index)>(&QCompleter::activated), this, &PropertyAssetCtrl::OnAutocomplete);

        m_view = new AssetCompleterListView(this);
        m_completer->setPopup(m_view);

        m_view->setModelColumn(1);

        m_view->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
        m_view->setSelectionBehavior(QAbstractItemView::SelectItems);

        m_model->SetFilter(GetSelectableAssetTypes());
    }

    void PropertyAssetCtrl::RefreshAutocompleter()
    {
        m_model->RefreshAssetList();
    }

    void PropertyAssetCtrl::EnableAutocompleter()
    {
        m_completerIsActive = true;
        m_browseEdit->lineEdit()->setCompleter(m_completer);
    }

    void PropertyAssetCtrl::DisableAutocompleter()
    {
        m_completerIsActive = false;
        m_browseEdit->lineEdit()->setCompleter(nullptr);
    }

    void PropertyAssetCtrl::HandleFieldClear()
    {
        if (m_allowEmptyValue)
        {
            SetSelectedAssetID(AZ::Data::AssetId());
        }
        else
        {
            SetSelectedAssetID(GetCurrentAssetID());
        }
    }

    void PropertyAssetCtrl::OnThumbnailClicked()
    {
        const AZ::Data::AssetId assetID = GetCurrentAssetID();
        if (m_thumbnailCallback)
        {
            AZ_Error("Asset Property", m_editNotifyTarget, "No notification target set for edit callback.");
            m_thumbnailCallback->Invoke(m_editNotifyTarget, assetID, GetCurrentAssetType());
            return;
        }
    }

    void PropertyAssetCtrl::OnCompletionModelReset()
    {
        if (!m_completerIsActive)
        {
            return;
        }

        // Update the minimum width of the popup to fit all strings
        int marginWidth = m_view->width() - m_view->viewport()->width();
        int frameWidth = 2 * m_view->frameWidth();
        int maxStringWidth = 0;
        for (int i = 0; m_completer->setCurrentRow(i); ++i)
        {
            QString currentCompletion = m_completer->currentCompletion();
            int stringWidth = m_view->fontMetrics().boundingRect(currentCompletion).width();
            if (stringWidth > maxStringWidth)
            {
                maxStringWidth = stringWidth;
            }
        }
        m_view->setMinimumWidth(marginWidth + frameWidth + maxStringWidth);
    }

    void PropertyAssetCtrl::OnAutocomplete(const QModelIndex& index)
    {
        SetSelectedAssetID(m_model->GetAssetIdFromIndex(GetSourceIndex(index)));
    }

    void PropertyAssetCtrl::OnReturnPressed()
    {
        if (m_completerIsActive)
        {
            m_view->SelectFirstItem();

            const QModelIndex selectedindex = m_view->currentIndex();
            if (selectedindex.isValid())
            {
                OnAutocomplete(selectedindex);
            }
            else
            {
                HandleFieldClear();
            }
        }
        else
        {
            HandleFieldClear();
        }

        m_browseEdit->lineEdit()->clearFocus();
    }

    void PropertyAssetCtrl::OnTextChange(const QString& text)
    {
        // Triggered when text in textEdit is deliberately changed by the user

        // 0 - Save position of cursor on lineEdit
        m_lineEditLastCursorPosition = m_browseEdit->lineEdit()->cursorPosition();

        // 1 - If the model for this field still hasn't been configured, do so.
        if (!m_completerIsConfigured)
        {
            ConfigureAutocompleter();
        }

        // 2a - Detect number of keys in lineEdit; if more than m_autocompleteAfterNumberOfChars, activate autocompleter
        int chars = text.size();
        if (chars >= s_autocompleteAfterNumberOfChars && !m_completerIsActive)
        {
            EnableAutocompleter();
        }
        // 2b - If less than m_autocompleteAfterNumberOfChars, deactivate autocompleter
        else if (chars < s_autocompleteAfterNumberOfChars && m_completerIsActive)
        {
            DisableAutocompleter();
        }

        // 3 - If completer is active, pass search string to its model to highlight the results
        if (m_completerIsActive)
        {
            m_model->SearchStringHighlight(text);
        }

        // 4 - Whenever the string is altered  mark the filename as incomplete.
        //     Asset is only legally selected by going through the Asset Browser Popup or via the Autocomplete.
        m_incompleteFilename = true;

        // 5 - If lineEdit isn't manually set to text, first alteration of the text isn't registered correctly. We're also restoring cursor position.
        m_browseEdit->setText(text);
        m_browseEdit->lineEdit()->setCursorPosition(m_lineEditLastCursorPosition);
    }

    void PropertyAssetCtrl::ShowContextMenu(const QPoint& pos)
    {
        QClipboard* clipboard = QApplication::clipboard();
        if (!clipboard)
        {
            // Can't do anything without a clipboard, so just return
            return;
        }

        QPoint globalPos = mapToGlobal(pos);

        QMenu myMenu;

        QAction* copyAction = myMenu.addAction(tr("Copy asset reference"));
        QAction* pasteAction = myMenu.addAction(tr("Paste asset reference"));

        copyAction->setEnabled(GetCurrentAssetID().IsValid());

        bool canPasteFromClipboard = false;

        // Do we have stuff on the clipboard?
        const QMimeData* mimeData = clipboard->mimeData();
        AZ::Data::AssetId readId;

        if (mimeData && mimeData->hasFormat(EditorAssetMimeDataContainer::GetMimeType()))
        {
            AZ::Data::AssetType readType;
            // This verifies that the mime data matches any restrictions for this particular asset property
            if (IsCorrectMimeData(mimeData, &readId, &readType))
            {
                if (readId.IsValid())
                {
                    canPasteFromClipboard = true;
                }
            }
        }

        pasteAction->setEnabled(canPasteFromClipboard);

        QAction* selectedItem = myMenu.exec(globalPos);
        if (selectedItem == copyAction)
        {
            QMimeData* newMimeData = new QMimeData();

            AzToolsFramework::EditorAssetMimeDataContainer mimeDataContainer;
            mimeDataContainer.AddEditorAsset(GetCurrentAssetID(), GetCurrentAssetType());
            mimeDataContainer.AddToMimeData(newMimeData);

            clipboard->setMimeData(newMimeData);
        }
        else if (selectedItem == pasteAction)
        {
            if (canPasteFromClipboard)
            {
                SetSelectedAssetID(readId);
            }
        }
    }

    bool PropertyAssetCtrl::CanAcceptAsset(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) const
    {
        const auto selectableAssetTypes = GetSelectableAssetTypes();
        const auto isSelectableAssetType =
            AZStd::find(selectableAssetTypes.begin(), selectableAssetTypes.end(), assetType) != selectableAssetTypes.end();
        return (assetId.IsValid() && !assetType.IsNull() && isSelectableAssetType);
    }

    bool PropertyAssetCtrl::IsCorrectMimeData(const QMimeData* pData, AZ::Data::AssetId* pAssetId, AZ::Data::AssetType* pAssetType) const
    {
        if (pAssetId)
        {
            pAssetId->SetInvalid();
        }

        if (pAssetType)
        {
            (*pAssetType) = AZ::Data::AssetType{};
        }

        if (!pData)
        {
            return false;
        }

        // Helper function to consistently check and set asset ID and type for all possible mime types
        auto checkAsset = [&, this](const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType)
        {
            if (CanAcceptAsset(assetId, assetType))
            {
                if (pAssetId)
                {
                    (*pAssetId) = assetId;
                }

                if (pAssetType)
                {
                    (*pAssetType) = assetType;
                }

                return true;
            }

            return false;
        };

        // Helper function to compare against acceptable file extensions if they are provided
        auto checkExtension = [this](const QString& path)
        {
            if (!m_optionalValidDragDropExtensions.isEmpty())
            {
                const int dotIndex = path.lastIndexOf('.');
                if (dotIndex >= 0)
                {
                    const QString& extension = path.mid(dotIndex);
                    return m_optionalValidDragDropExtensions.indexOf(extension) >= 0;
                }

                return false;
            }

            return true;
        };

        if (pData->hasFormat(EditorAssetMimeDataContainer::GetMimeType()))
        {
            AzToolsFramework::EditorAssetMimeDataContainer cont;
            if (cont.FromMimeData(pData))
            {
                // Searching the source data container for a compatible asset
                for (const auto& asset : cont.m_assets)
                {
                    if (checkAsset(asset.m_assetId, asset.m_assetType))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        if (pData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            AZStd::vector<const AssetBrowser::AssetBrowserEntry*> entries;
            if (AssetBrowser::Utils::FromMimeData(pData, entries))
            {
                // Search all of the entries for a compatible product asset
                for (const auto entry : entries)
                {
                    if (entry->GetEntryType() == AssetBrowser::AssetBrowserEntry::AssetEntryType::Product ||
                        entry->GetEntryType() == AssetBrowser::AssetBrowserEntry::AssetEntryType::Source)
                    {
                        // Support selecting and assigning source and product assets. If the entry is a matching product asset, it will be
                        // assigned immediately. If it is a source asset entry, enumerate all of the children and assign the first
                        // compatible product asset.
                        bool result = false;
                        entry->VisitDown(
                            [&](const auto& currentEntry)
                            {
                                if (!result)
                                {
                                    if (const auto product = azrtti_cast<const AssetBrowser::ProductAssetBrowserEntry*>(currentEntry))
                                    {
                                        result = checkAsset(product->GetAssetId(), product->GetAssetType());
                                    }
                                }
                                return !result;
                            });

                        if (result)
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        // If the mime data was dragged from explorer or an external source it will be represented as URLs or file paths.
        // This is searching all of the contained, local URLs for compatible asset paths.  These files must refer to a
        // valid source or product asset that can be identified using the asset system or catalog.
        for (const auto& url : pData->urls())
        {
            if (url.isLocalFile())
            {
                const AZStd::string fullPath = url.toLocalFile().toUtf8().constData();

                // Determine if this is an exact match product asset first to skip additional processing
                AZ::Data::AssetId assetId;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId,
                    &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                    fullPath.c_str(), AZ::Data::s_invalidAssetType, false);

                if (assetId.IsValid())
                {
                    AZ::Data::AssetInfo assetInfo;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo,
                        &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);

                    if (checkExtension(assetInfo.m_relativePath.c_str()))
                    {
                        if (checkAsset(assetInfo.m_assetId, assetInfo.m_assetType))
                        {
                            return true;
                        }
                    }
                }
                else
                {
                    // If this file could not be identified as a product asset then check if it is a source asset
                    bool sourceAssetInfoFound = false;
                    AZ::Data::AssetInfo sourceAssetInfo;
                    AZStd::string watchFolder;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceAssetInfoFound,
                        &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
                        fullPath.c_str(), sourceAssetInfo, watchFolder);

                    if (sourceAssetInfoFound)
                    {
                        // Search all of the products generated by the source asset for a compatible asset
                        AZStd::vector<AZ::Data::AssetInfo> productsAssetInfo;
                        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceAssetInfoFound,
                            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetsProducedBySourceUUID,
                            sourceAssetInfo.m_assetId.m_guid, productsAssetInfo);

                        for (const auto& assetInfo : productsAssetInfo)
                        {
                            if (checkExtension(assetInfo.m_relativePath.c_str()))
                            {
                                if (checkAsset(assetInfo.m_assetId, assetInfo.m_assetType))
                                {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    void PropertyAssetCtrl::ClearErrorButton()
    {
        if (m_errorButton)
        {
            layout()->removeWidget(m_errorButton);
            delete m_errorButton;
            m_errorButton = nullptr;
        }
    }

    void PropertyAssetCtrl::UpdateErrorButton()
    {
        if (m_errorButton)
        {
            // If the button is already active, disconnect its pressed handler so we don't get multiple popups
            disconnect(m_errorButton, &QPushButton::pressed, this, nullptr);
        }
        else
        {
            // If error button already set, don't do it again
            m_errorButton = new QPushButton(this);
            m_errorButton->setFlat(true);
            m_errorButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            m_errorButton->setFixedSize(QSize(16, 16));
            m_errorButton->setMouseTracking(true);
            m_errorButton->setIcon(QIcon(":/PropertyEditor/Resources/error_icon.png"));
            m_errorButton->setToolTip("Show Errors");

            // Insert the error button after the asset label
            qobject_cast<QHBoxLayout*>(layout())->insertWidget(1, m_errorButton);
        }
    }

    void PropertyAssetCtrl::UpdateErrorButtonWithLog(const AZStd::string& errorLog)
    {
        UpdateErrorButton();

        // Connect pressed to opening the error dialog
        // Must capture this for call to QObject::connect
        connect(m_errorButton, &QPushButton::pressed, this, [errorLog]() {
            // Create the dialog for the log panel, and set the layout
            QDialog* logDialog = new QDialog();
            logDialog->setMinimumSize(1024, 400);
            logDialog->setObjectName("Asset Errors");
            QHBoxLayout* pLayout = new QHBoxLayout(logDialog);
            logDialog->setLayout(pLayout);

            // Create the Generic Log Panel and place it into the QDialog
            AzToolsFramework::LogPanel::GenericLogPanel* logPanel = new AzToolsFramework::LogPanel::GenericLogPanel(logDialog);
            logDialog->layout()->addWidget(logPanel);

            // Give the log panel data to display
            logPanel->ParseData(errorLog.c_str(), errorLog.size());

            // The user can click the "reset button" to reset the tabs to default
            auto tabsResetFunction = [logPanel]() -> void
            {
                logPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("All output", "", ""));
                logPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Warnings/Errors Only", "", "", false, true, true, false));
            };

            // We call the above function to set the initial state to be the reset state, otherwise it would start blank.
            tabsResetFunction();

            // Connect the above function to when the user clicks the reset tabs button
            QObject::connect(logPanel, &AzToolsFramework::LogPanel::BaseLogPanel::TabsReset, logPanel, tabsResetFunction);
            // Connect to finished slot to delete the allocated dialog
            QObject::connect(logDialog, &QDialog::finished, logDialog, &QObject::deleteLater);

            // Show the dialog
            logDialog->adjustSize();
            logDialog->show();
        });
    }

    void PropertyAssetCtrl::UpdateErrorButtonWithMessage(const AZStd::string& message)
    {
        UpdateErrorButton();

        connect(m_errorButton, &QPushButton::clicked, this, [this, message]() {
            QMessageBox::critical(nullptr, "Error", message.c_str());

            // Without this, the error button would maintain focus after clicking, which left the red error icon in a blue-highlighted state
            if (parentWidget())
            {
                parentWidget()->setFocus();
            }
        });
    }

    void PropertyAssetCtrl::ClearAssetInternal()
    {
        ClearErrorButton();
        SetCurrentAssetHint(AZStd::string());
        SetSelectedAssetID(AZ::Data::AssetId());
        // To clear the asset we only need to refresh the values.
        ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay, Refresh_Values);
    }

    void PropertyAssetCtrl::SourceFileChanged(AZStd::string /*relativePath*/, AZStd::string /*scanFolder*/, AZ::Uuid sourceUUID)
    {
        if (GetCurrentAssetID().m_guid == sourceUUID)
        {
            ClearErrorButton();
        }
    }

    void PropertyAssetCtrl::SourceFileFailed(AZStd::string /*relativePath*/, AZStd::string /*scanFolder*/, AZ::Uuid sourceUUID)
    {
        if (GetCurrentAssetID().m_guid == sourceUUID)
        {
            UpdateAssetDisplay();
        }
    }

    void PropertyAssetCtrl::OnAssetCreated(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            [&assetInfo, assetId](AZ::Data::AssetCatalogRequestBus::Events* handler)
            {
                assetInfo = handler->GetAssetInfoById(assetId);
            }
        );

        if (assetInfo.m_assetType == GetCurrentAssetType())
        {
            SetSelectedAssetID(assetId);
        }
    }

    void PropertyAssetCtrl::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        if (GetCurrentAssetID() == assetId)
        {
            UpdateAssetDisplay();
        }
    }

    void PropertyAssetCtrl::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        if (GetCurrentAssetID() == assetId)
        {
            UpdateAssetDisplay();
        }
    }

    void PropertyAssetCtrl::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo&)
    {
        if (GetCurrentAssetID() == assetId)
        {
            UpdateAssetDisplay();
        }
    }

    void PropertyAssetCtrl::dragEnterEvent(QDragEnterEvent* event)
    {
        const bool validDropTarget = IsCorrectMimeData(event->mimeData());
        AzQtComponents::BrowseEdit::applyDropTargetStyle(m_browseEdit, validDropTarget);
        // Accept the event so that we get a dragLeaveEvent and can remove the style.
        // Note that this does not accept the proposed action.
        event->accept();

        if (validDropTarget)
        {
            event->acceptProposedAction();
        }
    }

    void PropertyAssetCtrl::dragLeaveEvent(QDragLeaveEvent* event)
    {
        (void)event;

        AzQtComponents::BrowseEdit::removeDropTargetStyle(m_browseEdit);
    }

    void PropertyAssetCtrl::dropEvent(QDropEvent* event)
    {
        //do nothing if the line edit is disabled
        if (m_browseEdit->isVisible() && !m_browseEdit->isEnabled())
        {
            return;
        }

        AZ::Data::AssetId readId;
        AZ::Data::AssetType readType;
        if (IsCorrectMimeData(event->mimeData(), &readId, &readType))
        {
            if (readId.IsValid())
            {
                SetSelectedAssetID(readId);
            }
            event->acceptProposedAction();
        }

        AzQtComponents::BrowseEdit::removeDropTargetStyle(m_browseEdit);
    }

    AssetSelectionModel PropertyAssetCtrl::GetAssetSelectionModel()
    {
        const bool multiselect = false;
        const bool supportSelectingSources = true;
        auto selectionModel = AssetSelectionModel::AssetTypeSelection(GetSelectableAssetTypes(), multiselect, supportSelectingSources);
        selectionModel.SetTitle(m_title);
        return selectionModel;
    }

    void PropertyAssetCtrl::UpdateTabOrder()
    {
        setTabOrder(m_browseEdit->lineEdit(), m_editButton);
    }

    bool PropertyAssetCtrl::eventFilter(QObject* obj, QEvent* event)
    {
        (void)obj;

        if (isEnabled())
        {
            switch (event->type())
            {
                case QEvent::FocusIn:
                {
                    OnLineEditFocus(true);
                    break;
                }
                case QEvent::FocusOut:
                {
                    OnLineEditFocus(false);
                    break;
                }
            }
        }

        return false;
    }

    PropertyAssetCtrl::~PropertyAssetCtrl()
    {
        AssetEditor::AssetEditorNotificationsBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        AssetSystemBus::Handler::BusDisconnect();
    }

    void PropertyAssetCtrl::OnEditButtonClicked()
    {
        const AZ::Data::AssetId assetID = GetCurrentAssetID();
        if (m_editNotifyCallback)
        {
            AZ_Error("Asset Property", m_editNotifyTarget, "No notification target set for edit callback.");
            m_editNotifyCallback->Invoke(m_editNotifyTarget, assetID, GetCurrentAssetType());
            return;
        }
        else
        {
            // Show default asset editor (property grid dialog) if this asset type has edit reflection.
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (serializeContext)
            {
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(GetCurrentAssetType());
                if (classData && classData->m_editData)
                {
                    if (!assetID.IsValid())
                    {
                        // No Asset Id selected - Open editor and create new asset for them
                        AssetEditor::AssetEditorRequestsBus::Broadcast(
                            &AssetEditor::AssetEditorRequests::CreateNewAsset, GetCurrentAssetType(), m_componentUuid);
                    }
                    else
                    {
                        // Open the asset with the preferred asset editor
                        bool handled = false;
                        AssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(
                            &AssetBrowser::AssetBrowserInteractionNotifications::OpenAssetInAssociatedEditor, assetID, handled);
                    }

                    return;
                }
            }
        }

        QMessageBox::warning(GetActiveWindow(), tr("Unable to Edit Asset"),
            QObject::tr("No callback is provided and no associated editor could be found."), QMessageBox::Ok, QMessageBox::Ok);
    }

    void PropertyAssetCtrl::PopupAssetPicker()
    {
        // Request the AssetBrowser Dialog and set a type filter
        AssetSelectionModel selection = GetAssetSelectionModel();
        selection.SetSelectedAssetId(m_selectedAssetID);

        AZStd::string defaultDirectory;
        if (m_defaultDirectoryCallback)
        {
            m_defaultDirectoryCallback->Invoke(m_editNotifyTarget, defaultDirectory);
            selection.SetDefaultDirectory(defaultDirectory);
        }

        if (m_hideProductFilesInAssetPicker)
        {
            FilterConstType displayFilter = selection.GetDisplayFilter();

            EntryTypeFilter* productsFilter = new EntryTypeFilter();
            productsFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Product);

            InverseFilter* noProductsFilter = new InverseFilter();
            noProductsFilter->SetFilter(FilterConstType(productsFilter));

            CompositeFilter* compFilter = new CompositeFilter(CompositeFilter::LogicOperatorType::AND);
            compFilter->AddFilter(FilterConstType(displayFilter));
            compFilter->AddFilter(FilterConstType(noProductsFilter));

            selection.SetDisplayFilter(FilterConstType(compFilter));
        }

        PickAssetSelectionFromDialog(selection, parentWidget());
        if (selection.IsValid())
        {
            const auto entry = selection.GetResult();
            if (entry->GetEntryType() == AssetBrowser::AssetBrowserEntry::AssetEntryType::Product ||
                entry->GetEntryType() == AssetBrowser::AssetBrowserEntry::AssetEntryType::Source)
            {
                // Support selecting and assigning source and product assets. If the entry is a matching product asset, it will be assigned
                // immediately. If it is a source asset entry, enumerate all of the children and assign the first compatible product asset.
                bool result = false;
                entry->VisitDown(
                    [&](const auto& currentEntry)
                    {
                        if (!result)
                        {
                            if (const auto product = azrtti_cast<const AssetBrowser::ProductAssetBrowserEntry*>(currentEntry))
                            {
                                if (CanAcceptAsset(product->GetAssetId(), product->GetAssetType()))
                                {
                                    SetSelectedAssetID(product->GetAssetId());
                                    result = true;
                                }
                            }
                        }
                        return !result;
                    });
            }
            else if (entry->GetEntryType() == AssetBrowser::AssetBrowserEntry::AssetEntryType::Folder)
            {
                SetFolderSelection(entry->GetRelativePath());
                SetSelectedAssetID(AZ::Data::AssetId());
            }
        }
    }

    void PropertyAssetCtrl::PickAssetSelectionFromDialog(AssetSelectionModel& selection, QWidget* parent)
    {
        AssetBrowserComponentRequestBus::Broadcast(&AssetBrowserComponentRequests::PickAssets, selection, parent);
    }

    void PropertyAssetCtrl::OnClearButtonClicked()
    {
        ClearAssetInternal();
        m_browseEdit->lineEdit()->clearFocus();
    }

    void PropertyAssetCtrl::SetSelectedAssetID(const AZ::Data::AssetId& newID)
    {
        m_incompleteFilename = false;

        // Early out if we're attempting to set the same asset ID unless the
        // asset is a folder. Folders don't have an asset ID, so we bypass
        // the early-out for folder selections. See PropertyHandlerDirectory.
        const bool isFolderSelection = !GetFolderSelection().empty();
        if (m_selectedAssetID == newID && !isFolderSelection)
        {
            UpdateAssetDisplay();
            return;
        }

        // If the new asset ID is not valid, raise a clearNotify callback
        // This is invoked before the new asset is assigned to ensure the callback
        // has access to the previous asset before it is cleared.
        if (!newID.IsValid() && m_clearNotifyCallback)
        {
            AZ_Error("Asset Property", m_editNotifyTarget, "No notification target set for clear callback.");
            m_clearNotifyCallback->Invoke(m_editNotifyTarget);
        }

        m_selectedAssetID = newID;

        // If the id is valid, connect to the asset system bus
        AssetSystemBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        if (newID.IsValid())
        {
            AssetSystemBus::Handler::BusConnect();
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        UpdateAssetDisplay();
        emit OnAssetIDChanged(newID);
    }

    void PropertyAssetCtrl::SetCurrentAssetType(const AZ::Data::AssetType& newType)
    {
        if (m_currentAssetType == newType)
        {
            return;
        }

        m_currentAssetType = newType;

        // Get Asset Type Name - If empty string (meaning it's an unregistered asset type), disable autocomplete and show label instead of lineEdit
        // (in short, we revert to previous functionality without autocomplete)
        AZStd::string assetTypeName;
        AZ::AssetTypeInfoBus::EventResult(assetTypeName, m_currentAssetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

        m_unnamedType = assetTypeName.empty();
        m_browseEdit->setLineEditReadOnly(m_unnamedType);

        UpdateAssetDisplay();
    }

    void PropertyAssetCtrl::SetSelectedAssetID(const AZ::Data::AssetId& newID, const AZ::Data::AssetType& newType)
    {
        m_incompleteFilename = false;

        if (m_selectedAssetID == newID && m_currentAssetType == newType)
        {
            return;
        }

        m_currentAssetType = newType;

        // Get Asset Type Name - If empty string (meaning it's an unregistered asset type), disable autocomplete and show label instead of lineEdit
        // (in short, we revert to previous functionality without autocomplete)
        AZStd::string assetTypeName;
        AZ::AssetTypeInfoBus::EventResult(assetTypeName, m_currentAssetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

        m_unnamedType = assetTypeName.empty();
        m_browseEdit->setLineEditReadOnly(m_unnamedType);

        m_selectedAssetID = newID;

        // If the id is valid, connect to the asset system bus
        AssetSystemBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        if (newID.IsValid())
        {
            AssetSystemBus::Handler::BusConnect();
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        UpdateAssetDisplay();
        emit OnAssetIDChanged(newID);
    }

    void PropertyAssetCtrl::SetCurrentAssetHint(const AZStd::string& hint)
    {
        m_currentAssetHint = hint;
    }

    // Note: Setting a Default Asset ID here will only display placeholder information when no asset is selected.
    // The default asset will not be automatically written into property (and thus won't display in-editor).
    // Functionality to use the default asset when no asset is selected has to be implemented on the component side.
    void PropertyAssetCtrl::SetDefaultAssetID(const AZ::Data::AssetId& defaultID)
    {
        m_defaultAssetID = defaultID;
        m_defaultAssetHint.clear();
        m_browseEdit->setPlaceholderText("");

        if (m_defaultAssetID.IsValid())
        {
            AZStd::string assetPath;

            if (m_showProductAssetName)
            {
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, m_defaultAssetID);
            }
            else
            {
                AZ::Data::AssetInfo assetInfo;
                AZStd::string rootFilePath;
                const AZStd::string platformName; // Empty for default
                for (const auto& assetType : GetSelectableAssetTypes())
                {
                    bool result = false;
                    AssetSystemRequestBus::BroadcastResult(
                        result,
                        &AssetSystem::AssetSystemRequest::GetAssetInfoById,
                        m_defaultAssetID,
                        assetType,
                        platformName,
                        assetInfo,
                        rootFilePath);
                    if (result)
                    {
                        assetPath = assetInfo.m_relativePath;
                        break;
                    }
                }
            }

            if (!assetPath.empty())
            {
                AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), m_defaultAssetHint);
            }

            m_browseEdit->setPlaceholderText((m_defaultAssetHint + m_DefaultSuffix).c_str());
        }

        UpdateAssetDisplay();
    }

    void PropertyAssetCtrl::UpdateAssetDisplay()
    {
        UpdateThumbnail();
        UpdateEditButton();

        const AZStd::string& folderPath = GetFolderSelection();
        if (!folderPath.empty())
        {
            m_currentAssetHint = folderPath;
        }
        else
        {
            const AZ::Data::AssetId assetID = GetCurrentAssetID();

            if (assetID.IsValid())
            {
                AZ::Outcome<AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
                AssetSystemJobRequestBus::BroadcastResult(jobOutcome, &AssetSystemJobRequestBus::Events::GetAssetJobsInfoByAssetID, assetID, false, false);

                if (jobOutcome.IsSuccess())
                {
                    AZStd::string assetPath;
                    AssetSystem::JobInfoContainer& jobs = jobOutcome.GetValue();

                    // Get the asset relative path
                    AssetSystem::JobStatus assetStatus = AssetSystem::JobStatus::Completed;

                    if (!jobs.empty())
                    {
                        // The default behavior is to show the source filename.
                        assetPath = jobs[0].m_sourceFile;

                        AZStd::string errorLog;

                        for (const auto& jobInfo : jobs)
                        {
                            // If the job has failed, mark the asset as failed, and collect the log.
                            switch (jobInfo.m_status)
                            {
                            case AssetSystem::JobStatus::Failed:
                            case AssetSystem::JobStatus::Failed_InvalidSourceNameExceedsMaxLimit:
                                {
                                    assetStatus = AssetSystem::JobStatus::Failed;

                                    AZ::Outcome<AZStd::string> logOutcome = AZ::Failure();
                                    AssetSystemJobRequestBus::BroadcastResult(logOutcome, &AssetSystemJobRequestBus::Events::GetJobLog, jobInfo.m_jobRunKey);
                                    if (logOutcome.IsSuccess())
                                    {
                                        errorLog += logOutcome.TakeValue();
                                        errorLog += '\n';
                                    }
                                }
                                break;

                            // If the job is in progress, mark the asset as in progress
                            case AssetSystem::JobStatus::InProgress:
                                {
                                    // Only mark asset in progress if it isn't already in progress, or marked as an error
                                    if (assetStatus == AssetSystem::JobStatus::Completed)
                                    {
                                        assetStatus = AssetSystem::JobStatus::InProgress;
                                    }
                                }
                                break;
                            }
                        }

                        switch (assetStatus)
                        {
                        // In case of failure, render failure icon
                        case AssetSystem::JobStatus::Failed:
                            {
                                UpdateErrorButtonWithLog(errorLog);
                            }
                            break;

                        // In case of success, remove error elements
                        case AssetSystem::JobStatus::Completed:
                            {
                                ClearErrorButton();
                            }
                            break;
                        }
                    }
                    else
                    {
                        // If there aren't any jobs and the asset ID is valid, the asset must have been removed.
                        if (assetID.IsValid())
                        {
                            UpdateErrorButtonWithMessage(AZStd::string::format(
                                "Asset has been removed.\n\nID: %s\nHint:%s",
                                assetID.ToString<AZStd::string>().c_str(),
                                GetCurrentAssetHint().c_str()));
                        }
                    }

                    // This can be turned on with an attribute in EditContext
                    if (m_showProductAssetName)
                    {
                        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, assetID);
                    }

                    // Only change the asset name if the asset not found or there's no last known good name for it
                    if (!assetPath.empty() && (assetStatus != AssetSystem::JobStatus::Completed || m_currentAssetHint != assetPath))
                    {
                        m_currentAssetHint = assetPath;
                    }
                }
                else
                {
                    // The asset might have been created in-memory (for example for the default asset provided
                    // via DefaultAsset attribute) and therefore it doesn't have a job info.
                    // Only report the asset missing if it doesn't exist in the asset manager.
                    if (!AZ::Data::AssetManager::Instance().FindAsset(assetID, AZ::Data::AssetLoadBehavior::Default))
                    {
                        UpdateErrorButtonWithMessage(AZStd::string::format(
                            "Asset is missing.\n\nID: %s\nHint:%s",
                            assetID.ToString<AZStd::string>().c_str(),
                            GetCurrentAssetHint().c_str()));
                    }
                }
            }
        }

        // Get the asset file name
        AZStd::string assetName = m_currentAssetHint;
        if (!m_currentAssetHint.empty())
        {
            AzFramework::StringFunc::Path::GetFileName(m_currentAssetHint.c_str(), assetName);
        }

        setToolTip(m_currentAssetHint.c_str());
        
        // If no asset is selected but a default asset id is, show the default name.
        if (m_selectedAssetID.IsValid())
        {
            m_browseEdit->setText(assetName.c_str());
        }
        else
        {
            m_browseEdit->setText("");
        }
    }

    void PropertyAssetCtrl::OnLineEditFocus(bool focus)
    {
        if (focus && m_completerIsConfigured)
        {
            RefreshAutocompleter();
        }

        // When focus is lost, revert to the selected asset
        if (!focus && m_incompleteFilename)
        {
            SetSelectedAssetID(GetCurrentAssetID());
        }
    }

    void PropertyAssetCtrl::SetEditButtonEnabled(bool enabled)
    {
        m_editButton->setEnabled(enabled);
    }

    void PropertyAssetCtrl::SetEditButtonVisible(bool visible)
    {
        m_showEditButton = visible;
        m_editButton->setVisible(m_showEditButton);
        UpdateEditButton();
    }

    void PropertyAssetCtrl::SetEditButtonIcon(const QIcon& icon)
    {
        m_editButton->setIcon(icon);
    }

    void PropertyAssetCtrl::SetTitle(const QString& title)
    {
        m_title = title;
    }

    void PropertyAssetCtrl::SetEditNotifyTarget(void* editNotifyTarget)
    {
        m_editNotifyTarget = editNotifyTarget;
    }

    void PropertyAssetCtrl::SetEditNotifyCallback(EditCallbackType* editNotifyCallback)
    {
        m_editNotifyCallback = editNotifyCallback;
    }

    void PropertyAssetCtrl::SetDefaultDirectoryCallback(DefaultDirectoryCallbackType* callback)
    {
        m_defaultDirectoryCallback = callback;
    }

    void PropertyAssetCtrl::SetClearNotifyCallback(ClearCallbackType* clearNotifyCallback)
    {
        m_clearNotifyCallback = clearNotifyCallback;
    }

    void PropertyAssetCtrl::SetEditButtonTooltip(QString tooltip)
    {
        m_editButton->setToolTip(tooltip);
    }

    void PropertyAssetCtrl::SetComponentId(const AZ::Uuid& uuid)
    {
        m_componentUuid = uuid;
        AssetEditor::AssetEditorNotificationsBus::Handler::BusConnect(m_componentUuid);
    }

    void PropertyAssetCtrl::SetBrowseButtonIcon(const QIcon& icon)
    {
        m_browseEdit->setAttachedButtonIcon(icon);
    }

    void PropertyAssetCtrl::SetBrowseButtonEnabled(bool enabled)
    {
        m_browseEdit->setEnabled(enabled);
    }

    void PropertyAssetCtrl::SetBrowseButtonVisible(bool visible)
    {
        m_browseEdit->setVisible(visible);
    }

    const QModelIndex PropertyAssetCtrl::GetSourceIndex(const QModelIndex& index)
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }

        // mapToSource is only available for QAbstractProxyModel but completionModel() returns a QAbstractItemModel, hence the cast.
        return static_cast<QAbstractProxyModel*>(m_completer->completionModel())->mapToSource(index);
    }

    void PropertyAssetCtrl::UpdateThumbnail()
    {
        m_thumbnail->setVisible(m_showThumbnail);

        if (m_showThumbnail)
        {
            m_thumbnail->ShowDropDownArrow(m_showThumbnailDropDownButton);
            const AZ::Data::AssetId assetID = GetCurrentAssetID();
            if (assetID.IsValid())
            {
                bool result = false;
                AZ::Data::AssetInfo assetInfo;
                AZ::Data::AssetType assetType;
                const AZStd::string platformName = ""; // Empty for default
                AZStd::string rootFilePath;
                AssetSystemRequestBus::BroadcastResult(result, &AssetSystemRequestBus::Events::GetAssetInfoById, assetID, assetType, platformName, assetInfo, rootFilePath);

                if (result)
                {
                    SharedThumbnailKey thumbnailKey = MAKE_TKEY(AzToolsFramework::AssetBrowser::ProductThumbnailKey, assetID);
                    if (m_showThumbnail)
                    {
                        m_thumbnail->SetThumbnailKey(thumbnailKey);
                    }
                    return;
                }
            }
        }

        m_thumbnail->ClearThumbnail();
    }

    void PropertyAssetCtrl::UpdateEditButton()
    {
        // if Edit button is in use (shown), enable/disable it depending on the current asset id.
        if (m_showEditButton && m_disableEditButtonWhenNoAssetSelected)
        {
            m_editButton->setEnabled(GetSelectedAssetID().IsValid());
        }
    }

    void PropertyAssetCtrl::SetClearButtonEnabled(bool enable)
    {
        m_browseEdit->setClearButtonEnabled(enable);
        m_allowEmptyValue = enable;
    }

    void PropertyAssetCtrl::SetClearButtonVisible(bool visible)
    {
        SetClearButtonEnabled(visible);
    }

    void PropertyAssetCtrl::SetShowProductAssetName(bool enable)
    {
        m_showProductAssetName = enable;
    }

    bool PropertyAssetCtrl::GetShowProductAssetName() const
    {
        return m_showProductAssetName;
    }

    void PropertyAssetCtrl::SetHideProductFilesInAssetPicker(bool hide)
    {
        m_hideProductFilesInAssetPicker = hide;
    }

    bool PropertyAssetCtrl::GetHideProductFilesInAssetPicker() const
    {
        return m_hideProductFilesInAssetPicker;
    }

    void PropertyAssetCtrl::SetDisableEditButtonWhenNoAssetSelected(bool disableEditButtonWhenNoAssetSelected)
    {
        m_disableEditButtonWhenNoAssetSelected = disableEditButtonWhenNoAssetSelected;
        UpdateEditButton();
    }

    bool PropertyAssetCtrl::GetDisableEditButtonWhenNoAssetSelected() const
    {
        return m_disableEditButtonWhenNoAssetSelected;
    }

    void PropertyAssetCtrl::SetShowThumbnail(bool enable)
    {
        m_showThumbnail = enable;
    }

    bool PropertyAssetCtrl::GetShowThumbnail() const
    {
        return m_showThumbnail;
    }

    void PropertyAssetCtrl::SetShowThumbnailDropDownButton(bool enable)
    {
        m_showThumbnailDropDownButton = enable;
    }

    bool PropertyAssetCtrl::GetShowThumbnailDropDownButton() const
    {
        return m_showThumbnailDropDownButton;
    }

    void PropertyAssetCtrl::SetCustomThumbnailEnabled(bool enabled)
    {
        m_thumbnail->SetCustomThumbnailEnabled(enabled);
    }

    void PropertyAssetCtrl::SetCustomThumbnailPixmap(const QPixmap& pixmap)
    {
        m_thumbnail->SetCustomThumbnailPixmap(pixmap);
    }

    void PropertyAssetCtrl::SetSupportedAssetTypes(const AZStd::vector<AZ::Data::AssetType>& supportedAssetTypes)
    {
        m_supportedAssetTypes = supportedAssetTypes;
    }

    const AZStd::vector<AZ::Data::AssetType>& PropertyAssetCtrl::GetSupportedAssetTypes() const
    {
        return m_supportedAssetTypes;
    }

    AZStd::vector<AZ::Data::AssetType> PropertyAssetCtrl::GetSelectableAssetTypes() const
    {
        AZStd::vector<AZ::Data::AssetType> types = GetSupportedAssetTypes();
        if (GetCurrentAssetType() != AZ::Data::s_invalidAssetType)
        {
            types.push_back(GetCurrentAssetType());
        }
        return types;
    }

    void PropertyAssetCtrl::SetThumbnailCallback(EditCallbackType* editNotifyCallback)
    {
        m_thumbnailCallback = editNotifyCallback;
    }

    AZ::TypeId AssetPropertyHandlerDefault::GetHandledType() const
    {
        return AZ::GetAssetClassId();
    }

    QWidget* AssetPropertyHandlerDefault::CreateGUI(QWidget* pParent)
    {
        PropertyAssetCtrl* newCtrl = aznew PropertyAssetCtrl(pParent);
        connect(newCtrl, &PropertyAssetCtrl::OnAssetIDChanged, this, [newCtrl](AZ::Data::AssetId newAssetID)
        {
            (void)newAssetID;
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
        return newCtrl;
    }

    void ConsumeAttributeForPropertyAssetCtrl(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        (void)debugName;

        if (attrib == AZ_CRC_CE("AssetPickerTitle"))
        {
            AZStd::string title;
            attrValue->Read<AZStd::string>(title);
            if (!title.empty())
            {
                GUI->SetTitle(title.c_str());
            }
        }
        else if (attrib == AZ_CRC_CE("DefaultStartingDirectoryCallback"))
        {
            // This is assumed to be an Asset Browser path to a specific folder to be used as a default by the asset picker if provided
            GUI->SetDefaultDirectoryCallback(azdynamic_cast<PropertyAssetCtrl::DefaultDirectoryCallbackType*>(attrValue->GetAttribute()));
        }
        else if (attrib == AZ_CRC_CE("EditCallback"))
        {
            PropertyAssetCtrl::EditCallbackType* func = azdynamic_cast<PropertyAssetCtrl::EditCallbackType*>(attrValue->GetAttribute());
            if (func)
            {
                GUI->SetEditButtonVisible(true);
                GUI->SetEditNotifyCallback(func);
            }
            else
            {
                GUI->SetEditNotifyCallback(nullptr);
            }
        }
        else if (attrib == AZ_CRC_CE("EditButton"))
        {
            GUI->SetEditButtonVisible(true);

            AZStd::string iconPath;
            attrValue->Read<AZStd::string>(iconPath);

            if (!iconPath.empty())
            {
                QString path(iconPath.c_str());

                if (!QFile::exists(path))
                {
                    AZ::IO::FixedMaxPathString engineRoot = AZ::Utils::GetEnginePath();
                    QDir engineDir = !engineRoot.empty() ? QDir(QString(engineRoot.c_str())) : QDir::current();

                    path = engineDir.absoluteFilePath(iconPath.c_str());
                }

                GUI->SetEditButtonIcon(QIcon(path));
            }
        }
        else if (attrib == AZ_CRC_CE("EditDescription"))
        {
            AZStd::string buttonTooltip;
            if (attrValue->Read<AZStd::string>(buttonTooltip))
            {
                GUI->SetEditButtonTooltip(QObject::tr(buttonTooltip.c_str()));
            }
        }
        else if (attrib == AZ_CRC_CE("ComponentIdentifier"))
        {
            AZ::Uuid uuid;
            if (attrValue->Read<AZ::Uuid>(uuid))
            {
                GUI->SetComponentId(uuid);
            }
        }
        else if (attrib == AZ_CRC_CE("DisableEditButtonWhenNoAssetSelected"))
        {
            bool disableEditButtonWhenNoAssetSelected = false;
            attrValue->Read<bool>(disableEditButtonWhenNoAssetSelected);
            GUI->SetDisableEditButtonWhenNoAssetSelected(disableEditButtonWhenNoAssetSelected);
        }
        else if (attrib == AZ::Edit::Attributes::DefaultAsset)
        {
            AZ::Data::AssetId assetId;
            if (attrValue->Read<AZ::Data::AssetId>(assetId))
            {
                GUI->SetDefaultAssetID(assetId);
            }
        }
        else if (attrib == AZ::Edit::Attributes::AllowClearAsset)
        {
            bool visible = true;
            attrValue->Read<bool>(visible);
            GUI->SetClearButtonVisible(visible);
        }
        else if (attrib == AZ::Edit::Attributes::ShowProductAssetFileName)
        {
            bool showProductAssetName = false;
            if (attrValue->Read<bool>(showProductAssetName))
            {
                GUI->SetShowProductAssetName(showProductAssetName);
            }
        }
        else if(attrib == AZ::Edit::Attributes::HideProductFilesInAssetPicker)
        {
            bool hideProductFilesInAssetPicker = false;
            if (attrValue->Read<bool>(hideProductFilesInAssetPicker))
            {
                GUI->SetHideProductFilesInAssetPicker(hideProductFilesInAssetPicker);
            }
        }
        else if (attrib == AZ::Edit::Attributes::ClearNotify)
        {
            PropertyAssetCtrl::ClearCallbackType* func = azdynamic_cast<PropertyAssetCtrl::ClearCallbackType*>(attrValue->GetAttribute());
            if (func)
            {
                GUI->SetClearButtonVisible(true);
                GUI->SetClearNotifyCallback(func);
            }
            else
            {
                GUI->SetClearNotifyCallback(nullptr);
            }
        }
        else if (attrib == AZ_CRC_CE("BrowseIcon"))
        {
            AZStd::string iconPath;
            if (attrValue->Read<AZStd::string>(iconPath) && !iconPath.empty())
            {
                GUI->SetBrowseButtonIcon(QIcon(iconPath.c_str()));
            }
            else
            {
                // A QPixmap object can't be assigned directly via an attribute.
                // This allows dynamic icon data to be supplied as a buffer containing a serialized QPixmap.
                AZStd::vector<char> pixmapBuffer;
                if (attrValue->Read<AZStd::vector<char>>(pixmapBuffer) && !pixmapBuffer.empty())
                {
                    QByteArray pixmapBytes(pixmapBuffer.data(), aznumeric_cast<int>(pixmapBuffer.size()));
                    QDataStream stream(&pixmapBytes, QIODevice::ReadOnly);
                    QPixmap pixmap;
                    stream >> pixmap;
                    if (!pixmap.isNull())
                    {
                        GUI->SetBrowseButtonIcon(pixmap);
                    }
                }
            }
        }
        else if (attrib == AZ_CRC_CE("BrowseButtonEnabled"))
        {
            bool enabled = true;
            if (attrValue->Read<bool>(enabled))
            {
                GUI->SetBrowseButtonEnabled(enabled);
            }
        }
        else if (attrib == AZ_CRC_CE("BrowseButtonVisible"))
        {
            bool visible = true;
            if (attrValue->Read<bool>(visible))
            {
                GUI->SetBrowseButtonVisible(visible);
            }
        }
        else if (attrib == AZ_CRC_CE("Thumbnail"))
        {
            bool showThumbnail = false;
            if (attrValue->Read<bool>(showThumbnail))
            {
                GUI->SetShowThumbnail(showThumbnail);
            }
        }
        else if (attrib == AZ_CRC_CE("ThumbnailIcon"))
        {
            GUI->SetCustomThumbnailEnabled(false);

            AZStd::string iconPath;
            if (attrValue->Read<AZStd::string>(iconPath) && !iconPath.empty())
            {
                GUI->SetCustomThumbnailEnabled(true);
                GUI->SetCustomThumbnailPixmap(QPixmap::fromImage(QImage(iconPath.c_str())));
            }
            else
            {
                // A QPixmap object can't be assigned directly via an attribute.
                // This allows dynamic icon data to be supplied as a buffer containing a serialized QPixmap.
                AZStd::vector<char> pixmapBuffer;
                if (attrValue->Read<AZStd::vector<char>>(pixmapBuffer) && !pixmapBuffer.empty())
                {
                    QByteArray pixmapBytes(pixmapBuffer.data(), aznumeric_cast<int>(pixmapBuffer.size()));
                    QDataStream stream(&pixmapBytes, QIODevice::ReadOnly);
                    QPixmap pixmap;
                    stream >> pixmap;
                    if (!pixmap.isNull())
                    {
                        GUI->SetCustomThumbnailEnabled(true);
                        GUI->SetCustomThumbnailPixmap(pixmap);
                    }
                }
            }
        }
        else if (attrib == AZ_CRC_CE("ThumbnailCallback"))
        {
            PropertyAssetCtrl::EditCallbackType* func = azdynamic_cast<PropertyAssetCtrl::EditCallbackType*>(attrValue->GetAttribute());
            if (func)
            {
                GUI->SetShowThumbnail(true);
                GUI->SetShowThumbnailDropDownButton(true);
                GUI->SetThumbnailCallback(func);
            }
            else
            {
                GUI->SetShowThumbnailDropDownButton(false);
                GUI->SetThumbnailCallback(nullptr);
            }
        }
        else if (attrib == AZ_CRC_CE("SupportedAssetTypes"))
        {
            AZStd::vector<AZ::Data::AssetType> supportedAssetTypes;
            if (attrValue->Read<AZStd::vector<AZ::Data::AssetType>>(supportedAssetTypes))
            {
                GUI->SetSupportedAssetTypes(supportedAssetTypes);
            }
        }
    }

    void AssetPropertyHandlerDefault::ConsumeAttribute(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeForPropertyAssetCtrl(GUI, attrib, attrValue, debugName);
    }

    void AssetPropertyHandlerDefault::WriteGUIValuesIntoPropertyInternal(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        if (!GUI->GetSelectedAssetID().IsValid())
        {
            instance = property_t(AZ::Data::AssetId(), GUI->GetCurrentAssetType(), "");
        }
        else
        {
            instance = property_t(GUI->GetSelectedAssetID(), GUI->GetCurrentAssetType(), GUI->GetCurrentAssetHint());
        }
    }

    void AssetPropertyHandlerDefault::WriteGUIValuesIntoProperty(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        WriteGUIValuesIntoPropertyInternal(index, GUI, instance, node);
    }

    bool AssetPropertyHandlerDefault::ReadValuesIntoGUIInternal(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        GUI->blockSignals(true);

        AZ_Assert(node->GetElementMetadata()->m_genericClassInfo, "Property does not have element data.");
        AZ_Assert(node->GetElementMetadata()->m_genericClassInfo->GetNumTemplatedArguments() == 1, "Asset<> should have only 1 template parameter.");

        const AZ::Uuid& assetTypeId = node->GetElementMetadata()->m_genericClassInfo->GetTemplatedTypeId(0);

        GUI->SetCurrentAssetHint(instance.GetHint());
        GUI->SetSelectedAssetID(instance.GetId(), assetTypeId);
        GUI->SetEditNotifyTarget(node->GetParent()->GetInstance(0));

        GUI->blockSignals(false);

        return false;
    }

    bool AssetPropertyHandlerDefault::ReadValuesIntoGUI(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        return ReadValuesIntoGUIInternal(index, GUI, instance, node);
    }

    AZ::Data::Asset<AZ::Data::AssetData>* AssetPropertyHandlerDefault::CastToInternal(void* instance, const InstanceDataNode* node)
    {
        if (node->GetElementMetadata()->m_genericClassInfo && node->GetElementMetadata()->m_genericClassInfo->GetGenericTypeId() == AZ::GetAssetClassId())
        {
            return static_cast<AZ::Data::Asset<AZ::Data::AssetData>*>(instance);
        }

        return nullptr;
    }

    AZ::Data::Asset<AZ::Data::AssetData>* AssetPropertyHandlerDefault::CastTo(void* instance, const InstanceDataNode* node, [[maybe_unused]] const AZ::Uuid& fromId, [[maybe_unused]] const AZ::Uuid& toId) const
    {
        return CastToInternal(instance, node);
    }

    QWidget* AssetIdPropertyHandlerDefault::CreateGUI(QWidget* pParent)
    {
        PropertyAssetCtrl* newCtrl = aznew PropertyAssetCtrl(pParent);
        connect(newCtrl, &PropertyAssetCtrl::OnAssetIDChanged, this, [newCtrl](AZ::Data::AssetId newAssetID)
        {
            (void)newAssetID;
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
        return newCtrl;
    }

    void AssetIdPropertyHandlerDefault::ConsumeAttribute(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeForPropertyAssetCtrl(GUI, attrib, attrValue, debugName);
    }

    void AssetIdPropertyHandlerDefault::WriteGUIValuesIntoProperty(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (void)index;
        (void)node;
        instance = GUI->GetSelectedAssetID();
    }

    bool AssetIdPropertyHandlerDefault::ReadValuesIntoGUI(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        GUI->blockSignals(true);
        GUI->SetSelectedAssetID(instance);
        GUI->SetEditNotifyTarget(node->GetParent()->GetInstance(0));
        GUI->blockSignals(false);
        return false;
    }

    QWidget* SimpleAssetPropertyHandlerDefault::CreateGUI(QWidget* pParent)
    {
        PropertyAssetCtrl* newCtrl = aznew PropertyAssetCtrl(pParent);
        connect(newCtrl, &PropertyAssetCtrl::OnAssetIDChanged, this, [newCtrl](AZ::Data::AssetId newAssetID)
        {
            (void)newAssetID;
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
        return newCtrl;
    }

    void SimpleAssetPropertyHandlerDefault::ConsumeAttribute(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeForPropertyAssetCtrl(GUI, attrib, attrValue, debugName);
    }

    void SimpleAssetPropertyHandlerDefault::WriteGUIValuesIntoPropertyInternal(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        AZStd::string assetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, GUI->GetSelectedAssetID());

        instance.SetAssetPath(assetPath.c_str());
    }

    void SimpleAssetPropertyHandlerDefault::WriteGUIValuesIntoProperty(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        WriteGUIValuesIntoPropertyInternal(index, GUI, instance, node);
    }

    bool SimpleAssetPropertyHandlerDefault::ReadValuesIntoGUIInternal(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        GUI->blockSignals(true);

        AZ::Data::AssetId assetId;
        if (!instance.GetAssetPath().empty())
        {
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                instance.GetAssetPath().c_str(), instance.GetAssetType(), true);
        }

        // Set the hint in case the asset is not able to be found by assetId
        GUI->SetCurrentAssetHint(instance.GetAssetPath());
        GUI->SetSelectedAssetID(assetId, instance.GetAssetType());
        GUI->SetEditNotifyTarget(node->GetParent()->GetInstance(0));

        GUI->blockSignals(false);
        return false;
    }

    bool SimpleAssetPropertyHandlerDefault::ReadValuesIntoGUI(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        return ReadValuesIntoGUIInternal(index, GUI, instance, node);
    }

    void RegisterAssetPropertyHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::RegisterPropertyType, aznew AssetPropertyHandlerDefault());
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::RegisterPropertyType, aznew AssetIdPropertyHandlerDefault());
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::RegisterPropertyType, aznew SimpleAssetPropertyHandlerDefault());
    }
}

#include "UI/PropertyEditor/moc_PropertyAssetCtrl.cpp"
