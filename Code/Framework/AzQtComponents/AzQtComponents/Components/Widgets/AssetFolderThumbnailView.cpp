/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>

#include <AzQtComponents/Components/Style.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option") // 4244: 'initializing': conversion from 'int' to 'float', possible loss of data
                                                                    // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
                                                                    // 4800: 'QFlags<QPainter::RenderHint>::Int': forcing value to bool 'true' or 'false' (performance warning)
#include <QAbstractItemDelegate>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSettings>
#include <QMenu>
AZ_POP_DISABLE_WARNING

namespace
{
    QString elidedTextWithExtension(const QFontMetrics& fm, const QString& text, int width)
    {
        if (fm.horizontalAdvance(text) <= width)
            return text;

        const int dot = text.lastIndexOf(QLatin1Char{'.'});
        if (dot != -1)
        {
            const auto baseName = text.left(dot);
            const auto extension = text.mid(dot + 1);
            return fm.elidedText(baseName, Qt::ElideRight, width - fm.horizontalAdvance(extension)) + extension;
        }
        else
        {
            return fm.elidedText(text, Qt::ElideRight, width);
        }
    }
}

namespace AzQtComponents
{
    static void paintExpandButton(QPainter* painter, const QRect& rect, bool closed, const AssetFolderThumbnailView::Config::ExpandButton& config)
    {
        // rectangle
        painter->setPen(Qt::NoPen);

        if (!closed)
        {
            painter->setBrush(config.backgroundColor);
            painter->drawRoundedRect(rect, config.borderRadius, config.borderRadius);
            // Remove rounded border from the left hand side
            painter->drawRect(rect.left(), rect.top(), config.borderRadius, rect.height());
        }

        // caret

        const auto caretWidth = config.caretWidth;
        const auto caretHeight = config.caretHeight;
        const auto center = QRectF{ rect }.center();

        const auto caretDirection = closed ? 1.0 : -1.0;

        QPolygonF caret;
        caret.append(center + QPointF{ -.5 * caretWidth * caretDirection, -caretHeight / 2 });
        caret.append(center + QPointF{ -.5 * caretWidth * caretDirection, caretHeight / 2 });
        caret.append(center + QPointF{ .5 * caretWidth * caretDirection, 0 });

        painter->setBrush(config.caretColor);
        painter->drawConvexPolygon(caret);
    }

    class AssetFolderThumbnailViewDelegate : public QAbstractItemDelegate
    {
    public:
        explicit AssetFolderThumbnailViewDelegate(QObject* parent = nullptr);

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        void polish(const AssetFolderThumbnailView::Config& config);

    private:
        AssetFolderThumbnailView::Config m_config;
    };

    AssetFolderThumbnailViewDelegate::AssetFolderThumbnailViewDelegate(QObject* parent)
        : QAbstractItemDelegate(parent)
    {
    }

    void AssetFolderThumbnailViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        const auto isTopLevel = index.data(static_cast<int>(AssetFolderThumbnailView::Role::IsTopLevel)).value<bool>();

        painter->save();

        painter->setFont(option.font);

        const auto& config = isTopLevel ? m_config.rootThumbnail : m_config.childThumbnail;

        const auto borderRadius = config.borderRadius;
        const auto padding = config.padding;
        const auto& rect = option.rect;

        const auto thumbnailRect = isTopLevel
            ? QRect{ rect.left(), rect.top(), rect.width(), rect.height() - 4 - option.fontMetrics.height() }
            : QRect{ rect.left(), rect.top(), rect.width(), rect.height() - m_config.childFrame.padding - 5 - option.fontMetrics.height() };
        const auto imageRect = thumbnailRect.adjusted(padding, padding, -padding, -padding);

        // border

