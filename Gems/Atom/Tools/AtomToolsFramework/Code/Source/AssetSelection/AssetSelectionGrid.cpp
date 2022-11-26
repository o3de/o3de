/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetSelection/ui_AssetSelectionGrid.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AtomToolsFramework/AssetSelection/AssetSelectionGrid.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailWidget.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QVBoxLayout>

namespace AtomToolsFramework
{
    AssetSelectionGrid::AssetSelectionGrid(
        const QString& title,
        const FilterFn& filterFn,
        const QSize& tileSize,
        QWidget* parent)
        : QDialog(parent)
        , m_tileSize(tileSize)
        , m_ui(new Ui::AssetSelectionGrid)
        , m_filterFn(filterFn)
    {
        m_ui->setupUi(this);

        QSignalBlocker signalBlocker(this);

        setWindowTitle(title);

        SetupAssetList();
        SetupSearchWidget();
        SetupDialogButtons();
        setModal(true);

        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    AssetSelectionGrid::~AssetSelectionGrid()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    void AssetSelectionGrid::Clear()
    {
        m_ui->m_assetList->clear();
    }

    void AssetSelectionGrid::Populate()
    {
        Clear();

        for (const auto& path : GetPathsInSourceFoldersMatchingFilter(m_filterFn))
        {
            AddPath(path);
        }

        m_ui->m_assetList->sortItems();
        m_ui->m_assetList->setCurrentItem(0);
    }

    void AssetSelectionGrid::SetFilter(const FilterFn& filterFn)
    {
        m_filterFn = filterFn;
    }

    const AssetSelectionGrid::FilterFn& AssetSelectionGrid::GetFilter() const
    {
        return m_filterFn;
    }

    void AssetSelectionGrid::AddPath(const AZStd::string& path)
    {
        if(m_filterFn && !m_filterFn(path))
        {
            return;
        }

        const QVariant pathItemData(QString::fromUtf8(path.data(), static_cast<int>(path.size())));
        const QString title(GetDisplayNameFromPath(path).c_str());

        // Skip creating this list item if one with the same path is already registered
        for (int i = 0; i < m_ui->m_assetList->count(); ++i)
        {
            QListWidgetItem* item = m_ui->m_assetList->item(i);
            if (pathItemData == item->data(Qt::UserRole))
            {
                return;
            }
        }

        const int itemBorder =
            aznumeric_cast<int>(AtomToolsFramework::GetSettingsValue<AZ::u64>("/O3DE/AtomToolsFramework/AssetSelectionGrid/ItemBorder", 4));
        const int itemSpacing = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingsValue<AZ::u64>("/O3DE/AtomToolsFramework/AssetSelectionGrid/ItemSpacing", 10));
        const int headerHeight = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingsValue<AZ::u64>("/O3DE/AtomToolsFramework/AssetSelectionGrid/HeaderHeight", 15));

        const QSize gridSize = m_ui->m_assetList->gridSize();
        m_ui->m_assetList->setGridSize(QSize(
            AZStd::max(gridSize.width(), m_tileSize.width() + itemSpacing),
            AZStd::max(gridSize.height(), m_tileSize.height() + itemSpacing + headerHeight)));

        QListWidgetItem* item = new QListWidgetItem(m_ui->m_assetList);
        item->setData(Qt::DisplayRole, title);
        item->setData(Qt::UserRole, pathItemData);
        item->setSizeHint(m_tileSize + QSize(itemBorder, itemBorder + headerHeight));
        m_ui->m_assetList->addItem(item);

        QWidget* itemWidget = new QWidget(m_ui->m_assetList);
        itemWidget->setLayout(new QVBoxLayout(itemWidget));
        itemWidget->layout()->setSpacing(0);
        itemWidget->layout()->setMargin(0);

        AzQtComponents::ElidingLabel* header = new AzQtComponents::ElidingLabel(itemWidget);
        header->setText(title);
        header->setFixedSize(QSize(m_tileSize.width(), headerHeight));
        header->setMargin(0);
        header->setStyleSheet("background-color: rgb(35, 35, 35)");
        AzQtComponents::Text::addPrimaryStyle(header);
        AzQtComponents::Text::addLabelStyle(header);
        itemWidget->layout()->addWidget(header);

