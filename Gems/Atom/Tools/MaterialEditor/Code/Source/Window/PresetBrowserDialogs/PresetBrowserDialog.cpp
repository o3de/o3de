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
        const QSize gridSize = m_ui->m_presetList->gridSize();
        m_ui->m_presetList->setGridSize(
            QSize(AZStd::max(gridSize.width(), size.width() + 10), AZStd::max(gridSize.height(), size.height() + 10)));

        QListWidgetItem* item = new QListWidgetItem(m_ui->m_presetList);
        item->setData(Qt::UserRole, title);
        item->setSizeHint(size + QSize(4, 4));
        m_ui->m_presetList->addItem(item);

        AzToolsFramework::Thumbnailer::ThumbnailWidget* thumbnail = new AzToolsFramework::Thumbnailer::ThumbnailWidget(m_ui->m_presetList);
        thumbnail->setFixedSize(size);
        thumbnail->SetThumbnailKey(
            MAKE_TKEY(AzToolsFramework::AssetBrowser::ProductThumbnailKey, assetId),
            AzToolsFramework::Thumbnailer::ThumbnailContext::DefaultContext);
        thumbnail->updateGeometry();

        AzQtComponents::ElidingLabel* previewLabel = new AzQtComponents::ElidingLabel(thumbnail);
        previewLabel->setText(title);
        previewLabel->setFixedSize(QSize(size.width(), 15));
        previewLabel->setMargin(0);
        previewLabel->setStyleSheet("background-color: rgb(35, 35, 35)");
        AzQtComponents::Text::addPrimaryStyle(previewLabel);
        AzQtComponents::Text::addLabelStyle(previewLabel);

        m_ui->m_presetList->setItemWidget(item, thumbnail);

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
