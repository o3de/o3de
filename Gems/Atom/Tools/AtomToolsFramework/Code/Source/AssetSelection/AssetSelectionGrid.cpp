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
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/ToolTipElement.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/StyleManager.h>
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
    // ----------------------------------------------------------------------------------
    // Flattened GUI: the header of an asset tile
    //
    // AddPath gave the header of every tile (an ElidingLabel) a sheet of one fill. The legacy GUI sets it as before; the flattened GUI
    // sets none, and this element draws what it drew: the fill, inside the margin the sheets gave every label. The sheet had no
    // selector, so it also reached the label the header keeps hidden to measure its text (never drawn), and the tooltip the header
    // shows for an elided name or a description (Qt hands a tip the sheet of the widget it is for). The tooltip element below gives
    // that tip the fill (AzQtComponents::ToolTipElement).
    // ----------------------------------------------------------------------------------

    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one translation
    //! unit, where anonymous namespaces merge.
    namespace AssetSelectionGridFlatInternal
    {
        //! background-color: AssetSelectionGrid.cpp:159 before the flattening (the form before the flattening):
        //! `header->setStyleSheet("background-color: rgb(35, 35, 35)")`. TOKENIZE: AssetSelectionGridHeaderFill
        const QColor HeaderFill(35, 35, 35);

        //! background-color: AssetSelectionGrid.cpp:266 (the fill the context menu of the search box gives itself, set as the legacy
        //! sheet of the menu). TOKENIZE: AssetSelectionGridSearchMenuFill
        constexpr QColor SearchMenuFill(0x33, 0x33, 0x33);

        //! The `background-color` of the header, over the box of every label (`.Label, QLabel { margin-top: 1px; margin-bottom: 1px }`,
        //! Text.qss:89-95, the margins at :93-94): an element is the whole definition of its frame, so it carries that margin, which
        //! Style gives a label without an element (Style::subElementRect, SE_ShapedFrameContents). The fill is drawn inside it, as the
        //! sheet drew it.
        const AzQtComponents::BoxElement Header{
            .m_name = "AtomTools.Box.AssetSelectionGridHeader",
            .m_margin = QMargins(0, AzQtComponents::Foundations::Label::MarginV, 0, AzQtComponents::Foundations::Label::MarginV),
            .m_idle = {
                .m_fill = QBrush(HeaderFill),
            },
        };

        //! The tooltip of the header (ElidingLabel::elide sets the full name, or the name and the description): the fill of the sheet
        //! (HeaderFill), which outranked `QToolTip { background-color: black }` (ToolTip.qss:13) there
        const AzQtComponents::ToolTipElement HeaderToolTip{
            .m_name = "AtomTools.ToolTip.AssetSelectionGridHeader",
            .m_fill = HeaderFill,
        };

        //! The header of a tile: an ElidingLabel (the class it is registered for) in the asset list of a grid. The grid has no other
        //! ElidingLabel; the label it keeps hidden is a plain QLabel.
        bool isHeader(const QWidget* widget)
        {
            return AzQtComponents::Selectors::within(widget, "m_assetList")
                && AzQtComponents::Selectors::withinClass(widget, "AtomToolsFramework::AssetSelectionGrid");
        }
    } // namespace AssetSelectionGridFlatInternal

    void AssetSelectionGrid::RegisterFlatElements()
    {
        static bool registered = false;
        if (!registered)
        {
            registered = true;
            AzQtComponents::BoxElement::registerElement(
                &AssetSelectionGridFlatInternal::Header, &AzQtComponents::ElidingLabel::staticMetaObject,
                &AssetSelectionGridFlatInternal::isHeader);
            AzQtComponents::ToolTipElement::registerElement(
                &AssetSelectionGridFlatInternal::HeaderToolTip, &AzQtComponents::ElidingLabel::staticMetaObject,
                &AssetSelectionGridFlatInternal::isHeader);
        }
    }

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

        // The tiles' headers are built after this, and polished when the dialog is shown
        if (AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            RegisterFlatElements();
        }

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
        m_ui->m_assetList->setCurrentItem(nullptr);
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

        const auto& pathWithAlias = GetPathWithAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(path);
        if (!QFileInfo::exists(pathWithoutAlias.c_str()))
        {
            return;
        }

        const QVariant pathItemData(QString::fromUtf8(pathWithAlias.c_str(), static_cast<int>(pathWithAlias.size())));
        const QString title(GetDisplayNameFromPath(pathWithAlias).c_str());

        // Skip creating this list item if one with the same path is already registered
        for (int i = 0; i < m_ui->m_assetList->count(); ++i)
        {
            QListWidgetItem* item = m_ui->m_assetList->item(i);
            if (pathItemData == item->data(Qt::UserRole))
            {
                return;
            }
        }

        // Compare the item title against all other items and append a suffix until the new title is unique
        QString uniqueTitle = title;
        int uniqueTitleSuffix = 0;
        bool uniqueTitleFound = true;
        while (uniqueTitleFound)
        {
            uniqueTitleFound = false;
            for (int i = 0; i < m_ui->m_assetList->count(); ++i)
            {
                QListWidgetItem* item = m_ui->m_assetList->item(i);
                if (uniqueTitle == item->data(Qt::DisplayRole))
                {
                    uniqueTitle = title + QString(" (%1)").arg(++uniqueTitleSuffix);
                    uniqueTitleFound = true;
                    break;
                }
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
        item->setData(Qt::DisplayRole, uniqueTitle);
        item->setData(Qt::UserRole, pathItemData);
        item->setData(Qt::ToolTipRole, pathWithoutAlias.c_str());
        item->setSizeHint(m_tileSize + QSize(itemBorder, itemBorder + headerHeight));
        m_ui->m_assetList->addItem(item);

        QWidget* itemWidget = new QWidget(m_ui->m_assetList);
        itemWidget->setLayout(new QVBoxLayout(itemWidget));
        itemWidget->layout()->setSpacing(0);
        itemWidget->layout()->setContentsMargins(0, 0, 0, 0);

        AzQtComponents::ElidingLabel* header = new AzQtComponents::ElidingLabel(itemWidget);
        header->setText(uniqueTitle);
        header->setFixedSize(QSize(m_tileSize.width(), headerHeight));
        header->setContentsMargins(0, 0, 0, 0);
        // The fill of the legacy GUI; the flattened GUI sets no sheet and draws it with the element of the header
        AzQtComponents::StyleManager::setLegacyStyleSheet(header,
            QStringLiteral("background-color: %1").arg(AssetSelectionGridFlatInternal::HeaderFill.name()));
        AzQtComponents::Text::addPrimaryStyle(header);
        AzQtComponents::Text::addLabelStyle(header);
        itemWidget->layout()->addWidget(header);

        bool result = false;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, pathWithoutAlias.c_str(), assetInfo, watchFolder);

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
        const auto& pathWithAlias = GetPathWithAlias(path);
        const QVariant pathItemData(QString::fromUtf8(pathWithAlias.c_str(), static_cast<int>(pathWithAlias.size())));
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
        const auto& pathWithAlias = GetPathWithAlias(path);
        const QVariant pathItemData(QString::fromUtf8(pathWithAlias.c_str(), static_cast<int>(pathWithAlias.size())));
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

        // The fill the menu gave itself, `menu->setStyleSheet("background-color: #333333")` (the form before the flattening): the legacy
        // GUI sets it as before. The flattened GUI sets none, and the menu is marked for the element that draws it (MenuElement)
        AzQtComponents::MenuElement::setFilledPopupSheet(menu.data(),
            QStringLiteral("background-color: %1").arg(AssetSelectionGridFlatInternal::SearchMenuFill.name()));

        menu->exec(m_ui->m_searchWidget->mapToGlobal(pos));
    }
} // namespace AtomToolsFramework

