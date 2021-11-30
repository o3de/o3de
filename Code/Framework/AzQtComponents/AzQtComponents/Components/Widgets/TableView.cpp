/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/TableView.h>

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzCore/Casting/numeric_cast.h>

#include <QHeaderView>
#include <QPainter>
#include <QSettings>
#include <QStyleOptionHeader>
#include <QStyleOptionViewItem>
#include <QSortFilterProxyModel>
#include <QTextLayout>
#include <QTimer>
#include <QTableView>
#include <QListView>

#include <QtGui/private/qtextengine_p.h>

namespace AzQtComponents
{
    TableView::Config TableView::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        settings.beginGroup(QStringLiteral("HeaderView"));
        ConfigHelpers::read<int>(settings, QStringLiteral("BorderWidth"), config.borderWidth);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("BorderColor"), config.borderColor);
        ConfigHelpers::read<qreal>(settings, QStringLiteral("FocusBorderWidth"), config.focusBorderWidth);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("FocusBorderColor"), config.focusBorderColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("FocusFillColor"), config.focusFillColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("HeaderFillColor"), config.headerFillColor);
        settings.endGroup();

        return config;
    }

    TableView::Config TableView::defaultConfig()
    {
        Config config;
        config.borderWidth = 1;
        config.borderColor = QStringLiteral("#dddddd");
        config.focusBorderWidth = 1;
        config.focusBorderColor = QStringLiteral("#94D2FF");
        config.focusFillColor = QStringLiteral("#10ffffff");
        config.headerFillColor = QStringLiteral("#2d2d2d");

        return config;
    }

    TableView::TableView(QWidget* parent)
        : QTreeView(parent)
        , m_previousSelectionMode(selectionMode())
    {
        setAlternatingRowColors(true);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setAllColumnsShowFocus(true);
        header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // default delegate is the one we specify
        setItemDelegate(new TableViewItemDelegate(this));

        connect(header(), &QHeaderView::sectionResized, this, &TableView::handleResize);
    }

    void TableView::setExpandOnSelection(bool expand)
    {
        if (m_expandOnSelection != expand)
        {
            if (expand)
            {
                m_previousSelectionMode = selectionMode();
            }
            m_expandOnSelection = expand;
            setSelectionMode(m_expandOnSelection ? SingleSelection : m_previousSelectionMode);

            const QModelIndexList indexes = selectionModel() ? selectionModel()->selectedRows() : QModelIndexList();
            if (!indexes.isEmpty())
            {
                selectRow(indexes.first());
            }
        }
    }

    bool TableView::expandOnSelection() const
    {
        return m_expandOnSelection;
    }

    bool TableView::dataModelSupportsIsSelectedRole() const
    {
        return (getTableViewModel() != nullptr);
    }

    void TableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        QTreeView::selectionChanged(selected, deselected);

        // ... if a QSortFilterProxyModel is attached to a TableView, then when a filter is applied
        // the selection can change. Calling selectRow below results in a dataChanged signal
        // being emitted from the underlying data model, which the QSortFilterProxyModel then
        // responds to (while it's still applying filtering and rejiggering it's own internal
        // booking). This results in a crash, because it assumes that the indexes flagged as changing
        // are properly in the right vectors/maps of its internal data, and they aren't.
        // The only part of the data from the source model that changes is the SizeHint, and
        // theoretically the QSortFilterProxyModel could only respond to changes when it's to roles
        // it cares about, but it wasn't implemented that way and so we have to come up with something
        // else.
        //
        // The something else is to queue our handling of selection change events, when it comes to
        // setting the size of the expanding rows.
        QTimer::singleShot(0, this, [this] {
            if (m_expandOnSelection)
            {
                const QModelIndexList indexes = selectedIndexes();
                selectRow(indexes.isEmpty() ? QModelIndex() : indexes.first());
            }
        });
    }

    void TableView::updateGeometries()
    {
        auto* delegate = qobject_cast<TableViewItemDelegate*>(itemDelegate());
        if (delegate)
        {
            delegate->geometriesUpdating(this);
        }

        QTreeView::updateGeometries();
    }

    void TableView::resizeEvent(QResizeEvent* e)
    {
        QTreeView::resizeEvent(e);

        handleResize();
    }

    bool TableView::drawHeader(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        const auto headerViewOption = qstyleoption_cast<const QStyleOptionHeader*>(option);
        if (!headerViewOption)
        {
            return false;
        }

        if (!((headerViewOption->state & QStyle::State_Enabled) &&
            widget->underMouse()))
        {
            return false;
        }

        // Draw the default style
        painter->save();
        style->baseStyle()->drawControl(QStyle::CE_Header, option, painter, widget);
        painter->restore();

        // Draw the border
        if (headerViewOption->section != 0)
        {
            painter->save();
            painter->setPen(Qt::NoPen);
            painter->setBrush(config.borderColor);
            painter->drawRect(option->rect.left(), option->rect.top() + 1, config.borderWidth, option->rect.height() - 2);
            painter->restore();
        }

        return true;
    }

    bool TableView::drawHeaderSection(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(widget);
        Q_UNUSED(style);

        const auto headerViewOption = qstyleoption_cast<const QStyleOptionHeader*>(option);
        if (!headerViewOption)
        {
            return false;
        }

        painter->fillRect(headerViewOption->rect, config.headerFillColor);

        return true;
    }

    bool TableView::drawFrameFocusRect(const Style* style, const QStyleOption* option, QPainter* painter, const Config& config)
    {
        Q_UNUSED(style);

        auto optionCopy = *option;

        auto tableView = qobject_cast<TableView*>(optionCopy.styleObject);
        auto qTableView = qobject_cast<QTableView*>(optionCopy.styleObject);
        auto qListView = qobject_cast<QListView*>(optionCopy.styleObject);
        auto qTreeView = qobject_cast<QTreeView*>(optionCopy.styleObject);
        if (!tableView && !qTableView && !qListView && !qTreeView)
        {
            return false;
        }

        // Need to temporarily stretch the item rectangle to the whole row
        // for QTableView and QListView, only within the scope of focus frame
        // rectangle drawing.
        if (qTableView || qListView)
        {
            int vHdr = 0;

            if (qTableView)
            {
                vHdr = qTableView->verticalHeader()->isVisible() ? qTableView->verticalHeader()->width() : 0;
            }

            optionCopy.rect.setX(vHdr);
            optionCopy.rect.setWidth((qTableView ? qTableView->width() : qListView->width()) - vHdr);
        }

        const auto borderWidth = config.focusBorderWidth;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->translate(0.5 * borderWidth, 0.5 * borderWidth);
        painter->setPen(QPen(config.focusBorderColor, borderWidth));
        painter->setBrush(config.focusFillColor);
        painter->drawRect(QRectF(optionCopy.rect).adjusted(0, 0, -borderWidth, -borderWidth));
        painter->restore();

        return true;
    }

    QRect TableView::itemViewItemRect(const Style* style, QStyle::SubElement element, const QStyleOptionViewItem* option, const QWidget* widget, const Config& config)
    {
        auto tableView = qobject_cast<const TableView*>(widget);
        if (!tableView)
        {
            return {};
        }

        // Only TableViewItemDelegate subclasses have the capability to compute rectangles for SE_ItemViewItem*
        auto delegate = qobject_cast<TableViewItemDelegate*>(tableView->itemDelegate());
        if (!delegate)
        {
            return {};
        }

        return delegate->itemViewItemRect(style, element, option, widget, config);
    }

    QSize TableView::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(contentsSize);
        Q_UNUSED(config);

        auto header = qobject_cast<const QHeaderView*>(widget);
        if (type == QStyle::CT_HeaderSection && header && header->orientation() == Qt::Horizontal)
        {
            QSize sz;
            if (const QStyleOptionHeader *hdr = qstyleoption_cast<const QStyleOptionHeader *>(option))
            {
                bool nullIcon = hdr->icon.isNull();
                int margin = style->pixelMetric(QStyle::PM_HeaderMargin, hdr, widget);
                int iconSize = 0;
                if (!nullIcon)
                {
                    iconSize = style->pixelMetric(QStyle::PM_SmallIconSize, hdr, widget);
                }
                QSize txt = hdr->fontMetrics.size(0, hdr->text);
                sz.setHeight(margin + qMax(iconSize, txt.height()) + margin);
                int width = iconSize + txt.width() + 16 + margin;
                if (!nullIcon)
                {
                    width += margin;
                }
                if (!hdr->text.isNull())
                {
                    width += margin;
                }
                sz.setWidth(width);

                if (hdr->sortIndicator != QStyleOptionHeader::None)
                {
                    if (hdr->orientation == Qt::Horizontal)
                    {
                        sz.rwidth() += sz.height() + margin;
                    }
                    else
                    {
                        sz.rheight() += sz.width() + margin;
                    }
                }
            }
            return sz;
        }

        return {};
    }

    bool TableView::polish(Style* style, QWidget* widget, const ScrollBar::Config& scrollBarConfig, const Config& config)
    {
        Q_UNUSED(config);

        auto tableView = qobject_cast<TableView*>(widget);
        if (!tableView)
        {
            return false;
        }

        // TableView derives from QAbstractScrollArea so make sure we apply that polish too.
        ScrollBar::polish(style, tableView, scrollBarConfig);

        // Nothing to do here

        return true;
    }

    bool TableView::unpolish(Style* style, QWidget* widget, const ScrollBar::Config& scrollBarConfig, const Config& config)
    {
        Q_UNUSED(config);

        auto tableView = qobject_cast<TableView*>(widget);
        if (!tableView)
        {
            return false;
        }

        // TableView derives from QAbstractScrollArea so make sure we apply that unpolish too.
        ScrollBar::unpolish(style, tableView, scrollBarConfig);

        // Reset the indentation so that we can set it from UI 2.0 style sheets using
        // `qproperty-indentation`.
        tableView->resetIndentation();

        // Reset the cached unexpanded row height so that it gets re-calculated via the stylesheet
        auto delegate = qobject_cast<TableViewItemDelegate*>(tableView->itemDelegate());
        if (delegate)
        {
            delegate->clearCachedValues();
        }

        return true;
    }

    TableViewModel* TableView::getTableViewModel() const
    {
        QAbstractItemModel* m = model();

        while (QSortFilterProxyModel* p = qobject_cast<QSortFilterProxyModel*>(m))
        {
            m = p->sourceModel();
        }

        return qobject_cast<TableViewModel*>(m);
    }

    QModelIndex TableView::mapToTableViewModel(QModelIndex unmappedIndex) const
    {
        QAbstractItemModel* m = model();

        QModelIndex mappedIndex;
        while (QSortFilterProxyModel* p = qobject_cast<QSortFilterProxyModel*>(m))
        {
            mappedIndex = p->mapToSource(unmappedIndex);
            m = p->sourceModel();
        }

        if (qobject_cast<TableViewModel*>(m))
        {
            return mappedIndex;
        }

        return {};
    }

    void TableView::selectRow(const QModelIndex& unmappedIndex)
    {
        auto delegate = qobject_cast<TableViewItemDelegate*>(itemDelegate());
        auto model = getTableViewModel();
        if (!delegate || !model)
        {
            return;
        }

        // tell the delegates to clear any caches
        delegate->clearCachedValues();
        
        // default these to known, invalid values
        int height = -1;
        int row = -1;

        if (unmappedIndex.isValid())
        {
            QModelIndex mappedIndex = mapToTableViewModel(unmappedIndex);
            height = delegate->sizeHintForSelectedRow(mappedIndex, this);
            row = mappedIndex.row();
        }

        model->setSelectedRowHeight(row, height);
    }

    void TableView::handleResize()
    {
        auto model = getTableViewModel();
        if (model)
        {
            const QModelIndexList indexes = selectionModel() ? selectionModel()->selectedRows() : QModelIndexList();
            if (!indexes.isEmpty())
            {
                // reselect the same row, so that height/width calculations are re-run
                selectRow(indexes.first());
            }
            
        }
    }

    QVariant TableViewModel::data(const QModelIndex& index, int role) const
    {
        if (hasIndex(index.row(), index.column(), index.parent()))
        {
            switch (role)
            {
            case Qt::SizeHintRole:
            {
                if (m_selectedRow == index.row())
                {
                    return QSize(-1, m_selectedRowHeight);
                }

                break;
            }

            case IsSelectedRole:
                return (m_selectedRow == index.row());
                break;

            default:
                break;
            }
        }

        return {};
    }

    void TableViewModel::setSelectedRowHeight(int selectedRow, int height)
    {
        const int oldRow = m_selectedRow;
        const int oldRowHeight = m_selectedRowHeight;
        m_selectedRow = qBound(-1, selectedRow, rowCount() -1);
        m_selectedRowHeight = m_selectedRow == -1 ? -1 : height;

        if (oldRow != -1 && oldRow != m_selectedRow)
        {
            emit dataChanged(index(oldRow, 0), index(oldRow, columnCount() -1), { Qt::SizeHintRole });
        }

        if (m_selectedRow != -1 && (oldRow != m_selectedRow || oldRowHeight != m_selectedRowHeight))
        {
            emit dataChanged(index(m_selectedRow, 0), index(m_selectedRow, columnCount() -1), { Qt::SizeHintRole });
        }
    }

    int TableViewModel::selectedRowHeight() const
    {
        return m_selectedRowHeight;
    }

    void TableViewModel::resetInternalData()
    {
        m_selectedRow = -1;
        m_selectedRowHeight = -1;
    }

    TableViewItemDelegate::TableViewItemDelegate(TableView* parent)
        : QStyledItemDelegate(parent)
        , m_parentView(parent)
    {
    }

    void TableViewItemDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
    {
        // need to use this, otherwise SVGs won't get properly set to Active when in Selected state
        // as initStyleOption calls into QSvgIconEngine the first time to paint and then cache
        // the icon into a pixmap and Style can only catch it if it knows that it's a TableView
        // it's dealing with.
        Style::DrawWidgetSentinel drawWidgetGuard(option->widget);

        QStyledItemDelegate::initStyleOption(option, index);

        if (option->state.testFlag(QStyle::State_Selected))
        {
            option->features |= QStyleOptionViewItem::WrapText;
        }

        if (m_parentView)
        {
            option->font = m_parentView->font();
        }
    }

    void TableViewItemDelegate::clearCachedValues()
    {
        m_unexpandedRowHeight = -1;
    }

    int TableViewItemDelegate::unexpandedRowHeight(const QModelIndex& index, const QWidget* view) const
    {
        // Asking QStyleSheet how high it would make this row, and saving the value for adapting
        // contents positions on expanded rows.
        // Since the height is fixed, there is no need to do it more than once unless the stylesheet changes.
        if (m_unexpandedRowHeight < 0)
        {
            QStyleOptionViewItem option;
            option.initFrom(view);
            option.state &= ~QStyle::State_Selected;
            option.widget = view;
            initStyleOption(&option, index);
            m_unexpandedRowHeight = view->style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, {}, view).height();
        }

        return m_unexpandedRowHeight;
    }

    int TableViewItemDelegate::sizeHintForSelectedRow(const QModelIndex& mappedIndex, const TableView* view) const
    {
        if (!mappedIndex.isValid())
        {
            return -1;
        }

        const auto header = view->header();
        int height = 0;

        for (int i = 0; i < header->count(); ++i)
        {
            QModelIndex columnIndex = mappedIndex.sibling(mappedIndex.row(), i);
            const int hint = sizeHintForSelectedIndex(columnIndex, view);
            height = qMax(height, hint);
        }

        return height;
    }

    // Copied out of qcommonstyle.cpp, because it's not exposed anywhere useful that we can get at
    static QSizeF viewItemTextLayout(QTextLayout &textLayout, int lineWidth)
    {
        qreal height = 0;
        qreal widthUsed = 0;
        textLayout.beginLayout();
        while (true)
        {
            QTextLine line = textLayout.createLine();
            if (!line.isValid())
            {
                break;
            }

            line.setLineWidth(lineWidth);
            line.setPosition(QPointF(0, height));
            height += line.height();
            widthUsed = qMax(widthUsed, line.naturalTextWidth());
        }
        textLayout.endLayout();
        return QSizeF(widthUsed, height);
    }

    int TableViewItemDelegate::sizeHintForSelectedIndex(const QModelIndex& index, const TableView* view) const
    {
        const auto baseStyle = StyleManager::baseStyle(view);
        const auto header = view->header();
        const int lineWidth = header->sectionSize(index.column());
        QStyleOptionViewItem option;

        option.initFrom(view);
        option.state |= QStyle::State_Selected;
        option.widget = view;
        option.rect = QRect(QPoint(), QSize(lineWidth, 1));
        initStyleOption(&option, index);

        // There is a 1 pixel difference between what compute item view style using QTextLayout and
        // what the underlying Qt code does when rendering, so we adjust for it manually here.
        // This +1 business is all over the place in QCommonStylePrivate in qcommonstyle.cpp and never
        // explained.
        const int randomQtPaintingOffset = 1;
        const auto styleMargin = baseStyle->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, view) + randomQtPaintingOffset;
        int lineHeight = option.fontMetrics.height();
        {
            QTextOption textOption;
            textOption.setWrapMode(QTextOption::WordWrap);

            QTextLayout textLayout(option.text, option.font);
            textLayout.setTextOption(textOption);

            textLayout.beginLayout();
            auto line = textLayout.createLine();
            if (line.isValid())
            {
                line.setLineWidth(lineWidth - (styleMargin * 2));
                line.setPosition(QPointF(0, 0));
                lineHeight = aznumeric_cast<int>(line.height());
            }
            textLayout.endLayout();
        }

        int localUnexpandedRowHeight = unexpandedRowHeight(index, view);

        const auto unexpandedMargin = (localUnexpandedRowHeight - lineHeight) / 2;
        QSize hint = baseStyle->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(), view);

        hint -= QSize(styleMargin * 2, 0);
        hint += QSize(unexpandedMargin, unexpandedMargin * 2);
        hint = hint.expandedTo(QSize(0, localUnexpandedRowHeight));

        // Qt does this weird thing where it sometimes, even with the calculations above, decides
        // to elide the text anyways. So we calculate what the actual text height should be below
        // with wrapping, and then re-adjust the size to that instead if need be.
        const bool wrapText = QStyleOptionViewItem::WrapText;
        QTextOption textOption;
        textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
        QTextLayout textLayout(option.text, option.font);
        textLayout.setTextOption(textOption);

        // add in a fudge factor, because Qt adds some 1's randomly when rendering and we want enough padding
        // that we always calculate a big enough height and eliding doesn't happen.
        const int fudgeFactor = 4;
        QSizeF fullSize = viewItemTextLayout(textLayout, lineWidth - (styleMargin * 2) - fudgeFactor);

        // If the adjustment was unneeded, then leave it be
        if (fullSize.height() <= localUnexpandedRowHeight)
        {
            return hint.height();
        }

        // Otherwise, take the unexpanded margin, driven by the stylesheet, into account
        // with the calculated height.
        return aznumeric_cast<int>(fullSize.height() + (unexpandedMargin * 2));
    }

    bool TableViewItemDelegate::isSelected(const QModelIndex& unmappedIndex) const
    {
        // Use this function to check for is selected instead of going direct
        // because a custom data model that does not inherit from TableViewModel
        // will not necessarily support TableViewModel::IsSelectedRole.
        if (m_parentView && m_parentView->dataModelSupportsIsSelectedRole())
        {
            return unmappedIndex.data(TableViewModel::IsSelectedRole).toBool();
        }

        return false;
    }

    QRect TableViewItemDelegate::itemViewItemRect(const AzQtComponents::Style* style, QStyle::SubElement element, const QStyleOptionViewItem* option, const QWidget* widget, const AzQtComponents::TableView::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(element);
        Q_UNUSED(option);
        Q_UNUSED(widget);
        Q_UNUSED(config);
        return QRect();
    }

    void TableViewItemDelegate::geometriesUpdating(TableView* /*tableView*/)
    {
    }
}; // namespace AzQtComponents

#include "Components/Widgets/moc_TableView.cpp"