        const auto halfBorderThickness = qMax(config.borderThickness, config.selectedBorderThickness) / 2;
        const auto borderRect = QRectF{thumbnailRect}.adjusted(halfBorderThickness, halfBorderThickness, -halfBorderThickness, -halfBorderThickness);
        QPen borderPen;
        if (option.state & QStyle::State_Selected)
        {
            borderPen = {config.selectedBorderColor, config.selectedBorderThickness};
        }
        else
        {
            borderPen = {config.borderColor, config.borderThickness};
        }
        painter->setBrush(config.backgroundColor);
        painter->setPen(borderPen);
        painter->drawRoundedRect(borderRect, borderRadius, borderRadius);

        // pixmap

        const auto& iconVariant = index.data(Qt::DecorationRole);
        if (!iconVariant.isNull())
        {
            const auto& icon = iconVariant.value<QIcon>();
            icon.paint(painter, imageRect);
        }

        if (isTopLevel)
        {
            // draw top right status icon if there's data in the corresponding role
            if (index.data(Qt::StatusTipRole).isValid())
            {
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor::fromRgb(0x22, 0x22, 0x22));
                painter->drawRect(rect.right() - config.borderThickness + 1 - 16, rect.top() + config.borderThickness, 16, 16);
            }

            // expand button

            if (option.state & QStyle::State_DownArrow)
            {
                const auto& buttonConfig = m_config.expandButton;
                const auto width = buttonConfig.width;
                const auto buttonRect = QRect{rect.left() + rect.width() - width, rect.top(), width, rect.width()};
                paintExpandButton(painter, buttonRect, (option.state & QStyle::State_DownArrow) != 0, buttonConfig);
            }

            // text

            const auto textHeight = option.fontMetrics.height();
            const auto textRect = QRect{rect.left(), rect.bottom() - textHeight, rect.width(), textHeight};

