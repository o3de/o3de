/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>
#include <AzCore/Utils/Utils.h>
#include <AzQtComponents/Components/StyledBusyLabel.h>
#include <AzToolsFramework/Editor/RichTextHighlighter.h>

#include <QApplication>
#include <QTextDocument>
#include <QLineEdit>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // 4251: class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
                                                               // 4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QAbstractItemView>
#include <QPainter>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        static constexpr const char* TreeIconPathFirst = "Assets/Editor/Icons/AssetBrowser/TreeBranch_First.svg";
        static constexpr const char* TreeIconPathMiddle = "Assets/Editor/Icons/AssetBrowser/TreeBranch_Middle.svg";
        static constexpr const char* TreeIconPathLast = "Assets/Editor/Icons/AssetBrowser/TreeBranch_Last.svg";
        static constexpr const char* TreeIconPathOneChild = "Assets/Editor/Icons/AssetBrowser/TreeBranch_OneChild.svg";

        const int EntrySpacingLeftPixels = 8;
        const int EntryIconMarginLeftPixels = 2;

        EntryDelegate::EntryDelegate(QWidget* parent)
            : QStyledItemDelegate(parent)
            , m_iconSize(qApp->style()->pixelMetric(QStyle::PM_SmallIconSize))
        {
        }

        EntryDelegate::~EntryDelegate() = default;

        QSize EntryDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            QSize baseHint = QStyledItemDelegate::sizeHint(option, index);
            if (baseHint.height() < m_iconSize)
            {
                baseHint.setHeight(m_iconSize);
            }
            return baseHint;
        }

        QWidget* EntryDelegate::createEditor(QWidget* parent, [[ maybe_unused]] const QStyleOptionViewItem& option, [[maybe_unused]] const QModelIndex& index) const
        {
            QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
            if (auto* lineEdit = qobject_cast<QLineEdit*>(widget))
            {
                connect(lineEdit, &QLineEdit::editingFinished,
                    this, [this]()
                    {
                        auto sendingLineEdit = qobject_cast<QLineEdit*>(sender());
                        if (sendingLineEdit)
                        {
                            emit RenameEntry(sendingLineEdit->text());
                        }
                    });
            }
            return widget;
        }

        void EntryDelegate::paintAssetBrowserEntry(QPainter* painter, int column, const AssetBrowserEntry* entry, const QStyleOptionViewItem& option) const
        {
            // Draw main entry thumbnail.
            QRect remainingRect(option.rect);
            remainingRect.adjust(EntryIconMarginLeftPixels, 0, 0, 0); // bump it rightwards to give some margin to the icon.

            QSize iconSize(m_iconSize, m_iconSize);
            // Note that the thumbnail might actually be smaller than the row if theres a lot of padding or font size
            // so it needs to center vertically with padding in that case:
            QPoint iconTopLeft(remainingRect.x(), remainingRect.y() + (remainingRect.height() / 2) - (m_iconSize / 2));

            QStyleOptionViewItem actualOption = option;

            auto sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
            if (column == aznumeric_cast<int>(AssetBrowserEntry::Column::Name))
            {
                int thumbX = DrawThumbnail(painter, iconTopLeft, iconSize, entry);
                if (sourceEntry)
                {
                    if (m_showSourceControl)
                    {
                        DrawThumbnail(painter, iconTopLeft, iconSize, entry);
                    }
                    // sources with no children should be greyed out.
                    if (sourceEntry->GetChildCount() == 0)
                    {
                        actualOption.state &= QStyle::State_Enabled;
                    }
                }

                remainingRect.adjust(thumbX, 0, 0, 0); // bump it to the right by the size of the thumbnail
                remainingRect.adjust(EntrySpacingLeftPixels, 0, 0, 0); // bump it to the right by the spacing.
            }
            // for display use the display name when the name column is aksed for, otherwise use the path.
            QString displayString = column == aznumeric_cast<int>(AssetBrowserEntry::Column::Name)
                ? qvariant_cast<QString>(entry->data(aznumeric_cast<int>(AssetBrowserEntry::Column::DisplayName)))
                : qvariant_cast<QString>(entry->data(aznumeric_cast<int>(AssetBrowserEntry::Column::Path)));

            if (!m_searchString.isEmpty())
            {
                displayString = AzToolsFramework::RichTextHighlighter::HighlightText(displayString, m_searchString);
            }

            RichTextHighlighter::PaintHighlightedRichText(displayString, painter, actualOption, remainingRect);
        }

        void EntryDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            auto data = index.data(AssetBrowserModel::Roles::EntryRole);
            if (data.canConvert<const AssetBrowserEntry*>())
            {
                QStyle* style = option.widget ? option.widget->style() : QApplication::style();

                // draw the background
                if (!index.parent().isValid() && !(option.state & QStyle::State_MouseOver) && !(option.state & QStyle::State_Selected))
                {
                    painter->fillRect(option.rect, 0x333333);
                }
                else
                {
                    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);
                }

                auto entry = qvariant_cast<const AssetBrowserEntry*>(data);


                paintAssetBrowserEntry(painter, index.column(), entry, option);
            }
        }

        void EntryDelegate::SetShowSourceControlIcons(bool showSourceControl)
        {
            m_showSourceControl = showSourceControl;
        }

        void EntryDelegate::SetShowFavoriteIcons(bool showFavoriteIcons)
        {
            m_showFavoriteIcons = showFavoriteIcons;
        }

        int EntryDelegate::DrawThumbnail(QPainter* painter, const QPoint& point, const QSize& size, const AssetBrowserEntry* entry) const
        {
            if (!m_showSourceControl)
            {
                const auto& qVariant = AssetBrowserViewUtils::GetThumbnail(entry, false, m_showFavoriteIcons);
                if (const auto& path = qVariant.value<QString>(); !path.isEmpty())
                {
                    QIcon icon;
                    icon.addFile(path, size, QIcon::Normal, QIcon::Off);
                    icon.paint(painter, QRect(point.x(), point.y(), size.width(), size.height()));
                }
                else if (const auto& pixmap = qVariant.value<QPixmap>(); !pixmap.isNull())
                {
                    // Scaling and centering pixmap within bounds to preserve aspect ratio
                    const QPixmap pixmapScaled = pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    const QSize sizeDelta = size - pixmapScaled.size();
                    const QPoint pointDelta = QPoint(sizeDelta.width() / 2, sizeDelta.height() / 2);
                    painter->drawPixmap(point + pointDelta, pixmapScaled);
                }
            }
            else if (auto sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry))
            {
                Thumbnailer::SharedThumbnailKey thumbnailKey = sourceEntry->GetSourceControlThumbnailKey();
                SharedThumbnail thumbnail;
                ThumbnailerRequestBus::BroadcastResult(thumbnail, &ThumbnailerRequests::GetThumbnail, thumbnailKey);
                AZ_Assert(thumbnail, "The shared thumbnail was not available from the ThumbnailerRequestBus.");
                AZ_Assert(painter, "A null QPainter was passed in to DrawThumbnail.");
                if (!painter || !thumbnail || thumbnail->GetState() == Thumbnail::State::Failed)
                {
                    return 0;
                }

                const Thumbnail::State thumbnailState = thumbnail->GetState();
                if (thumbnailState == Thumbnail::State::Loading)
                {
                    AzQtComponents::StyledBusyLabel* busyLabel;
                    AssetBrowserComponentRequestBus::BroadcastResult(busyLabel, &AssetBrowserComponentRequests::GetStyledBusyLabel);
                    if (busyLabel)
                    {
                        busyLabel->DrawTo(painter, QRectF(point.x(), point.y(), size.width(), size.height()));
                    }
                }
                else if (thumbnailState == Thumbnail::State::Ready)
                {
                    // Scaling and centering pixmap within bounds to preserve aspect ratio
                    const QPixmap pixmap = thumbnail->GetPixmap().scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    const QSize sizeDelta = size - pixmap.size();
                    const QPoint pointDelta = QPoint(sizeDelta.width() / 2, sizeDelta.height() / 2);
                    painter->drawPixmap(point + pointDelta, pixmap);
                }
                else
                {
                    AZ_Assert(false, "Thumbnail state %d unexpected here", int(thumbnailState));
                }
            }
            return m_iconSize;
        }

        void EntryDelegate::SetSearchString(const QString& searchString)
        {
            m_searchString = searchString;
        }

        SearchEntryDelegate::SearchEntryDelegate(QWidget* parent)
            : EntryDelegate(parent)
        {
            LoadBranchPixMaps();
        }

        void SearchEntryDelegate::Init()
        {
            AssetBrowserModel* assetBrowserModel;
            AssetBrowserComponentRequestBus::BroadcastResult(assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
            AZ_Assert(assetBrowserModel, "Failed to get filebrowser model");
            m_assetBrowserFilerModel = assetBrowserModel->GetFilterModel();
        }

        void SearchEntryDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            auto data = index.data(AssetBrowserModel::Roles::EntryRole);
            if (data.canConvert<const AssetBrowserEntry*>())
            {

                QStyle* style = option.widget ? option.widget->style() : QApplication::style();

                // draw the background
                style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);

                // Draw main entry thumbnail.
                QRect remainingRect(option.rect);

                QSize iconSize(m_iconSize, m_iconSize);
                // Note that the thumbnail might actually be smaller than the row if theres a lot of padding or font size
                // so it needs to center vertically with padding in that case:
                QPoint iconTopLeft;
                QPoint branchIconTopLeft = QPoint();

                auto entry = qvariant_cast<const AssetBrowserEntry*>(data);
                auto sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry);

                //If it is a SourceEntry or it is not the column name we don't want to add space for the branch Icon
                if (sourceEntry || index.column() != aznumeric_cast<int>(AssetBrowserEntry::Column::Name))
                {
                    remainingRect.adjust(EntryIconMarginLeftPixels, 0, 0, 0); // bump it rightwards to give some margin to the icon.
                    iconTopLeft = QPoint(remainingRect.x(), remainingRect.y() + (remainingRect.height() / 2) - (m_iconSize / 2));
                }
                else
                {
                    remainingRect.adjust(EntryIconMarginLeftPixels + m_iconSize, 0, 0, 0); // bump it rightwards to give some margin to the icon.
                    iconTopLeft = QPoint(remainingRect.x() / 2 + m_iconSize, remainingRect.y() + (remainingRect.height() / 2) - (m_iconSize / 2));
                    branchIconTopLeft = QPoint((remainingRect.x() / 2) - 2, remainingRect.y() + (remainingRect.height() / 2) - (m_iconSize / 2));
                }

                QPalette actualPalette(option.palette);

                if (index.column() == aznumeric_cast<int>(AssetBrowserEntry::Column::Name))
                {
                    int thumbX = DrawThumbnail(painter, iconTopLeft, iconSize, entry);
                    if (sourceEntry)
                    {
                        if (m_showSourceControl)
                        {
                            DrawThumbnail(painter, iconTopLeft, iconSize, sourceEntry);
                        }
                        // sources with no children should be greyed out.
                        if (sourceEntry->GetChildCount() == 0)
                        {
                            actualPalette.setCurrentColorGroup(QPalette::Disabled);
                        }
                    }
                    else
                    {
                        //Get the indexes above and below our entry to see what type are they.
                        QAbstractItemView* view = qobject_cast<QAbstractItemView*>(option.styleObject);
                        const QAbstractItemModel* viewModel = view->model();

                        const QModelIndex indexBelow = viewModel->index(index.row() + 1, index.column());
                        const QModelIndex indexAbove = viewModel->index(index.row() - 1, index.column());

                        auto belowEntry = qvariant_cast<const AssetBrowserEntry*>(indexBelow.data(AssetBrowserModel::Roles::EntryRole));
                        auto aboveEntry = qvariant_cast<const AssetBrowserEntry*>(indexAbove.data(AssetBrowserModel::Roles::EntryRole));

                        auto belowSourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(belowEntry);
                        auto aboveSourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(aboveEntry);

                        // Last item and the above entry is a source entry
                        // or indexBelow is a source entry and the index above is not
                        if (viewModel->rowCount() > 0 && index.row() == viewModel->rowCount() - 1)
                        {
                            if (aboveSourceEntry)
                            {
                                DrawBranchPixMap(EntryBranchType::OneChild, painter, branchIconTopLeft, iconSize);
                            }
                            else
                            {
                                DrawBranchPixMap(EntryBranchType::Last, painter, branchIconTopLeft, iconSize);
                            }
                        }
                        else if (belowSourceEntry && aboveSourceEntry)
                        {
                            DrawBranchPixMap(EntryBranchType::OneChild, painter, branchIconTopLeft, iconSize); // Draw One Child Icon
                        }
                        else if (belowSourceEntry && !aboveSourceEntry)
                        {
                            DrawBranchPixMap(EntryBranchType::Last, painter, branchIconTopLeft, iconSize);
                        }
                        else if (aboveSourceEntry) // The index above is a source entry
                        {
                            DrawBranchPixMap(EntryBranchType::First, painter, branchIconTopLeft, iconSize); // Draw First Child Icon
                        }
                        else //the index above and below are also child entries
                        {
                            DrawBranchPixMap(EntryBranchType::Middle, painter, branchIconTopLeft, iconSize); // Draw Default child Icon.
                        }
                    }

                    remainingRect.adjust(thumbX, 0, 0, 0); // bump it to the right by the size of the thumbnail
                    remainingRect.adjust(EntrySpacingLeftPixels, 0, 0, 0); // bump it to the right by the spacing.
                }
                // if we are asking for the name, use the display name.  Else we must be asking for the path, so use that.
                QString displayString = index.column() == aznumeric_cast<int>(AssetBrowserEntry::Column::Name)
                    ? qvariant_cast<QString>(entry->data(aznumeric_cast<int>(AssetBrowserEntry::Column::DisplayName)))
                    : qvariant_cast<QString>(entry->data(aznumeric_cast<int>(AssetBrowserEntry::Column::Path)));

                QStyleOptionViewItem optionV4{ option };
                initStyleOption(&optionV4, index);
                optionV4.state &= ~(QStyle::State_HasFocus | QStyle::State_Selected);

                if (m_assetBrowserFilerModel && m_assetBrowserFilerModel->GetStringFilter()
                    && !m_assetBrowserFilerModel->GetStringFilter()->GetFilterString().isEmpty())
                {
                    displayString = RichTextHighlighter::HighlightText(displayString, m_assetBrowserFilerModel->GetStringFilter()->GetFilterString());
                }

                RichTextHighlighter::PaintHighlightedRichText(displayString, painter, optionV4, remainingRect);
            }
        }

        void SearchEntryDelegate::LoadBranchPixMaps()
        {
            AZ::IO::BasicPath<AZ::IO::FixedMaxPathString> absoluteIconPath;
            for (int branchType = EntryBranchType::First; branchType != EntryBranchType::Count; ++branchType)
            {
                QPixmap pixmap;
                switch (branchType)
                {
                case AzToolsFramework::AssetBrowser::EntryBranchType::First:
                    absoluteIconPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / TreeIconPathFirst;
                    break;
                case AzToolsFramework::AssetBrowser::EntryBranchType::Middle:
                    absoluteIconPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / TreeIconPathMiddle;
                    break;
                case AzToolsFramework::AssetBrowser::EntryBranchType::Last:
                    absoluteIconPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / TreeIconPathLast;
                    break;
                case AzToolsFramework::AssetBrowser::EntryBranchType::OneChild:
                    absoluteIconPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / TreeIconPathOneChild;
                    break;
                }
                [[maybe_unused]] bool pixmapLoadedSuccess = pixmap.load(absoluteIconPath.c_str());
                AZ_Assert(pixmapLoadedSuccess, "Error loading Branch Icons in SearchEntryDelegate");

                m_branchIcons[static_cast<EntryBranchType>(branchType)] = pixmap;

            }
        }

        void SearchEntryDelegate::DrawBranchPixMap(
            EntryBranchType branchType, QPainter* painter, const QPoint& point, const QSize& size) const
        {
            const QPixmap& pixmap = m_branchIcons[branchType];

            pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            const QSize sizeDelta = size - pixmap.size();
            const QPoint pointDelta = QPoint(sizeDelta.width() / 2, sizeDelta.height() / 2);
            painter->drawPixmap(point + pointDelta, pixmap);
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include "AssetBrowser/Views/moc_EntryDelegate.cpp"