        bool result = false;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, path.data(), assetInfo, watchFolder);

        AzToolsFramework::Thumbnailer::ThumbnailWidget* thumbnail = new AzToolsFramework::Thumbnailer::ThumbnailWidget(itemWidget);
        thumbnail->setFixedSize(m_tileSize);
        thumbnail->SetThumbnailKey(MAKE_TKEY(AzToolsFramework::AssetBrowser::SourceThumbnailKey, assetInfo.m_assetId.m_guid));
        thumbnail->updateGeometry();
        itemWidget->layout()->addWidget(thumbnail);

        m_ui->m_assetList->setItemWidget(item, itemWidget);
        m_ui->m_assetList->sortItems();
    }

    void AssetSelectionGrid::RemovePath(const AZStd::string& path)
    {
        const QVariant pathItemData(QString::fromUtf8(path.data(), static_cast<int>(path.size())));
        for (int i = 0; i < m_ui->m_assetList->count(); ++i)
        {
            QListWidgetItem* item = m_ui->m_assetList->item(i);
            if (pathItemData == item->data(Qt::UserRole))
            {
                m_ui->m_assetList->removeItemWidget(item);
                return;
            }
        }
    }

    void AssetSelectionGrid::SelectPath(const AZStd::string& path)
    {
        const QVariant pathItemData(QString::fromUtf8(path.data(), static_cast<int>(path.size())));
        for (int i = 0; i < m_ui->m_assetList->count(); ++i)
        {
            QListWidgetItem* item = m_ui->m_assetList->item(i);
            if (pathItemData == item->data(Qt::UserRole))
            {
                m_ui->m_assetList->setCurrentItem(item);
                return;
            }
        }
    }

    AZStd::string AssetSelectionGrid::GetSelectedPath() const
    {
        auto item = m_ui->m_assetList->currentItem();
        return item ? AZStd::string(item->data(Qt::UserRole).toString().toUtf8().constData()) : AZStd::string();
    }

    void AssetSelectionGrid::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AddPath(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    void AssetSelectionGrid::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetInfo& assetInfo)
    {
        RemovePath(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    void AssetSelectionGrid::SetupAssetList()
    {
        m_ui->m_assetList->setFlow(QListView::LeftToRight);
        m_ui->m_assetList->setResizeMode(QListView::Adjust);
        m_ui->m_assetList->setGridSize(QSize(0, 0));
        m_ui->m_assetList->setWrapping(true);

        QObject::connect(m_ui->m_assetList, &QListWidget::currentItemChanged, [this](){ emit PathSelected(GetSelectedPath()); });
    }

    void AssetSelectionGrid::SetupSearchWidget()
    {
        m_ui->m_searchWidget->setReadOnly(false);
        m_ui->m_searchWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        AzQtComponents::LineEdit::applySearchStyle(m_ui->m_searchWidget);
        connect(m_ui->m_searchWidget, &QLineEdit::textChanged, this, [this](){ ApplySearchFilter(); });
        connect(m_ui->m_searchWidget, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos){ ShowSearchMenu(pos); });
    }

    void AssetSelectionGrid::SetupDialogButtons()
    {
        connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(this, &QDialog::rejected, this, &AssetSelectionGrid::PathRejected);
    }

    void AssetSelectionGrid::ApplySearchFilter()
    {
        for (int index = 0; index < m_ui->m_assetList->count(); ++index)
        {
            QListWidgetItem* item = m_ui->m_assetList->item(index);
            const QString& title = item->data(Qt::DisplayRole).toString();
            const QString filter = m_ui->m_searchWidget->text();
            item->setHidden(!filter.isEmpty() && !title.contains(filter, Qt::CaseInsensitive));
        }
    }

    void AssetSelectionGrid::ShowSearchMenu(const QPoint& pos)
    {
        QScopedPointer<QMenu> menu(m_ui->m_searchWidget->createStandardContextMenu());
        menu->setStyleSheet("background-color: #333333");
        menu->exec(m_ui->m_searchWidget->mapToGlobal(pos));
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/AssetSelection/moc_AssetSelectionGrid.cpp>
