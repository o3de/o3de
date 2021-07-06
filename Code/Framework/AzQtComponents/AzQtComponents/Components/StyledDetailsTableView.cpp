/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/StyledDetailsTableView.h>
#include <AzQtComponents/Components/StyledDetailsTableModel.h>

#include <QStyledItemDelegate>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QProxyStyle>
#include <QPainter>
#include <QApplication>
#include <QStyleFactory>
#include <QDebug>
#include <QScrollBar>
#include <QKeyEvent>
#include <QMimeData>
#include <QClipboard>
#include <QMenu>
#include <QTimer>

namespace AzQtComponents
{
    static const int StyledTreeDetailsPadding = 20;

    struct StyledDetailsTableDetailsInfo
    {
        StyledDetailsTableDetailsInfo(const QStyleOptionViewItem& opt, const QModelIndex& index)
        {
            if (auto view = qobject_cast<const QTableView*>(opt.widget))
            {
                auto firstCol = index.sibling(index.row(), 0);
                const auto data = firstCol.data(StyledDetailsTableModel::Details);
                if (data.isValid())
                {
                    option = opt;
                    option.rect.setLeft(StyledTreeDetailsPadding);
                    option.rect.setRight(view->horizontalHeader()->length() - StyledTreeDetailsPadding);

                    // for some strange reason, and not well-documented, you have to use QChar::LineSeparator
                    // so that the view actually renders newlines.
                    option.text = data.toString().replace(QChar('\n'), QChar::LineSeparator);

                    option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::WrapText;
                    option.state &= ~(QStyle::State_Selected);
                    option.icon = {};
                    option.displayAlignment = Qt::Alignment(Qt::AlignVCenter | Qt::AlignLeft);
                    sizeHint = view->style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, {}, view);
                    sizeHint.rheight() += StyledTreeDetailsPadding * 2;
                    option.rect.setTop(option.rect.bottom() - sizeHint.height());
                }
            }
        }

