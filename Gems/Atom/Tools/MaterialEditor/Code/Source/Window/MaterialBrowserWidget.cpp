/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Document/MaterialDocumentRequestBus.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <Window/MaterialBrowserWidget.h>
#include <Window/ui_MaterialBrowserWidget.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QAction>
#include <QByteArray>
#include <QCursor>
#include <QDesktopServices>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    MaterialBrowserWidget::MaterialBrowserWidget(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::MaterialBrowserWidget)
    {
        using namespace AzToolsFramework::AssetBrowser;

        m_ui->setupUi(this);

        m_ui->m_searchWidget->Setup(true, true);
        m_ui->m_searchWidget->SetFilterState("", AZ::RPI::StreamingImageAsset::Group, true);
        m_ui->m_searchWidget->SetFilterState("", AZ::RPI::MaterialAsset::Group, true);
        m_ui->m_searchWidget->setMinimumSize(QSize(150, 0));
        m_ui->m_viewOptionButton->setIcon(QIcon(":/Icons/View.svg"));
        m_ui->m_splitter->setSizes(QList<int>() << 400 << 200);
        m_ui->m_splitter->setStretchFactor(0, 1);

        connect(m_ui->m_viewOptionButton, &QPushButton::clicked, this, &MaterialBrowserWidget::OpenOptionsMenu);

        // Get the asset browser model
        AssetBrowserModel* assetBrowserModel = nullptr;
        AssetBrowserComponentRequestBus::BroadcastResult(assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
        AZ_Assert(assetBrowserModel, "Failed to get file browser model");

        // Hook up the data set to the tree view
        m_filterModel = aznew AssetBrowserFilterModel(this);
        m_filterModel->setSourceModel(assetBrowserModel);
        m_filterModel->SetFilter(CreateFilter());

        m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel);
        m_ui->m_assetBrowserTreeViewWidget->SetShowSourceControlIcons(false);
        m_ui->m_assetBrowserTreeViewWidget->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

        // Maintains the tree expansion state between runs
        m_ui->m_assetBrowserTreeViewWidget->SetName("AssetBrowserTreeView_main");

        connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal, m_filterModel, &AssetBrowserFilterModel::filterUpdatedSlot);
        connect(m_filterModel, &AssetBrowserFilterModel::filterChanged, this, [this]()
        {
            const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
            constexpr bool selectFirstFilteredIndex = true;
            m_ui->m_assetBrowserTreeViewWidget->UpdateAfterFilter(hasFilter, selectFirstFilteredIndex);
        });
        connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::activated, this, &MaterialBrowserWidget::OpenSelectedEntries);
        connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::selectionChangedSignal, [this]() {
            const auto& selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
            if (!selectedAssets.empty())
            {
                m_ui->m_previewerFrame->Display(selectedAssets.front());
            }
            else
            {
                m_ui->m_previewerFrame->Clear();
            }
        });

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect();
    }

    MaterialBrowserWidget::~MaterialBrowserWidget()
    {
        // Maintains the tree expansion state between runs
        m_ui->m_assetBrowserTreeViewWidget->SaveState();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    AzToolsFramework::AssetBrowser::FilterConstType MaterialBrowserWidget::CreateFilter() const
    {
        using namespace AzToolsFramework::AssetBrowser;

        QSharedPointer<EntryTypeFilter> sourceFilter(new EntryTypeFilter);
        sourceFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Source);

        QSharedPointer<EntryTypeFilter> folderFilter(new EntryTypeFilter);
        folderFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Folder);

        QSharedPointer<CompositeFilter> sourceOrFolderFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::OR));
        sourceOrFolderFilter->AddFilter(sourceFilter);
        sourceOrFolderFilter->AddFilter(folderFilter);

        QSharedPointer<CompositeFilter> finalFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND));
        finalFilter->AddFilter(sourceOrFolderFilter);
        finalFilter->AddFilter(m_ui->m_searchWidget->GetFilter());

        return finalFilter;
    }

    void MaterialBrowserWidget::OpenSelectedEntries()
    {
        const AZStd::vector<AssetBrowserEntry*> entries = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();

        const int multiSelectPromptThreshold = 10;
        if (entries.size() >= multiSelectPromptThreshold)
        {
            if (QMessageBox::question(
                QApplication::activeWindow(),
                QString("Attemptng to open %1 files").arg(entries.size()),
                "Would you like to open anyway?",
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
            {
                return;
            }
        }

        for (const AssetBrowserEntry* entry : entries)
        {
            if (entry)
            {
                if (AzFramework::StringFunc::Path::IsExtension(entry->GetFullPath().c_str(), AZ::RPI::MaterialSourceData::Extension))
                {
                    AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument, entry->GetFullPath());
                }
                else if (AzFramework::StringFunc::Path::IsExtension(entry->GetFullPath().c_str(), AZ::RPI::MaterialTypeSourceData::Extension))
                {
                    //ignore AZ::RPI::MaterialTypeSourceData::Extension
                }
                else
                {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(entry->GetFullPath().c_str()));
                }
            }
        }
    }

    void MaterialBrowserWidget::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        AZStd::string absolutePath;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(absolutePath, documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetAbsolutePath);
        if (!absolutePath.empty())
        {
            // Selecting a new asset in the browser is not guaranteed to happen immediately.
            // The asset browser model notifications are sent before the model is updated.
            // Instead of relying on the notifications, queue the selection and process it on tick until this change occurs.
            m_pathToSelect = absolutePath;
            AzFramework::StringFunc::Path::Normalize(m_pathToSelect);
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void MaterialBrowserWidget::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        AZ_UNUSED(time);
        AZ_UNUSED(deltaTime);

        if (!m_pathToSelect.empty())
        {
            // Attempt to select the new path
            AzToolsFramework::AssetBrowser::AssetBrowserViewRequestBus::Broadcast(
                &AzToolsFramework::AssetBrowser::AssetBrowserViewRequestBus::Events::SelectFileAtPath, m_pathToSelect);

            // Iterate over the selected entries to verify if the selection was made 
            for (const AssetBrowserEntry* entry : m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets())
            {
                if (entry)
                {
                    AZStd::string sourcePath = entry->GetFullPath();
                    AzFramework::StringFunc::Path::Normalize(sourcePath);
                    if (m_pathToSelect == sourcePath)
                    {
                        // Once the selection is confirmed, cancel the operation and disconnect
                        AZ::TickBus::Handler::BusDisconnect();
                        m_pathToSelect.clear();
                    }
                }
            }
        }
    }

    void MaterialBrowserWidget::OpenOptionsMenu()
    {
        QMenu menu;

        QAction* action = new QAction("Show Asset Preview", this);
        action->setCheckable(true);
        action->setChecked(m_ui->m_previewerFrame->isVisible());
        connect(action, &QAction::triggered, [this]() {
            bool isPreviewFrameVisible = m_ui->m_previewerFrame->isVisible();
            m_ui->m_previewerFrame->setVisible(!isPreviewFrameVisible);
            if (isPreviewFrameVisible)
            {
                m_materialBrowserState = m_ui->m_splitter->saveState();
                m_ui->m_splitter->setSizes(QList({ 1, 0 }));
            }
            else
            {
                m_ui->m_splitter->restoreState(m_materialBrowserState);
            }
        });
        menu.addAction(action);
        menu.exec(QCursor::pos());
    }

} // namespace MaterialEditor

#include <Window/moc_MaterialBrowserWidget.cpp>
