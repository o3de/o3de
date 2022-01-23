/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetGridDialog/ui_AssetGridDialog.h>
#include <AtomToolsFramework/AssetGridDialog/AssetGridDialog.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <AzToolsFramework/Thumbnails/ThumbnailWidget.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QVBoxLayout>

namespace AtomToolsFramework
{
    AssetGridDialog::AssetGridDialog(
        const QString& title,
        const SelectableAssetVector& selectableAssets,
        const AZ::Data::AssetId& selectedAsset,
        const QSize& tileSize,
        QWidget* parent)
        : QDialog(parent)
        , m_tileSize(tileSize)
        , m_initialSelectedAsset(selectedAsset)
        , m_ui(new Ui::AssetGridDialog)
    {
        m_ui->setupUi(this);

        QSignalBlocker signalBlocker(this);

        setWindowTitle(title);

        SetupAssetList();
        SetupSearchWidget();
        SetupDialogButtons();
        setModal(true);

        QListWidgetItem* selectedItem = nullptr;
        for (const auto& selectableAsset : selectableAssets)
        {
            QListWidgetItem* item = CreateListItem(selectableAsset);
            if (!selectedItem || m_initialSelectedAsset == selectableAsset.m_assetId)
            {
                selectedItem = item;
            }
        }

        m_ui->m_assetList->sortItems();

        if (selectedItem)
        {
            m_ui->m_assetList->setCurrentItem(selectedItem);
            m_ui->m_assetList->scrollToItem(selectedItem);
        }
    }

    AssetGridDialog::~AssetGridDialog()
    {
    }

    QListWidgetItem* AssetGridDialog::CreateListItem(const SelectableAsset& selectableAsset)
    {
        const int itemBorder = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingOrDefault<AZ::u64>("/O3DE/Atom/AtomToolsFramework/AssetGridDialog/ItemBorder", 4));
        const int itemSpacing = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingOrDefault<AZ::u64>("/O3DE/Atom/AtomToolsFramework/AssetGridDialog/ItemSpacing", 10));
        const int headerHeight = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingOrDefault<AZ::u64>("/O3DE/Atom/AtomToolsFramework/AssetGridDialog/HeaderHeight", 15));

        const QSize gridSize = m_ui->m_assetList->gridSize();
        m_ui->m_assetList->setGridSize(QSize(
            AZStd::max(gridSize.width(), m_tileSize.width() + itemSpacing),
            AZStd::max(gridSize.height(), m_tileSize.height() + itemSpacing + headerHeight)));

        QListWidgetItem* item = new QListWidgetItem(m_ui->m_assetList);
        item->setData(Qt::DisplayRole, selectableAsset.m_title);
        item->setData(Qt::UserRole, QString(selectableAsset.m_assetId.ToString<AZStd::string>().c_str()));
        item->setSizeHint(m_tileSize + QSize(itemBorder, itemBorder + headerHeight));
        m_ui->m_assetList->addItem(item);

        QWidget* itemWidget = new QWidget(m_ui->m_assetList);
        itemWidget->setLayout(new QVBoxLayout(itemWidget));
        itemWidget->layout()->setSpacing(0);
        itemWidget->layout()->setMargin(0);

        AzQtComponents::ElidingLabel* header = new AzQtComponents::ElidingLabel(itemWidget);
        header->setText(selectableAsset.m_title);
        header->setFixedSize(QSize(m_tileSize.width(), headerHeight));
        header->setMargin(0);
        header->setStyleSheet("background-color: rgb(35, 35, 35)");
        AzQtComponents::Text::addPrimaryStyle(header);
        AzQtComponents::Text::addLabelStyle(header);
        itemWidget->layout()->addWidget(header);

        AzToolsFramework::Thumbnailer::ThumbnailWidget* thumbnail = new AzToolsFramework::Thumbnailer::ThumbnailWidget(itemWidget);
        thumbnail->setFixedSize(m_tileSize);
        thumbnail->SetThumbnailKey(
            MAKE_TKEY(AzToolsFramework::AssetBrowser::ProductThumbnailKey, selectableAsset.m_assetId),
            AzToolsFramework::Thumbnailer::ThumbnailContext::DefaultContext);
        thumbnail->updateGeometry();
        itemWidget->layout()->addWidget(thumbnail);

        m_ui->m_assetList->setItemWidget(item, itemWidget);

        return item;
    }

    void AssetGridDialog::SetupAssetList()
    {
        m_ui->m_assetList->setFlow(QListView::LeftToRight);
        m_ui->m_assetList->setResizeMode(QListView::Adjust);
        m_ui->m_assetList->setGridSize(QSize(0, 0));
        m_ui->m_assetList->setWrapping(true);

        QObject::connect(m_ui->m_assetList, &QListWidget::currentItemChanged, [this](){ SelectCurrentAsset(); });
    }

    void AssetGridDialog::SetupSearchWidget()
    {
        m_ui->m_searchWidget->setReadOnly(false);
        m_ui->m_searchWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        AzQtComponents::LineEdit::applySearchStyle(m_ui->m_searchWidget);
        connect(m_ui->m_searchWidget, &QLineEdit::textChanged, this, [this](){ ApplySearchFilter(); });
        connect(m_ui->m_searchWidget, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos){ ShowSearchMenu(pos); });
    }

    void AssetGridDialog::SetupDialogButtons()
    {
        connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(this, &QDialog::rejected, this, [this](){ SelectInitialAsset(); });
    }

    void AssetGridDialog::ApplySearchFilter()
    {
        for (int index = 0; index < m_ui->m_assetList->count(); ++index)
        {
            QListWidgetItem* item = m_ui->m_assetList->item(index);
            const QString& title = item->data(Qt::DisplayRole).toString();
            const QString filter = m_ui->m_searchWidget->text();
            item->setHidden(!filter.isEmpty() && !title.contains(filter, Qt::CaseInsensitive));
        }
    }

    void AssetGridDialog::ShowSearchMenu(const QPoint& pos)
    {
        QScopedPointer<QMenu> menu(m_ui->m_searchWidget->createStandardContextMenu());
        menu->setStyleSheet("background-color: #333333");
        menu->exec(m_ui->m_searchWidget->mapToGlobal(pos));
    }

    void AssetGridDialog::SelectCurrentAsset()
    {
        auto item = m_ui->m_assetList->currentItem();
        if (item)
        {
            AZ::Data::AssetId assetId = AZ::Data::AssetId::CreateString(item->data(Qt::UserRole).toString().toUtf8().constData());
            emit AssetSelected(assetId);
        }
    }

    void AssetGridDialog::SelectInitialAsset()
    {
        emit AssetSelected(m_initialSelectedAsset);
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/AssetGridDialog/moc_AssetGridDialog.cpp>