        QSize sizeHint = { 0, 0 };
        QStyleOptionViewItem option;
    };

    class StyledTableStyle : public QProxyStyle
    {
    public:
        explicit StyledTableStyle(QObject* parent)
            : QProxyStyle()
        {
            setParent(parent);
        }

        QRect subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const override
        {
            auto rect = QProxyStyle::subElementRect(element, option, widget);
            switch (element)
            {
                case SE_ItemViewItemText:
                case SE_ItemViewItemCheckIndicator:
                case SE_ItemViewItemDecoration:
                    if (option->state.testFlag(State_Selected))
                    {
                        auto vOpt = static_cast<const QStyleOptionViewItem*>(option);
                        const auto offset = StyledDetailsTableDetailsInfo(*vOpt, vOpt->index).sizeHint.height();
                        if (element == SE_ItemViewItemText)
                        {
                            rect.setBottom(rect.bottom() - offset);
                        }
                        else
                        {
                            rect.setTop(rect.top() - offset);
                        }
                    }
                    if (element == SE_ItemViewItemDecoration)
                    {
                        rect.moveLeft(rect.left() + StyledTreeDetailsPadding);
                    }
                    break;
                default:
                    break;
            }
            return rect;
        }

        void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                           QPainter* painter, const QWidget* widget) const override
        {
            switch (element)
            {
            case PE_PanelItemViewItem:
            case PE_PanelItemViewRow:
            {
                const auto cg = !option->state.testFlag(State_Enabled) ? QPalette::Disabled
                              : option->state.testFlag(State_Active) ? QPalette::Normal
                              : QPalette::Inactive;

                if (option->state.testFlag(State_Selected))
                {
                    painter->fillRect(option->rect, option->palette.brush(cg, QPalette::Highlight));
                }
                else if (auto vOpt = qstyleoption_cast<const QStyleOptionViewItem*>(option))
                {
                    if (vOpt->features.testFlag(QStyleOptionViewItem::Alternate))
                    {
                        painter->fillRect(option->rect, option->palette.brush(cg, QPalette::AlternateBase));
                    }
                }
                break;
            }
            default:
                QProxyStyle::drawPrimitive(element, option, painter, widget);
                break;
            }
        }
    };

    class StyledDetailsTableDelegate : public QStyledItemDelegate
    {
    public:
        StyledDetailsTableDelegate(StyledDetailsTableView* table)
            : QStyledItemDelegate(table)
            , m_table(table)
        {
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override
        {
            auto copy = opt;
            initStyleOption(&copy, index);

            PrecalculateHeights(opt, index);

            QVariant decorationData = index.data(Qt::DecorationRole);
            if (decorationData.isNull())
            {
                // draw the item once, without text, so that we get the proper background rendering of everything
                auto noTextOptions = copy;
                noTextOptions.text = "";
                DrawItemViewItem(painter, noTextOptions);

                // draw the item again, with the text this time, but with the adjusted height;
                // we need to specify the height here because it was mucked with earlier for
                // the Details and we want the text centered vertically within the original box
                auto textOptions = copy;
                textOptions.state &= ~(QStyle::State_Selected);
                textOptions.rect.setHeight(m_maximumTextHeights[index.row()]);
                textOptions.displayAlignment = Qt::Alignment(Qt::AlignVCenter | Qt::AlignLeft);
                DrawItemViewItem(painter, textOptions);
            }
            else
            {
                // draw the cell without the pixmap so that the background draws properly
                auto pixmapOptions = copy;
                pixmapOptions.icon = {};
                DrawItemViewItem(painter, pixmapOptions);

                // draw the pixmap, centered according to the maximum text height precalculated for this row
                QPixmap pix = qvariant_cast<QPixmap>(decorationData);
                int maxTextHeight = m_maximumTextHeights[index.row()];
                QPoint pos = { pixmapOptions.rect.center().x(), pixmapOptions.rect.top() + (maxTextHeight  / 2) - (pix.height() / 2) };
                painter->drawPixmap(pos, pix);
            }

            if (!m_detailsOptions.contains(index.row()) &&
                    (opt.state.testFlag(QStyle::State_Selected) ||
                     index.data(StyledDetailsTableModel::HasOnlyDetails).toBool()))
            {
                m_detailsOptions.insert(index.row(), StyledDetailsTableDetailsInfo(copy, index).option);
            }
        }

        void DrawDetails(QWidget* viewport) const
        {
            QPainter painter(viewport);
            for (const auto &opt: m_detailsOptions)
            {
                DrawItemViewItem(&painter, opt);
            }
            m_detailsOptions.clear();

            m_precalculatedHeights.clear();
            m_maximumTextHeights.clear();
        }

        void DrawItemViewItem(QPainter* painter, const QStyleOptionViewItem& option) const
        {
            const auto widget = option.widget;
            const auto style = widget ? widget->style() : qApp->style();
            
            QStyleOptionViewItem copy = option;
            copy.state &= ~(QStyle::State_Selected);
            style->drawControl(QStyle::CE_ItemViewItem, &copy, painter, widget);
        }

        QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override
        {
            QStyleOptionViewItem copy = opt;
            initStyleOption(&copy, index);
            const auto widget = opt.widget;
            const auto style = widget ? widget->style() : qApp->style();
            auto size = style->sizeFromContents(QStyle::CT_ItemViewItem, &copy, QSize(), widget);

            // option not fully initialized (no State_Selected), use the widget
            const auto view = qobject_cast<const QAbstractItemView*>(widget);
            if (index.data(StyledDetailsTableModel::HasOnlyDetails).toBool())
            {
                size.rheight() = StyledDetailsTableDetailsInfo(opt, index).sizeHint.height();
            }
            else if (view && view->selectionModel()->isSelected(index))
            {
                size.rheight() += StyledDetailsTableDetailsInfo(opt, index).sizeHint.height();
            }

            if (copy.features.testFlag(QStyleOptionViewItem::HasDecoration))
            {
                size.rwidth() += StyledTreeDetailsPadding * 2;
            }

            return size;
        }

        void Reset()
        {
            m_detailsOptions.clear();
            m_precalculatedHeights.clear();
            m_maximumTextHeights.clear();
        }

    protected:

        void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
        {
            QStyledItemDelegate::initStyleOption(option, index);
            option->decorationAlignment = Qt::Alignment(Qt::AlignTop | Qt::AlignHCenter);
            option->decorationPosition = QStyleOptionViewItem::Left;
            // Don't show focused state
            option->state &= ~(QStyle::State_HasFocus);
        }

        void PrecalculateHeights(const QStyleOptionViewItem& opt, const QModelIndex& index) const
        {
            // check if we've already calculated the text heights for this row
            if (m_precalculatedHeights.contains(index.row()))
            {
                return;
            }
            const auto widget = opt.widget;
            const auto style = widget ? widget->style() : qApp->style();

            // need to calculate it now; do some work for every row, even the invisible ones
            int columnCount = m_table->model()->columnCount(QModelIndex());
            for (int i = 0; i < columnCount; i++)
            {
                QModelIndex tempIndex = index.sibling(index.row(), i);

                QVariant decorationData = tempIndex.data(Qt::DecorationRole);
                if (!decorationData.isNull())
                {
                    continue;
                }

                QString textData = tempIndex.data(Qt::DisplayRole).toString();

                // figure out what the original height of the cell should be
                // pretending it's not selected so we won't get any special details heights added
                auto textOptions = opt;
                textOptions.state &= ~(QStyle::State_Selected);
                textOptions.features |= QStyleOptionViewItem::WrapText;
                textOptions.features |= QStyleOptionViewItem::HasDisplay;
                textOptions.text = textData;
                textOptions.rect.setWidth(m_table->columnWidth(i));
                QSize originalContentsSize = style->sizeFromContents(QStyle::CT_ItemViewItem, &textOptions, {}, widget);

                m_precalculatedHeights[tempIndex.row()][i] = originalContentsSize.height();

                if (m_maximumTextHeights[tempIndex.row()] < originalContentsSize.height())
                {
                    m_maximumTextHeights[tempIndex.row()] = originalContentsSize.height();
                }
            }
        }

    private:
        StyledDetailsTableView* m_table;
        mutable QHash<int, QStyleOptionViewItem> m_detailsOptions;
        mutable QHash<int, QHash<int, int>> m_precalculatedHeights;
        mutable QHash<int, int> m_maximumTextHeights;
    };

    StyledDetailsTableView::StyledDetailsTableView(QWidget* parent)
        : QTableView(parent)
        , m_resizeTimer(new QTimer(this))
    {
        setStyle(new StyledTableStyle(qApp));
        setAlternatingRowColors(true);
        setSelectionMode(SingleSelection);
        setSelectionBehavior(SelectRows);
        setShowGrid(false);
        setItemDelegate(new StyledDetailsTableDelegate(this));
        setSortingEnabled(true);

        verticalHeader()->hide();

        auto font = horizontalHeader()->font();
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        horizontalHeader()->setFont(font);
        horizontalHeader()->setStyle(qApp->style());
        horizontalHeader()->setHighlightSections(false);
        horizontalHeader()->setStretchLastSection(true);
        horizontalHeader()->setDefaultAlignment(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter));
        setContextMenuPolicy(Qt::ActionsContextMenu);

        QAction* copyAction = new QAction(tr("Copy"), this);
        copyAction->setShortcut(QKeySequence::Copy);
        connect(copyAction, &QAction::triggered, this, &StyledDetailsTableView::copySelectionToClipboard);
        addAction(copyAction);

        m_resizeTimer->setSingleShot(true);
        m_resizeTimer->setInterval(0);
        connect(m_resizeTimer, &QTimer::timeout, this, [this]()
        {
            resizeRowsToContents();
            if (m_scrollOnInsert)
            {
                m_scrollOnInsert = false;
                scrollToBottom();
            }

            scheduleDelayedItemsLayout();
        });

        auto startResizeTimer = static_cast<void(QTimer::*)(void)>(&QTimer::start);
        connect(horizontalHeader(), &QHeaderView::geometriesChanged, m_resizeTimer, startResizeTimer);
        connect(horizontalHeader(), &QHeaderView::sectionResized, m_resizeTimer, startResizeTimer);

    }

    void StyledDetailsTableView::setModel(QAbstractItemModel* model)
    {
        if (selectionModel())
        {
            selectionModel()->disconnect(this);
        }

        if (model)
        {
            model->disconnect(this);
        }

        QTableView::setModel(model);

        if (model)
        {
            auto startResizeTimer = static_cast<void(QTimer::*)(void)>(&QTimer::start);
            connect(model, &QAbstractItemModel::layoutChanged, m_resizeTimer, startResizeTimer);
            connect(model, &QAbstractItemModel::rowsInserted, m_resizeTimer, startResizeTimer);

            connect(model, &QAbstractItemModel::rowsAboutToBeInserted, this, [this]
            {
                m_scrollOnInsert = !selectionModel()->hasSelection()
                    && (verticalScrollBar()->value() == verticalScrollBar()->maximum());
            });
            connect(model, &QAbstractItemModel::dataChanged, this,
                [this](const QModelIndex&, const QModelIndex&, const QVector<int>& roles)
            {
                if (roles.contains(StyledDetailsTableModel::Details))
                {
                    updateItemSelection(selectionModel()->selection());
                }
            });
        }

        if (selectionModel())
        {
            connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
                    [this](const QItemSelection& sel, const QItemSelection& desel)
            {
                updateItemSelection(desel);
                updateItemSelection(sel);
            });
        }
    }

    void StyledDetailsTableView::ResetDelegate()
    {
        auto delegate = static_cast<StyledDetailsTableDelegate*>(itemDelegate());
        if (delegate)
        {
            delegate->Reset();
        }
    }

    void StyledDetailsTableView::paintEvent(QPaintEvent* ev)
    {
        auto delegate = static_cast<StyledDetailsTableDelegate*>(itemDelegate());
        QTableView::paintEvent(ev);
        delegate->DrawDetails(viewport());
    }

    void StyledDetailsTableView::keyPressEvent(QKeyEvent* ev)
    {
        QTableView::keyPressEvent(ev);
    }

    QItemSelectionModel::SelectionFlags StyledDetailsTableView::selectionCommand(
        const QModelIndex& index, const QEvent* event) const
    {
        auto base = QTableView::selectionCommand(index, event);
        if (!selectionModel()->isSelected(index) || event->type() == QEvent::MouseMove ||
            !base.testFlag(QItemSelectionModel::ClearAndSelect))
        {
            return base;
        }
        if (event->type() == QEvent::MouseButtonPress)
        {
            auto mEv = static_cast<const QMouseEvent*>(event);
            if (mEv->button() != Qt::LeftButton)
            {
                return base;
            }
        }
        // Toggle selection off during selection event if already selected
        return QItemSelectionModel::Rows | QItemSelectionModel::Deselect;
    }

    void StyledDetailsTableView::copySelectionToClipboard()
    {
        const auto selection = selectionModel()->selection();
        if (selection.isEmpty())
        {
            return;
        }


        auto index = selection.first().topLeft();

        const QString details = index.data(StyledDetailsTableModel::Details).toString();
        QStringList cells;
        if (!index.data(StyledDetailsTableModel::HasOnlyDetails).toBool())
        {
            while (index.isValid())
            {
                cells.append(index.data(Qt::DisplayRole).toString());
                index = index.sibling(index.row(), index.column() + 1);
            }
        }

        auto clipboard = qApp->clipboard();
        auto qdata = new QMimeData();
        {
            const static auto textFormat = QStringLiteral("%1\n%2");
            qdata->setText(textFormat.arg(cells.join(QChar::fromLatin1('\t')), details).trimmed());
        }
        {
            const static auto htmlFormat = QStringLiteral("<table><tr>%1</tr><tr colspan=%2>%3</tr></table>");
            const static auto htmlCellFormat = QStringLiteral("<td>%1</td>");
            const static auto cellsToHtml = [](const QStringList cells)
            {
                QString row;
                for (const auto &cell: cells)
                {
                    row += htmlCellFormat.arg(cell);
                }
                return row;
            };
            qdata->setHtml(htmlFormat.arg(cellsToHtml(cells),
                                         QString::number(model()->columnCount()),
                                         htmlCellFormat.arg(details)));
        }
        clipboard->setMimeData(qdata);
    }

    void StyledDetailsTableView::updateItemSelection(const QItemSelection& selection)
    {
        for (const auto& range : selection)
        {
            for (int i = range.top(); i <= range.bottom(); ++i)
            {
                resizeRowToContents(i);
            }
        }
    }

} // namespace AzQtComponents

#include "Components/moc_StyledDetailsTableView.cpp"
