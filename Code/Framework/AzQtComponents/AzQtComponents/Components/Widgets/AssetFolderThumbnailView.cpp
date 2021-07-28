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

        const auto buttonRect = rect.adjusted(2, 2, -2, -2);
        painter->setPen(Qt::NoPen);
        painter->setBrush(config.backgroundColor);
        painter->drawRoundedRect(buttonRect, config.borderRadius, config.borderRadius);

        // caret

        const auto caretWidth = config.caretWidth;
        const auto center = QRectF{buttonRect}.center();

        const auto caretDirection = closed ? 1.0 : -1.0;

        QPolygonF caret;
        caret.append(center + QPointF{-.5f * caretWidth * caretDirection, -caretWidth});
        caret.append(center + QPointF{-.5f * caretWidth * caretDirection, caretWidth});
        caret.append(center + QPointF{.5f * caretWidth * caretDirection, 0});

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
        const auto isTopLevel = index.parent().isValid() == false;

        painter->save();

        painter->setFont(option.font);

        const auto& config = isTopLevel ? m_config.rootThumbnail : m_config.childThumbnail;

        const auto borderRadius = config.borderRadius;
        const auto padding = config.padding;
        const auto& rect = option.rect;

        const auto thumbnailRect = QRect{rect.left(), rect.top(), rect.width(), rect.width()};
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
            // expand button

            if ((option.state & QStyle::State_UpArrow) || (option.state & QStyle::State_DownArrow))
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
            painter->drawText(textRect, elidedTextWithExtension(option.fontMetrics, index.data().toString(), textRect.width()), QTextOption{option.decorationAlignment});
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
        thumbnail.smallSize = settings.value(QStringLiteral("SmallSize"), thumbnail.smallSize).toInt();
        thumbnail.mediumSize = settings.value(QStringLiteral("MediumSize"), thumbnail.mediumSize).toInt();
        thumbnail.largeSize = settings.value(QStringLiteral("LargeSize"), thumbnail.largeSize).toInt();
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
        expandButton.borderRadius = settings.value(QStringLiteral("BorderRadius"), expandButton.borderRadius).toReal();
        readColor(settings, "BackgroundColor", expandButton.backgroundColor);
        readColor(settings, "CaretColor", expandButton.caretColor);
    }

    static void readChildFrame(QSettings& settings, AssetFolderThumbnailView::Config::ChildFrame& childFrame)
    {
        childFrame.padding = settings.value(QStringLiteral("Padding"), childFrame.padding).toInt();
        childFrame.borderRadius = settings.value(QStringLiteral("BorderRadius"), childFrame.borderRadius).toReal();
        readColor(settings, "BackgroundColor", childFrame.backgroundColor);
    }

    AssetFolderThumbnailView::Config AssetFolderThumbnailView::loadConfig(QSettings& settings)
    {
        auto config = defaultConfig();

        config.margin = settings.value(QStringLiteral("Margin"), config.margin).toInt();

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

        config.margin = 8;

        config.rootThumbnail.smallSize = 72;
        config.rootThumbnail.mediumSize = 84;
        config.rootThumbnail.largeSize = 96;
        config.rootThumbnail.borderRadius = 8.0;
        config.rootThumbnail.padding = 12;
        config.rootThumbnail.backgroundColor = QColor(0x44, 0x44, 0x44);
        config.rootThumbnail.borderThickness = 1.0;
        config.rootThumbnail.selectedBorderThickness = 2.0;
        config.rootThumbnail.borderColor = QColor(0x00, 0x00, 0x00);
        config.rootThumbnail.selectedBorderColor = QColor(0xff, 0xff, 0xff);

        config.childThumbnail.smallSize = 39;
        config.childThumbnail.mediumSize = 48;
        config.childThumbnail.largeSize = 54;
        config.childThumbnail.borderRadius = 2.4;
        config.childThumbnail.padding = 7;
        config.childThumbnail.backgroundColor = QColor(0x44, 0x44, 0x44);
        config.childThumbnail.borderThickness = 1.0;
        config.childThumbnail.selectedBorderThickness = 2.0;
        config.childThumbnail.borderColor = QColor(0x38, 0x38, 0x38);
        config.childThumbnail.selectedBorderColor = QColor(0xff, 0xff, 0xff);

        config.expandButton.width = 18;
        config.expandButton.caretWidth = 8.0;
        config.expandButton.borderRadius = 4.0;
        config.expandButton.backgroundColor = QColor(0x55, 0x55, 0x55);
        config.expandButton.caretColor = QColor(0xff, 0xff, 0xff);

        config.childFrame.padding = 18;
        config.childFrame.borderRadius = 4.0;
        config.childFrame.backgroundColor = QColor(0x00, 0x00, 0x00);

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
        if (!model())
        {
            return;
        }

        const auto translatedRect = rect.translated(horizontalOffset(), verticalOffset());

        for (auto it = m_itemGeometry.constBegin(); it != m_itemGeometry.constEnd(); ++it)
        {
            if (it.value().intersects(translatedRect))
            {
                selectionModel()->select(it.key(), flags);
            }
        }
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
        return !index.parent().isValid() && model()->rowCount(index) > 0;
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

                const auto borderRadius = m_config.childFrame.borderRadius;
                painter->setPen(Qt::NoPen);
                painter->setBrush(m_config.childFrame.backgroundColor);
                painter->drawRoundedRect(rect, borderRadius, borderRadius);

                if (firstFrame)
                {
                    // draw title

                    const auto textHeight = fontMetrics().height();
                    const auto textWidth = rect.width() - 2 * m_config.childFrame.padding;
                    const auto textRect = QRect{rect.left() + m_config.childFrame.padding, rect.top(), textWidth, textHeight};

                    painter->setPen(palette().color(QPalette::Text));
                    painter->drawText(textRect, fontMetrics().elidedText(title, Qt::ElideRight, textRect.width()));

                    firstFrame = false;
                }
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
                if (m_expandedRows.contains(index.row()))
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

    void AssetFolderThumbnailView::mousePressEvent(QMouseEvent* event)
    {
        const auto p = event->pos() + QPoint{horizontalOffset(), verticalOffset()};

        // check the expand/collapse buttons on one of the top level items was clicked

        {
            auto it = std::find_if(m_itemGeometry.keyBegin(), m_itemGeometry.keyEnd(), [this, &p](const QModelIndex& index) {
                if (isExpandable(index))
                {
                    const auto& rect = m_itemGeometry.value(index);
                    const auto width = m_config.expandButton.width;
                    const auto buttonRect = QRect{rect.left() + rect.width() - width, rect.top(), width, rect.width()};
                    return buttonRect.contains(p);
                }
                return false;
            });
            if (it != m_itemGeometry.keyEnd())
            {
                const auto row = it->row();
                if (m_expandedRows.contains(row))
                {
                    m_expandedRows.remove(row);
                }
                else
                {
                    m_expandedRows.insert(row);
                }
                scheduleDelayedItemsLayout();
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
                const auto row = it->index.row();
                if (m_expandedRows.contains(row))
                {
                    m_expandedRows.remove(row);
                    scheduleDelayedItemsLayout();
                    return;
                }
            }
        }

        QAbstractItemView::mousePressEvent(event);
    }

    int AssetFolderThumbnailView::rootThumbnailSizeInPixels() const
    {
        switch (m_thumbnailSize)
        {
            case ThumbnailSize::Small:
                return m_config.rootThumbnail.smallSize;
            case ThumbnailSize::Medium:
                return m_config.rootThumbnail.mediumSize;
            case ThumbnailSize::Large:
                return m_config.rootThumbnail.largeSize;
        }
        Q_UNREACHABLE();
    }

    int AssetFolderThumbnailView::childThumbnailSizeInPixels() const
    {
        switch (m_thumbnailSize)
        {
            case ThumbnailSize::Small:
                return m_config.childThumbnail.smallSize;
            case ThumbnailSize::Medium:
                return m_config.childThumbnail.mediumSize;
            case ThumbnailSize::Large:
                return m_config.childThumbnail.largeSize;
        }
        Q_UNREACHABLE();
    }

    void AssetFolderThumbnailView::updateGeometries()
    {
        m_itemGeometry.clear();
        m_childFrames.clear();

        if (!model())
        {
            return;
        }

        const auto rowCount = model()->rowCount();
        if (rowCount == 0)
        {
            return;
        }

        const auto rootThumbnailSize = rootThumbnailSizeInPixels();
        const QSize itemSize{rootThumbnailSize, rootThumbnailSize + 4 + fontMetrics().height()};

        const int viewportWidth = viewport()->width() - m_config.margin;
        const int rowHeight = itemSize.height() + m_config.margin;

        const auto childThumbnailSize = childThumbnailSizeInPixels();
        const QSize childItemSize{childThumbnailSize, childThumbnailSize};

        const int childItemYOffset = (rootThumbnailSize - childThumbnailSize) / 2;

        int x = m_config.margin;
        int y = m_config.margin;

        for (int row = 0; row < rowCount; ++row)
        {
            if (row > 0 && x + itemSize.width() > viewportWidth)
            {
                x = m_config.margin;
                y += rowHeight;
            }

            // add item geometry

            const auto index = model()->index(row, 0, rootIndex());
            m_itemGeometry[index] = {QPoint{x, y}, itemSize};
            x += itemSize.width() + m_config.margin;

            // add child items and frames if item is expanded

            const auto childRowCount = model()->rowCount(index);
            if (childRowCount && m_expandedRows.contains(row))
            {
                ChildFrame childFrame{index};

                auto& childFrameRects = childFrame.rects;

                const auto addChildFrame = [this, &childFrameRects, childItemYOffset, childThumbnailSize](int frameLeft, int frameRight, int yRow) {
                    const auto top = yRow + childItemYOffset - m_config.childFrame.padding;
                    const auto width = frameRight - frameLeft + m_config.childFrame.padding;
                    const auto height = childThumbnailSize + 2 * m_config.childFrame.padding;
                    childFrameRects.append({frameLeft, top, width, height});
                };

                auto frameLeft = x;
                x += m_config.childFrame.padding;

                for (int childRow = 0; childRow < childRowCount; ++childRow)
                {
                    if (x + childItemSize.width() + m_config.childFrame.padding > viewportWidth)
                    {
                        if (childRow > 0)
                        {
                            addChildFrame(frameLeft, x - m_config.margin, y);
                        }
                        x = m_config.margin + m_config.childFrame.padding;
                        y += rowHeight;
                        frameLeft = m_config.margin;
                    }

                    const auto childIndex = model()->index(childRow, 0, index);
                    m_itemGeometry[childIndex] = {QPoint{x, y + childItemYOffset}, childItemSize};
                    x += childItemSize.width() + m_config.margin;
                }
                addChildFrame(frameLeft, x - m_config.margin, y);

                m_childFrames.append(childFrame);

                x += m_config.childFrame.padding;
            }
        }

        verticalScrollBar()->setPageStep(viewport()->height());
        verticalScrollBar()->setRange(0, y + rowHeight - viewport()->height());
    }
}

#include "Components/Widgets/moc_AssetFolderThumbnailView.cpp"