            painter->setPen(option.palette.color(QPalette::Text));
            painter->drawText(
                textRect,
                elidedTextWithExtension(option.fontMetrics, index.data().toString(), textRect.width()),
                QTextOption{ option.decorationAlignment });
        }
        else
        {
            // text

            const auto textHeight = option.fontMetrics.height();
            const auto textRect = QRect{ rect.left(), rect.bottom() - textHeight, rect.width(), textHeight };

            painter->setPen(option.palette.color(QPalette::Text));
            painter->drawText(
                textRect,
                elidedTextWithExtension(option.fontMetrics, index.data().toString(), textRect.width()),
                QTextOption{ option.decorationAlignment });
        }

        painter->restore();
    }

    QSize AssetFolderThumbnailViewDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        Q_UNUSED(index);
        return option.rect.size();
    }

    void AssetFolderThumbnailViewDelegate::polish(const AssetFolderThumbnailView::Config& config)
    {
        m_config = config;
    }

    static void readColor(QSettings& settings, const QString& name, QColor& color)
    {
        // only overwrite the value if it's set; otherwise, it'll stay the default
        if (settings.contains(name))
        {
            color = QColor(settings.value(name).toString());
        }
    }

    static void readThumbnail(QSettings& settings, AssetFolderThumbnailView::Config::Thumbnail& thumbnail)
    {
        thumbnail.width = settings.value(QStringLiteral("Width"), thumbnail.width).toInt();
        thumbnail.height = settings.value(QStringLiteral("Height"), thumbnail.height).toInt();
        thumbnail.borderRadius = settings.value(QStringLiteral("BorderRadius"), thumbnail.borderRadius).toReal();
        thumbnail.padding = settings.value(QStringLiteral("Padding"), thumbnail.padding).toInt();
        readColor(settings, "BackgroundColor", thumbnail.backgroundColor);
        thumbnail.borderThickness = settings.value(QStringLiteral("BorderThickness"), thumbnail.borderThickness).toReal();
        thumbnail.selectedBorderThickness = settings.value(QStringLiteral("SelectedBorderThickness"), thumbnail.selectedBorderThickness).toReal();
        readColor(settings, "BorderColor", thumbnail.borderColor);
        readColor(settings, "SelectedBorderColor", thumbnail.selectedBorderColor);
    }

    static void readExpandButton(QSettings& settings, AssetFolderThumbnailView::Config::ExpandButton& expandButton)
    {
        expandButton.width = settings.value(QStringLiteral("Width"), expandButton.width).toInt();
        expandButton.caretWidth = settings.value(QStringLiteral("CaretWidth"), expandButton.caretWidth).toReal();
        expandButton.caretHeight = settings.value(QStringLiteral("CaretHeight"), expandButton.caretHeight).toReal();
        expandButton.borderRadius = settings.value(QStringLiteral("BorderRadius"), expandButton.borderRadius).toReal();
        readColor(settings, "BackgroundColor", expandButton.backgroundColor);
        readColor(settings, "CaretColor", expandButton.caretColor);
    }

    static void readChildFrame(QSettings& settings, AssetFolderThumbnailView::Config::ChildFrame& childFrame)
    {
        childFrame.padding = settings.value(QStringLiteral("Padding"), childFrame.padding).toInt();
        childFrame.borderRadius = settings.value(QStringLiteral("BorderRadius"), childFrame.borderRadius).toReal();
        readColor(settings, "BackgroundColor", childFrame.backgroundColor);
        childFrame.closeButtonWidth = settings.value(QStringLiteral("CloseButtonWidth"), childFrame.padding).toInt();
    }

    AssetFolderThumbnailView::Config AssetFolderThumbnailView::loadConfig(QSettings& settings)
    {
        auto config = defaultConfig();

        config.viewportPadding = settings.value(QStringLiteral("ViewportPadding"), config.viewportPadding).toInt();
        config.topItemsHorizontalSpacing =
            settings.value(QStringLiteral("TopItemsHorizontalSpacing"), config.topItemsHorizontalSpacing).toInt();
        config.topItemsVerticalSpacing = settings.value(QStringLiteral("TopItemsVerticalSpacing"), config.topItemsVerticalSpacing).toInt();
        config.childrenItemsHorizontalSpacing =
            settings.value(QStringLiteral("ChildrenItemsHorizontalSpacing"), config.childrenItemsHorizontalSpacing).toInt();

        settings.beginGroup(QStringLiteral("RootThumbnail"));
        readThumbnail(settings, config.rootThumbnail);
        settings.endGroup();

        settings.beginGroup(QStringLiteral("ChildThumbnail"));
        readThumbnail(settings, config.childThumbnail);
        settings.endGroup();

        settings.beginGroup(QStringLiteral("ExpandButton"));
        readExpandButton(settings, config.expandButton);
        settings.endGroup();

        settings.beginGroup(QStringLiteral("ChildFrame"));
        readChildFrame(settings, config.childFrame);
        settings.endGroup();

        return config;
    }

    AssetFolderThumbnailView::Config AssetFolderThumbnailView::defaultConfig()
    {
        Config config;

        config.viewportPadding = 8;
        config.topItemsHorizontalSpacing = 18;
        config.topItemsVerticalSpacing = 18;
        config.childrenItemsHorizontalSpacing = 7;

        config.rootThumbnail.width = 96;
        config.rootThumbnail.height = 96;
        config.rootThumbnail.borderRadius = 2.0;
        config.rootThumbnail.padding = 12;
        config.rootThumbnail.backgroundColor = QColor(0x44, 0x44, 0x44);
        config.rootThumbnail.borderThickness = 1.0;
        config.rootThumbnail.selectedBorderThickness = 1.0;
        config.rootThumbnail.borderColor = QColor(0x22, 0x22, 0x22);
        config.rootThumbnail.selectedBorderColor = QColor(30, 112, 235);

        config.childThumbnail.width = 70;
        config.childThumbnail.height = 80;
        config.childThumbnail.borderRadius = 2.0;
        config.childThumbnail.padding = 9;
        config.childThumbnail.backgroundColor = QColor(0x22, 0x22, 0x22);
        config.childThumbnail.borderThickness = 1.0;
        config.childThumbnail.selectedBorderThickness = 1.0;
        config.childThumbnail.borderColor = QColor(0x38, 0x38, 0x38);
        config.childThumbnail.selectedBorderColor = QColor(0xff, 0xff, 0xff);

        config.expandButton.width = 15;
        config.expandButton.caretWidth = 8.0;
        config.expandButton.borderRadius = 2.0;
        config.expandButton.backgroundColor = QColor(0x69, 0x69, 0x69);
        config.expandButton.caretColor = QColor(0xff, 0xff, 0xff);

        config.childFrame.padding = 18;
        config.childFrame.borderRadius = 4.0;
        config.childFrame.borderColor = QColor(0x22, 0x22, 0x22);
        config.childFrame.backgroundColor = QColor(0x44, 0x44, 0x44);
        config.childFrame.closeButtonWidth = 15;

        return config;
    }

    bool AssetFolderThumbnailView::polish(Style* style, QWidget* widget, const ScrollBar::Config& scrollBarConfig, const Config& config)
    {
        auto thumbnailView = qobject_cast<AssetFolderThumbnailView*>(widget);
        if (!thumbnailView)
        {
            return false;
        }

        ScrollBar::polish(style, thumbnailView, scrollBarConfig);

        thumbnailView->polish(config);
        style->repolishOnSettingsChange(thumbnailView);

        return true;
    }

    void AssetFolderThumbnailView::polish(const Config& config)
    {
        m_config = config;
        m_delegate->polish(config);
    }

    AssetFolderThumbnailView::AssetFolderThumbnailView(QWidget* parent)
        : QAbstractItemView(parent)
        , m_delegate(new AssetFolderThumbnailViewDelegate(this))
        , m_thumbnailSize(ThumbnailSize::Small)
        , m_config(defaultConfig())
    {
        setItemDelegate(m_delegate);
    }

    AssetFolderThumbnailView::~AssetFolderThumbnailView() = default;

    void AssetFolderThumbnailView::setThumbnailSize(ThumbnailSize size)
    {
        if (size == m_thumbnailSize)
        {
            return;
        }
        m_thumbnailSize = size;
        scheduleDelayedItemsLayout();
    }

    AssetFolderThumbnailView::ThumbnailSize AssetFolderThumbnailView::thumbnailSize() const
    {
        return m_thumbnailSize;
    }

    QModelIndex AssetFolderThumbnailView::indexAt(const QPoint& point) const
    {
        if (!model())
        {
            return {};
        }

        const auto p = point + QPoint{horizontalOffset(), verticalOffset()};

        const auto it = std::find_if(m_itemGeometry.constBegin(), m_itemGeometry.constEnd(), [&p](const QRect& rect) {
            return rect.contains(p);
        });
        if (it != m_itemGeometry.end())
        {
            return it.key();
        }

        return {};
    }

    void AssetFolderThumbnailView::scrollTo(const QModelIndex& index, QAbstractItemView::ScrollHint hint)
    {
        if (!index.isValid())
        {
            return;
        }

        auto it = m_itemGeometry.find(index);
        if (it == m_itemGeometry.end())
        {
            return;
        }

        const auto& rect = it.value();

        switch (hint)
        {
            case EnsureVisible:
                if (horizontalOffset() > rect.right())
                {
                    horizontalScrollBar()->setValue(rect.left());
                }
                else if ((horizontalOffset() + viewport()->width()) < rect.left())
                {
                    horizontalScrollBar()->setValue(rect.right() - viewport()->width());
                }

                if (verticalOffset() > rect.bottom())
                {
                    verticalScrollBar()->setValue(rect.top());
                }
                else if ((verticalOffset() + viewport()->height()) < rect.top())
                {
                    verticalScrollBar()->setValue(rect.bottom() - viewport()->height());
                }
                break;

            case PositionAtTop:
                horizontalScrollBar()->setValue(rect.left());
                verticalScrollBar()->setValue(rect.top());
                break;

            case PositionAtBottom:
                horizontalScrollBar()->setValue(rect.right() - viewport()->width());
                verticalScrollBar()->setValue(rect.bottom() - viewport()->height());
                break;

            case PositionAtCenter:
                horizontalScrollBar()->setValue(rect.center().x() - (viewport()->width() / 2));
                verticalScrollBar()->setValue(rect.center().y() - (viewport()->height() / 2));
                break;
        }
    }

    QRect AssetFolderThumbnailView::visualRect(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return {};
        }

        const auto it = m_itemGeometry.find(index);
        if (it == m_itemGeometry.end())
        {
            return {};
        }

        const auto& rect = it.value();
        return rect.translated(-horizontalOffset(), -verticalOffset());
    }

    void AssetFolderThumbnailView::setRootIndex(const QModelIndex& index)
    {
        if (index != rootIndex())
        {
            QAbstractItemView::setRootIndex(index);
            m_expandedIndexes.clear();
            emit rootIndexChanged(index);
        }
    }

    QModelIndex AssetFolderThumbnailView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
    {
        Q_UNUSED(modifiers);

        if (!model())
        {
            return {};
        }

        const auto rowCount = model()->rowCount();
        if (rowCount == 0)
        {
            return {};
        }

        switch (cursorAction)
        {
            case MoveHome:
                return model()->index(0, 0, rootIndex());

            case MoveEnd:
                return model()->index(rowCount - 1, 0, rootIndex());

            case MovePrevious:
            {
                const auto current = currentIndex();
                if (current.isValid())
                {
                    return model()->index((current.row() - 1) % rowCount, 0, rootIndex());
                }
            }
            break;

            case MoveNext:
            {
                const auto current = currentIndex();
                if (current.isValid())
                {
                    return model()->index((current.row() + 1) % rowCount, 0, rootIndex());
                }
            }
            break;

            case MoveUp:
            case MoveDown:
            case MoveLeft:
            case MoveRight:
            case MovePageUp:
            case MovePageDown:
                // TODO
                break;
        }

        return {};
    }

    int AssetFolderThumbnailView::horizontalOffset() const
    {
        return horizontalScrollBar()->value();
    }

    int AssetFolderThumbnailView::verticalOffset() const
    {
        return verticalScrollBar()->value();
    }

    bool AssetFolderThumbnailView::isIndexHidden(const QModelIndex&) const
    {
        return false;
    }

    void AssetFolderThumbnailView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags)
    {
        Q_UNUSED(rect);
        Q_UNUSED(flags);
    }

    QRegion AssetFolderThumbnailView::visualRegionForSelection(const QItemSelection& selection) const
    {
        QRegion region;
        for (const auto& index : selection.indexes())
        {
            region += visualRect(index);
        }
        return region;
    }

    bool AssetFolderThumbnailView::isExpandable(const QModelIndex& index) const
    {
        return index.data(static_cast<int>(AssetFolderThumbnailView::Role::IsExpandable)).value<bool>();
    }

    void AssetFolderThumbnailView::paintEvent(QPaintEvent* event)
    {
        QAbstractItemView::paintEvent(event);

        const auto rowCount = model()->rowCount();
        if (m_itemGeometry.isEmpty() && rowCount)
        {
            updateGeometries();
        }

        QPainter painter{viewport()};
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        painter.setBackground(palette().window());
        painter.setFont(font());

        painter.translate(-horizontalOffset(), -verticalOffset());

        paintChildFrames(&painter);

        paintItems(&painter);
    }

    void AssetFolderThumbnailView::paintChildFrames(QPainter* painter) const
    {
        for (const auto& childFrame : m_childFrames)
        {
            const auto& index = childFrame.index;
            const auto childCount = model()->rowCount(index);
            const auto title = tr("%1 Assets (%2)").arg(index.data().toString()).arg(childCount);

            bool firstFrame = true;

            for (const auto& rect : childFrame.rects)
            {
                // draw frame

                const auto borderRadius = firstFrame ? 0 : m_config.childFrame.borderRadius;
                painter->setPen(m_config.childFrame.borderColor);
                painter->setBrush(m_config.childFrame.backgroundColor);
                painter->drawRoundedRect(rect, borderRadius, borderRadius);
            }

            // draw expand button

            const auto& lastRect = childFrame.rects.last();
            const auto& buttonConfig = m_config.expandButton;
            const auto width = buttonConfig.width;
            const auto buttonRect = QRect{lastRect.left() + lastRect.width() - width, lastRect.top(), width, lastRect.height()};
            paintExpandButton(painter, buttonRect, false, buttonConfig);
        }
    }

    void AssetFolderThumbnailView::paintItems(QPainter* painter) const
    {
        QStyleOptionViewItem option;
        option.palette = palette();
        option.font = font();
        option.fontMetrics = fontMetrics();
        option.decorationAlignment = Qt::AlignCenter;

        const QRect visibleRect{QPoint{horizontalOffset(), verticalOffset()}, viewport()->contentsRect().size()};

        for (auto it = m_itemGeometry.constBegin(); it != m_itemGeometry.constEnd(); ++it)
        {
            const auto& index = it.key();

            option.rect = it.value();
            if (!option.rect.intersects(visibleRect))
            {
                continue;
            }

            option.state = QStyle::State_None;
            if (selectionModel()->isSelected(index))
            {
                option.state |= QStyle::State_Selected;
            }
            if (isExpandable(index))
            {
                if (m_expandedIndexes.contains(index))
                {
                    option.state |= QStyle::State_UpArrow;
                }
                else
                {
                    option.state |= QStyle::State_DownArrow;
                }
            }

            itemDelegate(index)->paint(painter, option, index);
        }
    }

    QModelIndex AssetFolderThumbnailView::indexAtPos(const QPoint& pos) const
    {
        auto it = std::find_if(
            m_itemGeometry.keyBegin(),
            m_itemGeometry.keyEnd(),
            [this, &pos](const QModelIndex& index)
            {
                return m_itemGeometry.value(index).contains(pos);
            });

        if (it != m_itemGeometry.keyEnd())
        {
            return *it;
        }
        return {};
    }

    void AssetFolderThumbnailView::mousePressEvent(QMouseEvent* event)
    {
        const auto p = event->pos() + QPoint{horizontalOffset(), verticalOffset()};

        // check the expand/collapse buttons on one of the top level items was clicked

        {
            auto it = std::find_if(
                m_itemGeometry.keyBegin(),
                m_itemGeometry.keyEnd(),
                [this, &p](const QModelIndex& index)
                {
                    if (isExpandable(index) && !m_expandedIndexes.contains(index))
                    {
                        const auto& rect = m_itemGeometry.value(index);
                        const auto width = m_config.expandButton.width;
                        const auto buttonRect = QRect{ rect.left() + rect.width() - width, rect.top(), width, rect.width() };
                        return buttonRect.contains(p);
                    }
                    return false;
                });
            if (it != m_itemGeometry.keyEnd())
            {
                if (m_expandedIndexes.contains(*it))
                {
                    m_expandedIndexes.remove(*it);
                }
                else
                {
                    m_expandedIndexes.insert(*it);
                }
                scheduleDelayedItemsLayout();
                return;
            }
        }

        // check that the preview on one of the top level items was clicked
        // No need to do computations on m_itemGeometry entries since we handled the expand/collapse button with the case above
        {
            auto idx = indexAtPos(p);

            if (idx.isValid())
            {
                selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
                emit clicked(idx);
                return;
            }
        }

        // check the collapse button on one of the child frames was clicked

        {
            auto it = std::find_if(m_childFrames.begin(), m_childFrames.end(), [this, &p](const ChildFrame& childFrame) {
                const auto& rect = childFrame.rects.last();
                const auto width = m_config.expandButton.width;
                const auto buttonRect = QRect{rect.left() + rect.width() - width, rect.top(), width, rect.width()};
                return buttonRect.contains(p);
            });
            if (it != m_childFrames.end())
            {
                if (m_expandedIndexes.contains(it->index))
                {
                    m_expandedIndexes.remove(it->index);
                    scheduleDelayedItemsLayout();
                    return;
                }
            }
        }

        QAbstractItemView::mousePressEvent(event);
    }

    void AssetFolderThumbnailView::mouseDoubleClickEvent(QMouseEvent* event)
    {
        const auto p = event->pos() + QPoint{ horizontalOffset(), verticalOffset() };

        // check the expand/collapse buttons on one of the top level items was clicked
        auto idx = indexAtPos(p);

        if (idx.isValid())
        {
            selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
            emit doubleClicked(idx);
            return;
        }

        QAbstractItemView::mouseDoubleClickEvent(event);
    }

    void AssetFolderThumbnailView::contextMenuEvent(QContextMenuEvent* event)
    {
        // For now we only have a context menu in search mode for the "show in folder" option
        if (!m_showSearchResultsMode)
        {
            return;
        }

        const auto p = event->pos() + QPoint{ horizontalOffset(), verticalOffset() };
        auto idx = indexAtPos(p);

        if (idx.isValid())
        {
            m_contextMenu = new QMenu(this);
            auto action = m_contextMenu->addAction("Show In Folder");
            connect(
                action,
                &QAction::triggered,
                this,
                [this, idx]()
                {
                    emit showInFolderTriggered(idx);
                });
            m_contextMenu->exec(event->globalPos());
            delete m_contextMenu;
            m_contextMenu = nullptr;
        }
    }

    int AssetFolderThumbnailView::rootThumbnailSizeInPixels() const
    {
        return m_config.rootThumbnail.width;
    }

    int AssetFolderThumbnailView::childThumbnailSizeInPixels() const
    {
        return m_config.childThumbnail.width;
    }

    void AssetFolderThumbnailView::updateGeometries()
    {
        m_itemGeometry.clear();
        m_childFrames.clear();

        if (!model())
        {
            return;
        }

        int x = m_config.viewportPadding;
        int y = m_config.viewportPadding;

        const QSize itemSize{ m_config.rootThumbnail.width, m_config.rootThumbnail.height + 4 + fontMetrics().height() };
        const int rowHeight = itemSize.height() + m_config.topItemsVerticalSpacing;

        if (m_showSearchResultsMode || !rootIndex().isValid())
        {
            updateGeometriesInternal(model()->index(0, 0, {}), x, y);
        }
        else
        {
            updateGeometriesInternal(rootIndex(), x, y);
        }

        verticalScrollBar()->setPageStep(viewport()->height());
        verticalScrollBar()->setRange(0, y + rowHeight - viewport()->height());
    }

    void AssetFolderThumbnailView::updateGeometriesInternal(const QModelIndex& idx, int& x, int& y)
    {
        const auto rowCount = model()->rowCount(idx);
        if (rowCount == 0)
        {
            return;
        }

        const QSize itemSize{ m_config.rootThumbnail.width, m_config.rootThumbnail.height + 4 + fontMetrics().height() };

        const int viewportWidth = viewport()->width() - 2 * m_config.viewportPadding;
        const int rowHeight = itemSize.height() + m_config.topItemsVerticalSpacing;

        const QSize childItemSize{ m_config.childThumbnail.width, m_config.childThumbnail.height };

        const int childItemYOffset = (m_config.rootThumbnail.height - m_config.childThumbnail.height) / 2;

        for (int row = 0; row < rowCount; ++row)
        {
            const auto index = model()->index(row, 0, idx);

            // When in search results mode, we visit the whole asset tree, but only display entries that are
            // exact matches for the search filter. This is reflected in the IsVisible role on the associated
            // AssetBrowserFilterModel model.
            if (m_showSearchResultsMode && !index.data(static_cast<int>(Role::IsVisible)).value<bool>())
            {
                continue;
            }

            if (row > 0 && x + itemSize.width() > viewportWidth)
            {
                x = m_config.viewportPadding;
                y += rowHeight;
            }

            // add item geometry
            m_itemGeometry[index] = { QPoint{ x, y }, itemSize };
            x += itemSize.width();

            // add child items and frames if item is expanded
            const auto childRowCount = model()->rowCount(index);

            // this top item has no children, we can compute the position of next one already and continue computing top level items
            // geometries
            if (!childRowCount)
            {
                x += m_config.topItemsHorizontalSpacing;
                continue;
            }

            if (childRowCount && m_expandedIndexes.contains(index))
            {
                ChildFrame childFrame{index};

                auto& childFrameRects = childFrame.rects;

                const auto addChildFrame =
                    [this, &childFrameRects, childItemYOffset, childItemSize](int frameLeft, int frameRight, int yRow)
                {
                    const auto top = yRow + childItemYOffset - m_config.childFrame.padding;
                    const auto width = frameRight - frameLeft + m_config.childFrame.padding;
                    const auto height = childItemSize.height() + 2 * m_config.childFrame.padding;
                    childFrameRects.append({ frameLeft, top, width, height });
                };

                // Children frame begins right after the parent item
                auto frameLeft = x;

                // Start computing sizes for all child frame rects
                //
                // A child frame is sized with a pattern like the following:
                // padding - {item width - horizontal spacing} ... (n - 1) times - item width - padding - close button
                //
                // as soon as adding an item makes the child frame too wide we add a new one on a new row
                // with a similar pattern.

                // Add initial padding
                x += m_config.childFrame.padding;

                for (int childRow = 0; childRow < childRowCount; ++childRow)
                {
                    if (x + childItemSize.width() + m_config.childFrame.padding > viewportWidth)
                    {
                        if (childRow > 0)
                        {
                            addChildFrame(frameLeft, x - m_config.childrenItemsHorizontalSpacing, y);
                        }
                        x = m_config.viewportPadding + m_config.childFrame.padding;
                        y += rowHeight;
                        frameLeft = m_config.viewportPadding;
                    }

                    const auto childIndex = model()->index(childRow, 0, index);
                    m_itemGeometry[childIndex] = { QPoint{ x, y + childItemYOffset },
                                                   childItemSize + QSize{ 0, m_config.childFrame.padding + 5 + fontMetrics().height() } };
                    x += childItemSize.width();
                    if (childRow < childRowCount - 1)
                    {
                        x += m_config.childrenItemsHorizontalSpacing;
                    }
                }

                // Make room for the close button when adding the children frame
                addChildFrame(frameLeft, x + m_config.childFrame.closeButtonWidth, y);

                m_childFrames.append(childFrame);

                x += m_config.childFrame.padding + m_config.topItemsHorizontalSpacing;
            }

            // Deal with spacing between top level items
            x += m_config.topItemsHorizontalSpacing;
        }

        // Generate geometries recursively for all children if in search results mode
        if (m_showSearchResultsMode)
        {
            for (int row = 0; row < rowCount; ++row)
            {
                const auto index = model()->index(row, 0, idx);
                updateGeometriesInternal(index, x, y);
            }
        }
    }

    void AssetFolderThumbnailView::SetShowSearchResultsMode(bool searchMode)
    {
        m_showSearchResultsMode = searchMode;
    }

} // namespace AzQtComponents

#include "Components/Widgets/moc_AssetFolderThumbnailView.cpp"
