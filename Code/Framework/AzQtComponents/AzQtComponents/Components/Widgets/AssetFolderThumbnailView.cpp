/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/algorithm.h>
#include <AzQtComponents/Components/Style.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option") // 4244: 'initializing': conversion from 'int' to 'float', possible loss of data
                                                                    // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
                                                                    // 4800: 'QFlags<QPainter::RenderHint>::Int': forcing value to bool 'true' or 'false' (performance warning)
#include <QBrush>
#include <QDrag>
#include <QGuiApplication>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTimer>
AZ_POP_DISABLE_WARNING

namespace
{
    // Returns the QString that will be displayed under each thumbnail
    QString elidedTextWithExtension(const QFontMetrics& fm, const QString& text, int width)
    {
        auto textWidth = fm.horizontalAdvance(text);
        const int dot = text.lastIndexOf(QLatin1Char{ '.' });
        QString extension = "";
        int extensionWidth = 0;

        // If the file has an extension, move it to the second row
        if (dot != -1)
        {
            auto baseName = text.left(dot);
            extension = text.mid(dot + 1);
            extensionWidth = fm.horizontalAdvance(extension);
            if (extensionWidth > width)
            {
                extension = fm.elidedText(extension, Qt::ElideRight, width);
                extensionWidth = fm.horizontalAdvance(extension);
            }
            if (baseName.isEmpty())
            {
                return text;
            }
            if (fm.horizontalAdvance(baseName) <= width)
            {
                return baseName + "\n" + extension;
            }
        }

        // Return if text fits within one row
        if (textWidth <= width)
        {
            return text;
        }

        // Text does not fit within one row, calculate the number of characters in each row
        double percentOfTextPerLine = static_cast<double>(width) / static_cast<double>(textWidth);
        int charactersPerLine = percentOfTextPerLine * text.size() - 1;
        auto firstLine = text.left(charactersPerLine);
        auto secondLine = text.mid(charactersPerLine);
        auto secondLineWidth = fm.horizontalAdvance(secondLine);
        // Check if text fits within the two rows
        if (secondLineWidth <= width)
        {
            return firstLine + "\n" + secondLine;
        }
        // If there is an extension present, elide to show it
        if (!extension.isEmpty())
        {
            auto secondLineBase = secondLine.left(dot);
            auto elidedBase = fm.elidedText(secondLineBase, Qt::ElideRight, width - extensionWidth); 
            return firstLine + "\n" + elidedBase + extension;
        }
        // Elide left to show the beginning of the text in the first row and the end of the text in the second row
        return firstLine + "\n" + fm.elidedText(secondLine, Qt::ElideLeft, width);
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

    AssetFolderThumbnailViewDelegate::AssetFolderThumbnailViewDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
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
        const auto& qVariant = index.data(Qt::DecorationRole);
        if (!qVariant.isNull())
        {
            QIcon icon;
            if (const auto& path = qVariant.value<QString>(); !path.isEmpty())
            {
                icon.addFile(path, imageRect.size(), QIcon::Normal, QIcon::Off);
                icon.paint(painter, imageRect);
            }
            else if (const auto& pixmap = qVariant.value<QPixmap>(); !pixmap.isNull())
            {
                icon.addPixmap(pixmap.scaled(imageRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation), QIcon::Normal, QIcon::Off);
                icon.paint(painter, imageRect);
            }
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
            const auto textRect = QRect{rect.left(), rect.bottom() - textHeight, rect.width(), textHeight * 2};

            painter->setPen(option.palette.color(QPalette::Text));

            QString text = index.data().toString();

            QRegularExpression htmlMarkupRegex("<[^>]*>");
            if (htmlMarkupRegex.match(text).hasMatch())
            {
                // Grab the plain text to measure.
                QTextDocument textDoc;
                textDoc.setHtml(text);
                QString plainText = textDoc.toPlainText();
                textDoc.clear();

                auto textWidth = option.fontMetrics.horizontalAdvance(plainText);

                QString alignment = textWidth > textRect.width() ? "left" : "center";

                QStyleOptionViewItem optionV4{ option };
                initStyleOption(&optionV4, index);
                optionV4.state &= ~(QStyle::State_HasFocus | QStyle::State_Selected);

                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);

                textDoc.setDefaultFont(option.font);

                textDoc.setDefaultStyleSheet("body {color: white}");

                textDoc.setHtml("<body align='" + alignment + "'>" + index.data().toString() + "</span> </body>");
                painter->translate(textRect.topLeft());
                textDoc.setTextWidth(textRect.width());
                textDoc.drawContents(painter, QRectF(0, 0, textRect.width(), textRect.height()));
                painter->restore();
            }
            else
            {
                text = elidedTextWithExtension(option.fontMetrics, index.data().toString(), textRect.width());

                (text.contains(QChar::LineFeed) ? painter->drawText(textRect, Qt::AlignTop | Qt::AlignLeft, text)
                                                : painter->drawText(textRect, Qt::AlignTop | Qt::AlignHCenter, text));
            }
        }
        else
        {
            // text
            const auto textHeight = option.fontMetrics.height();
            const auto textRect = QRect{ rect.left(), rect.bottom() - textHeight, rect.width(), textHeight * 2};

            painter->setPen(option.palette.color(QPalette::Text));
            QString text = elidedTextWithExtension(option.fontMetrics, index.data().toString(), textRect.width());
            (text.contains(QChar::LineFeed) ? painter->drawText(textRect, Qt::AlignTop | Qt::AlignLeft, text)
                                            : painter->drawText(textRect, Qt::AlignTop | Qt::AlignHCenter, text));
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

    QWidget* AssetFolderThumbnailViewDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget);
        if (lineEdit)
        {
            connect(lineEdit, &QLineEdit::editingFinished, this, &AssetFolderThumbnailViewDelegate::editingFinished);
        }
        return widget;
    }

    void AssetFolderThumbnailViewDelegate::editingFinished()
    {
        if (auto sendingLineEdit = qobject_cast<QLineEdit*>(sender()); sendingLineEdit)
        {
            // debounce this call by disconnecting from the signal immediately.
            // This is because editingFinished is triggered repeatedly, for example,
            // when the user presses enter but also when the line edit loses focus.  
            // If the user presses enter, both happen, causing a double send if we don't debounce like this.
            QObject::disconnect(sendingLineEdit, &QLineEdit::editingFinished, this, &AssetFolderThumbnailViewDelegate::editingFinished); 
            emit RenameThumbnail(sendingLineEdit->text());
        }
    }

    void AssetFolderThumbnailViewDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor))
        {
            const auto& rect = option.rect;
            const auto textHeight = option.fontMetrics.height();
            const auto textRect = QRect{ rect.left(), rect.bottom() - textHeight, rect.width(), textHeight * 2 };
            lineEdit->setGeometry(textRect);
            lineEdit->setMaximumWidth(rect.width());
            return;
        }
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
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
        config.scrollSpeed = settings.value(QStringLiteral("ScrollSpeed"), config.scrollSpeed).toInt();

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
        config.scrollSpeed = 45;

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
        setSelectionMode(ExtendedSelection);

        connect(
            m_delegate,
            &AssetFolderThumbnailViewDelegate::RenameThumbnail,
            this,
            [this](const QString& value)
            {
                emit afterRename(value);
            });

        // Enable drag/drop.
        setDragEnabled(true);
        setAcceptDrops(true);
        setDragDropMode(QAbstractItemView::DragDrop);
        setDropIndicatorShown(true);
        setDragDropOverwriteMode(true);

        // Setup the Selection Updater timer. Only start during a drag selection operation.
        m_selectionUpdater = new QTimer(this);
        connect(m_selectionUpdater, &QTimer::timeout, this,
            [&](){
                SelectAllEntitiesInSelectionRect();
                update();
            }
        );
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

        // Draw the drag selection rect.
        if (m_isDragSelectActive && m_queuedMouseEvent)
        {
            // Retrieve the two corners of the rect.
            const QPoint point1 = (m_queuedMouseEvent->pos()); // The position the drag operation started at.
            const QPoint point2 = (m_mousePosition); // The current mouse position.

            // We need the top left and bottom right corners, which may not be the two corners we got above.
            // So we composite the corners based on the coordinates of the points.
            const QPoint topLeft(AZStd::min(point1.x(), point2.x()), AZStd::min(point1.y(), point2.y()));
            const QPoint bottomRight(AZStd::max(point1.x(), point2.x()), AZStd::max(point1.y(), point2.y()));

            // Paint the rect.
            painter.setBrush(m_dragSelectRectColor);
            painter.setPen(m_dragSelectBorderColor);
            painter.drawRect(QRect(topLeft, bottomRight));
        }
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
        const auto p = event->pos() + QPoint{ horizontalOffset(), verticalOffset() };
        auto idx = indexAtPos(p);
        emit contextMenu(idx);
    }

    int AssetFolderThumbnailView::rootThumbnailSizeInPixels() const
    {
        return m_config.rootThumbnail.width;
    }

    int AssetFolderThumbnailView::childThumbnailSizeInPixels() const
    {
        return m_config.childThumbnail.width;
    }

    void AssetFolderThumbnailView::rowsInserted(const QModelIndex& parent, int start, int end)
    {
        scheduleDelayedItemsLayout();

        QAbstractItemView::rowsInserted(parent, start, end);
    }

    void AssetFolderThumbnailView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
    {
        scheduleDelayedItemsLayout();

        QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
    }

    void AssetFolderThumbnailView::reset()
    {
        m_itemGeometry.clear();
        m_expandedIndexes.clear();
        m_childFrames.clear();

        QAbstractItemView::reset();
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
        verticalScrollBar()->setSingleStep(m_config.scrollSpeed);
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

    bool AssetFolderThumbnailView::InSearchResultsMode() const
    {
        return m_showSearchResultsMode;
    }

    void AssetFolderThumbnailView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        QAbstractItemView::selectionChanged(selected, deselected);
        Q_EMIT selectionChangedSignal(selected, deselected);
    }

    void AssetFolderThumbnailView::mousePressEvent(QMouseEvent* event)
    {
        // Postponing normal mouse press logic until mouse is released or dragged.
        // This allows drag/drop of non-selected items.
        ClearQueuedMouseEvent();
        m_queuedMouseEvent = new QMouseEvent(*event);
    }

    void AssetFolderThumbnailView::mouseMoveEvent(QMouseEvent* event)
    {
        // Prevent multiple updates throughout the function for changing UIs.
        bool forceUpdate = false;

        m_mousePosition = event->pos();

        if (m_queuedMouseEvent && !m_isDragSelectActive)
        {
            // Determine whether the mouse move should trigger a rect selection or an entity drag.
            QModelIndex clickedIndex = indexAt(m_queuedMouseEvent->pos());
            if (clickedIndex.isValid())
            {
                HandleDrag();
            }
            else
            {
                // Store previous selection - will be used if key modifiers are being held.
                m_previousSelection = selectionModel()->selection();

                // Start the updater to only refresh the selection at regular intervals.
                m_selectionUpdater->start(m_selectionUpdateInterval);

                m_isDragSelectActive = true;
                forceUpdate = true;
            }
        }

        if (forceUpdate)
        {
            update();
        }
    }

    void AssetFolderThumbnailView::mouseReleaseEvent(QMouseEvent* event)
    {
        if (m_isDragSelectActive)
        {
            SelectAllEntitiesInSelectionRect();
            update();
        }
        else if (m_queuedMouseEvent)
        {
            ProcessQueuedMousePressedEvent(m_queuedMouseEvent);
        }

        ClearQueuedMouseEvent();

        m_previousSelection.clear();
        m_selectionUpdater->stop();
        m_isDragSelectActive = false;

        QAbstractItemView::mouseReleaseEvent(event);
    }

    void AssetFolderThumbnailView::SelectAllEntitiesInSelectionRect()
    {
        if (!m_queuedMouseEvent)
        {
            return;
        }

        // TODO - Handle Ctrl and Shift

        // Retrieve the two opposing corners of the rect.
        const QPoint point1 = (m_queuedMouseEvent->pos()); // The position the drag operation started at.
        const QPoint point2 = (m_mousePosition); // The current mouse position.

        // Determine which point's y is the top and which is the bottom.
        const int top(AZStd::min(point1.y(), point2.y()));
        const int bottom(AZStd::max(point1.y(), point2.y()));
        const int left(AZStd::min(point1.x(), point2.x()));
        const int right(AZStd::max(point1.x(), point2.x()));

        const QRect selectionRect(QPoint(left, top), QPoint(right, bottom));

        QItemSelection selection;
        for (auto iter = m_itemGeometry.begin(), end = m_itemGeometry.end(); iter != end; ++iter)
        {
            QRect indexRect = iter.value();
            indexRect.translate(-horizontalOffset(), -verticalOffset());

            if (selectionRect.intersects(indexRect))
            {
                const auto& index = iter.key();
                selection.select(index, index);
            }
        }

        switch (QGuiApplication::queryKeyboardModifiers())
        {
        case Qt::ControlModifier:
            {
                selection.merge(m_previousSelection, QItemSelectionModel::ToggleCurrent);
                selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
            }
            break;

        case Qt::ShiftModifier:
            {
                // Replicates behavior in Windows explorer.
                // This will add all newly selected items to the existing selection, but if an item is added
                // to the selection rect and then removed, it will be removed from the previous selection.

                // Remove new selection from previous selection.
                m_previousSelection.merge(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Current);

                // Add previous selection to new selection.
                selection.merge(m_previousSelection, QItemSelectionModel::SelectCurrent);

                // Select both (updated) previous and current selection.
                selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
            }
            break;

        default:
            {
                selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
            }
            break;
        }
    }

    void AssetFolderThumbnailView::ClearQueuedMouseEvent()
    {
        if (m_queuedMouseEvent)
        {
            delete m_queuedMouseEvent;
            m_queuedMouseEvent = nullptr;
        }
    }

    void AssetFolderThumbnailView::ProcessQueuedMousePressedEvent(QMouseEvent* event)
    {
        const auto p = event->pos() + QPoint{ horizontalOffset(), verticalOffset() };

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
        auto idx = indexAtPos(p);
        if (idx.isValid())
        {
            bool isRightClick = event->button() == Qt::RightButton;
            if (selectionMode() == ExtendedSelection && QGuiApplication::queryKeyboardModifiers() == Qt::ControlModifier)
            {
                auto selectionFlag = isRightClick ? QItemSelectionModel::SelectionFlag::Select : QItemSelectionModel::SelectionFlag::Toggle;
                selectionModel()->select(idx, selectionFlag);
            }
            else if (selectionMode() == ExtendedSelection && QGuiApplication::queryKeyboardModifiers() == Qt::ShiftModifier)
            {
                auto selectedIndex =
                    (selectionModel()->hasSelection() ? selectionModel()->currentIndex() : model()->index(0, 0, rootIndex()));
                QItemSelectionRange indexRange;
                if (selectedIndex.row() < idx.row())
                {
                    indexRange = QItemSelectionRange(selectedIndex, idx);
                }
                else if (selectedIndex.row() > idx.row())
                {
                    indexRange = QItemSelectionRange(idx, selectedIndex);
                }
                if (!indexRange.isEmpty())
                {
                    QItemSelectionModel::SelectionFlag selectionFlag;
                    if (selectionModel()->isSelected(idx) && !isRightClick)
                    {
                        selectionFlag = QItemSelectionModel::SelectionFlag::Deselect;
                    }
                    else
                    {
                        selectionFlag = QItemSelectionModel::SelectionFlag::Select;
                    }
                    for (auto index : indexRange.indexes())
                    {
                        selectionModel()->select(index, selectionFlag);
                    }
                }
            }
            else if (!selectionModel()->isSelected(idx))
            {
                selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
            }
            update();
            // Pass event to base class to enable drag/drop selections.
            QAbstractItemView::mousePressEvent(event);
            return;
        }

        // check the collapse button on one of the child frames was clicked
        {
            auto it = std::find_if(
                m_childFrames.begin(),
                m_childFrames.end(),
                [this, &p](const ChildFrame& childFrame)
                {
                    const auto& rect = childFrame.rects.last();
                    const auto width = m_config.expandButton.width;
                    const auto buttonRect = QRect{ rect.left() + rect.width() - width, rect.top(), width, rect.width() };
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

        // If empty space in the view is clicked, clear the current selection and update the view
        if (!idx.isValid() && selectionModel()->hasSelection())
        {
            selectionModel()->clear();
            emit deselected();
            update();
            return;
        }
    }

    void AssetFolderThumbnailView::HandleDrag()
    {
        // Retrieve the index at the click position.
        QModelIndex indexAtClick = indexAt(m_queuedMouseEvent->pos()).siblingAtColumn(0);

        // If the index is selected, we should move the whole selection.
        if (selectionModel()->isSelected(indexAtClick))
        {
            StartCustomDrag(selectionModel()->selectedIndexes(), defaultDropAction());
        }
        else
        {
            StartCustomDrag(QModelIndexList{ indexAtClick }, defaultDropAction());
        }
    }

    void AssetFolderThumbnailView::StartCustomDrag(const QModelIndexList& indexList, Qt::DropActions /* supportedActions */)
    {
        QMimeData* mimeData = model()->mimeData(indexList);

        if (mimeData)
        {
            QDrag* drag = new QDrag(this);
            drag->setPixmap(QPixmap::fromImage(CreateDragImage(indexList)));
            drag->setMimeData(mimeData);

            drag->exec({ Qt::CopyAction }, Qt::CopyAction);
        }
    }

    QImage AssetFolderThumbnailView::CreateDragImage(const QModelIndexList& indexList)
    {
        if (indexList.isEmpty())
        {
            return QImage();
        }

        // Define size of rect for asset/folder icons.
        QRect rect(0, 9, 36, 36);

        // Generate a drag image of the dragged element.
        // We get the configuration of the first item in the list.
        auto index = indexList.at(0);
        const auto isTopLevel = index.data(static_cast<int>(AssetFolderThumbnailView::Role::IsTopLevel)).value<bool>();
        const auto& config = isTopLevel ? m_config.rootThumbnail : m_config.childThumbnail;

        // Initialize option
        QStyleOptionViewItem option;
        option.palette = palette();
        option.font = font();
        option.fontMetrics = fontMetrics();
        option.decorationAlignment = Qt::AlignCenter;
        option.rect = rect;

        // Initialize painter from desired output image.
        QImage dragImage(QRect(0, 0, 44, 56).size(), QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&dragImage);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(dragImage.rect(), Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        painter.save();

        // Define border color.
        if (selectionModel()->isSelected(index))
        {
            painter.setPen(QPen(config.selectedBorderColor, config.selectedBorderThickness));
        }
        else
        {
            painter.setPen(QPen(config.borderColor, config.borderThickness));
        }
        
        // Paint additional rectangles under the top one to signify multiple selection.
        painter.setBrush(QColor("#555")); // TODO - move to const members

        if (indexList.size() > 2)
        {
            painter.setOpacity(0.5f);
            painter.drawRoundedRect(rect.translated(7, 7), 1, 1); // TODO - move to const members
        }

        if (indexList.size() > 1)
        {
            painter.setOpacity(1.0f);
            painter.drawRoundedRect(rect.translated(3, 3), 1, 1); // TODO - move to const members
        }

        // Reset opacity to full.
        painter.setOpacity(1.0f);



        const auto thumbnailRect = rect;

        const auto padding = 5;
        const auto imageRect = thumbnailRect.adjusted(padding, padding, -padding, -padding);

        // border
        const auto halfBorderThickness = qMax(config.borderThickness, config.selectedBorderThickness) / 2;
        const auto borderRect =
            QRectF{ thumbnailRect }.adjusted(halfBorderThickness, halfBorderThickness, -halfBorderThickness, -halfBorderThickness);
        painter.setBrush(config.backgroundColor);
        painter.drawRoundedRect(borderRect, config.borderRadius, config.borderRadius);

        // pixmap
        const auto& qVariant = index.data(Qt::DecorationRole);
        if (!qVariant.isNull())
        {
            QIcon icon;
            if (const auto& path = qVariant.value<QString>(); !path.isEmpty())
            {
                icon.addFile(path, imageRect.size(), QIcon::Normal, QIcon::Off);
                icon.paint(&painter, imageRect);
            }
            else if (const auto& pixmap = qVariant.value<QPixmap>(); !pixmap.isNull())
            {
                icon.addPixmap(pixmap.scaled(imageRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation), QIcon::Normal, QIcon::Off);
                icon.paint(&painter, imageRect);
            }
        }

        if (isTopLevel)
        {
            // draw top right status icon if there's data in the corresponding role
            if (index.data(Qt::StatusTipRole).isValid())
            {
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor::fromRgb(0x22, 0x22, 0x22));
                painter.drawRect(rect.right() - config.borderThickness + 1 - 16, rect.top() + config.borderThickness, 16, 16);
            }
        }

        painter.restore();

        painter.end();
        return dragImage;
    }

} // namespace AzQtComponents

#include "Components/Widgets/moc_AssetFolderThumbnailView.cpp"
