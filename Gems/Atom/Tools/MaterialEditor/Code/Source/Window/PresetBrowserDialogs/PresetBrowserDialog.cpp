/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Util/Util.h>
#include <AzFramework/Application/Application.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <AzToolsFramework/Thumbnails/ThumbnailWidget.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <Window/PresetBrowserDialogs/PresetBrowserDialog.h>

#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QVBoxLayout>

namespace MaterialEditor
{
    PresetBrowserDialog::PresetBrowserDialog(QWidget* parent)
        : QDialog(parent)
        , m_ui(new Ui::PresetBrowserDialog)
    {
        m_ui->setupUi(this);

        QSignalBlocker signalBlocker(this);

        SetupPresetList();
        SetupSearchWidget();
        SetupDialogButtons();
        setModal(true);
    }

    void PresetBrowserDialog::SetupPresetList()
    {
        m_ui->m_presetList->setFlow(QListView::LeftToRight);
        m_ui->m_presetList->setResizeMode(QListView::Adjust);
        m_ui->m_presetList->setGridSize(QSize(0, 0));
        m_ui->m_presetList->setWrapping(true);

        QObject::connect(m_ui->m_presetList, &QListWidget::currentItemChanged, [this](){ SelectCurrentPreset(); });
    }

    QListWidgetItem* PresetBrowserDialog::CreateListItem(const QString& title, const AZ::Data::AssetId& assetId, const QSize& size)
    {
        const int itemBorder = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingOrDefault<AZ::u64>("/O3DE/Atom/MaterialEditor/PresetBrowserDialog/ItemBorder", 4));
        const int itemSpacing = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingOrDefault<AZ::u64>("/O3DE/Atom/MaterialEditor/PresetBrowserDialog/ItemSpacing", 10));
        const int headerHeight = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingOrDefault<AZ::u64>("/O3DE/Atom/MaterialEditor/PresetBrowserDialog/HeaderHeight", 15));

        const QSize gridSize = m_ui->m_presetList->gridSize();
        m_ui->m_presetList->setGridSize(QSize(
            AZStd::max(gridSize.width(), size.width() + itemSpacing),
            AZStd::max(gridSize.height(), size.height() + itemSpacing + headerHeight)));

        QListWidgetItem* item = new QListWidgetItem(m_ui->m_presetList);
        item->setData(Qt::UserRole, title);
        item->setSizeHint(size + QSize(itemBorder, itemBorder + headerHeight));
        m_ui->m_presetList->addItem(item);

        QWidget* itemWidget = new QWidget(m_ui->m_presetList);
        itemWidget->setLayout(new QVBoxLayout(itemWidget));
        itemWidget->layout()->setSpacing(0);
        itemWidget->layout()->setMargin(0);

        AzQtComponents::ElidingLabel* header = new AzQtComponents::ElidingLabel(itemWidget);
        header->setText(title);
        header->setFixedSize(QSize(size.width(), headerHeight));
        header->setMargin(0);
        header->setStyleSheet("background-color: rgb(35, 35, 35)");
        AzQtComponents::Text::addPrimaryStyle(header);
        AzQtComponents::Text::addLabelStyle(header);
        itemWidget->layout()->addWidget(header);

        AzToolsFramework::Thumbnailer::ThumbnailWidget* thumbnail = new AzToolsFramework::Thumbnailer::ThumbnailWidget(itemWidget);
        thumbnail->setFixedSize(size);
        thumbnail->SetThumbnailKey(
            MAKE_TKEY(AzToolsFramework::AssetBrowser::ProductThumbnailKey, assetId),
            AzToolsFramework::Thumbnailer::ThumbnailContext::DefaultContext);
        thumbnail->updateGeometry();
        itemWidget->layout()->addWidget(thumbnail);

        m_ui->m_presetList->setItemWidget(item, itemWidget);

        return item;
    }

    void PresetBrowserDialog::SetupSearchWidget()
    {
        m_ui->m_searchWidget->setReadOnly(false);
        m_ui->m_searchWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        AzQtComponents::LineEdit::applySearchStyle(m_ui->m_searchWidget);
        connect(m_ui->m_searchWidget, &QLineEdit::textChanged, this, [this](){ ApplySearchFilter(); });
        connect(m_ui->m_searchWidget, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos){ ShowSearchMenu(pos); });
    }

    void PresetBrowserDialog::SetupDialogButtons()
    {
        connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(this, &QDialog::rejected, this, [this](){ SelectInitialPreset(); });
    }

    void PresetBrowserDialog::ApplySearchFilter()
    {
        for (int index = 0; index < m_ui->m_presetList->count(); ++index)
        {
            QListWidgetItem* item = m_ui->m_presetList->item(index);
            const QString& title = item->data(Qt::UserRole).toString();
            const QString filter = m_ui->m_searchWidget->text();
            item->setHidden(!filter.isEmpty() && !title.contains(filter, Qt::CaseInsensitive));
        }
    }

    void PresetBrowserDialog::ShowSearchMenu(const QPoint& pos)
    {
        QScopedPointer<QMenu> menu(m_ui->m_searchWidget->createStandardContextMenu());
        menu->setStyleSheet("background-color: #333333");
        menu->exec(m_ui->m_searchWidget->mapToGlobal(pos));
    }
} // namespace MaterialEditor

#include <Window/PresetBrowserDialogs/moc_PresetBrowserDialog.cpp>
