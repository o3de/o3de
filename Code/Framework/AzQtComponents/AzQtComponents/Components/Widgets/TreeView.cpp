/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/TreeView.h>

#include <QDrag>
#include <QEvent>
#include <QSettings>
#include <QPainter>

#include <AzCore/std/algorithm.h>

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/ConfigHelpers.h>

#include <QtWidgets/private/qstylesheetstyle_p.h>

namespace AzQtComponents
{
    constexpr const char* g_branchLinesEnabledProperty = "BranchLinesEnabled";

    class TreeViewWatcher
        : public QObject
    {
    public:
        TreeViewWatcher(QObject* parent = nullptr)
            : QObject(parent)
        {
        }

        bool eventFilter(QObject* obj, QEvent* event) override
        {
            if (qobject_cast<QTreeView*>(obj) && !qobject_cast<const TableView*>(obj))
            {
                switch (event->type())
                {
                    case QEvent::DynamicPropertyChange:
                    {
                        auto widget = qobject_cast<QWidget*>(obj);
                        auto styleSheet = StyleManager::styleSheetStyle(widget);
                        if (styleSheet)
                        {
                            styleSheet->repolish(widget);
                        }
                        widget->update();
                        break;
                    }
                }
            }

            return QObject::eventFilter(obj, event);
        }
    };

    bool hasVisibleChildren(const QTreeView* treeView, const QModelIndex& parent)
    {
        if (parent.flags().testFlag(Qt::ItemNeverHasChildren))
        {
            return false;
        }
        const auto *model = treeView->model();
        Q_ASSERT(model);
        if (model->hasChildren(parent))
        {
            const auto rowCount = model->rowCount(parent);
            for (int i = 0; i < rowCount; ++i)
            {
                if (!treeView->isRowHidden(i, parent))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool BranchDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex &index)
    {
        auto copy = option;
        if (auto treeView = qobject_cast<const QTreeView*>(option.styleObject))
        {
            // Unfortunately it looks like we can't test the State_Children flag here;
            // it's never set, not even for indices with children
            if (TreeView::isBranchLinesEnabled(qobject_cast<const QTreeView*>(treeView)) && hasVisibleChildren(treeView, index))
            {
                copy.rect.setLeft(copy.rect.left() + treeView->indentation());
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, copy, index);
    }

    void BranchDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        auto copy = option;
        if (auto treeView = qobject_cast<const QTreeView*>(option.styleObject))
        {
            // See comment above on State_Children
            if (TreeView::isBranchLinesEnabled(treeView) && hasVisibleChildren(treeView, index))
            {
                copy.rect.setLeft(copy.rect.left() + treeView->indentation());
            }
        }
        QStyledItemDelegate::updateEditorGeometry(editor, copy, index);
    }

    TreeView::Config TreeView::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();
        settings.beginGroup(QStringLiteral("TreeView"));
        ConfigHelpers::read<int>(settings, QStringLiteral("BranchLineWidth"), config.branchLineWidth);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("BranchLineColor"), config.branchLineColor);
        settings.endGroup();
        return config;
    }

    TreeView::Config TreeView::defaultConfig()
    {
        Config config;
        config.branchLineWidth = 1;
        config.branchLineColor = QStringLiteral("#878787");
        return config;
    }

    void TreeView::setBranchLinesEnabled(QTreeView* widget, bool enabled)
    {
        widget->setProperty(g_branchLinesEnabledProperty, enabled);
    }

    bool TreeView::isBranchLinesEnabled(const QTreeView* widget)
    {
        return widget->property(g_branchLinesEnabledProperty).toBool();
    }

    QPointer<TreeViewWatcher> TreeView::s_treeViewWatcher = nullptr;
    unsigned int TreeView::s_watcherReferenceCount = 0;

    void TreeView::initializeWatcher()
    {
        if (!s_treeViewWatcher)
        {
            Q_ASSERT(s_watcherReferenceCount == 0);
            s_treeViewWatcher = new TreeViewWatcher;
        }

        ++s_watcherReferenceCount;
    }

    void TreeView::uninitializeWatcher()
    {
        Q_ASSERT(!s_treeViewWatcher.isNull());
        Q_ASSERT(s_watcherReferenceCount > 0);

        --s_watcherReferenceCount;

        if (s_watcherReferenceCount == 0)
        {
            delete s_treeViewWatcher;
            s_treeViewWatcher = nullptr;
        }
    }

    bool TreeView::drawBranchIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        auto treeView = qobject_cast<const QTreeView*>(widget);
        if (!(treeView && !qobject_cast<const TableView*>(widget)) || !qobject_cast<BranchDelegate*>(treeView->itemDelegate()))
        {
            return false;
        }

        if (auto itemOption = qstyleoption_cast<const QStyleOptionViewItem*>(option))
        {
            // HACK: make sure we clear the State_Children flag so that the base style
            // doesn't paint the arrow
            auto copy = *itemOption;
            if (copy.state.testFlag(QStyle::State_Children) && isBranchLinesEnabled(qobject_cast<const QTreeView*>(treeView)))
            {
                // also extend the rectangle so that it covers the arrow background
                copy.rect.adjust(treeView->indentation(), 0, treeView->indentation(), 0);
                style->baseStyle()->drawPrimitive(QStyle::PE_IndicatorBranch, &copy, painter, widget);
                copy.state.setFlag(QStyle::State_Children, false);
                copy.rect.adjust(-treeView->indentation(), 0, -treeView->indentation(), 0);
                style->baseStyle()->drawPrimitive(QStyle::PE_IndicatorBranch, &copy, painter, widget);
            }
            else
            {
                style->baseStyle()->drawPrimitive(QStyle::PE_IndicatorBranch, &copy, painter, widget);
            }
        }

        // Paint branch lines
        if (isBranchLinesEnabled(qobject_cast<const QTreeView*>(treeView)))
        {
            painter->save();
            painter->setPen(QPen(config.branchLineColor, config.branchLineWidth));

            const auto centerX = option->rect.center().x();
            const auto centerY = option->rect.center().y();

            const auto top = option->rect.top();
            const auto bottom = option->rect.bottom();
            const auto right = option->rect.right();

            switch (option->state & (QStyle::State_Sibling | QStyle::State_Item))
            {
                case QStyle::State_Sibling:
                    painter->drawLine(centerX, top, centerX, bottom);
                    break;

                case QStyle::State_Sibling | QStyle::State_Item:
                    painter->drawLine(centerX, top, centerX, bottom);
                    painter->drawLine(centerX, centerY, right, centerY);
                    break;

                case QStyle::State_Item:
                    painter->drawLine(centerX, top, centerX, centerY);
                    painter->drawLine(centerX, centerY, right, centerY);
                    break;
            }

            painter->restore();
        }

        return true;
    }

    bool TreeView::polish(Style* style, QWidget* widget, const ScrollBar::Config& scrollBarConfig, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        auto treeView = qobject_cast<QTreeView*>(widget);
        if (treeView && !qobject_cast<TableView*>(widget))
        {
            ScrollBar::polish(style, widget, scrollBarConfig);
            treeView->installEventFilter(s_treeViewWatcher);
            treeView->setIndentation(IndentationWidth);
            return true;
        }

        return false;
    }

    bool TreeView::unpolish(Style* style, QWidget* widget, const ScrollBar::Config& scrollBarConfig, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(scrollBarConfig);
        Q_UNUSED(config);

        return qobject_cast<QTreeView*>(widget) && !qobject_cast<TableView*>(widget);
    }

    StyledTreeView::StyledTreeView(QWidget* parent)
        : QTreeView(parent)
    {
    }

    void StyledTreeView::startDrag(Qt::DropActions supportedActions)
    {
        if (!selectionModel()->selectedIndexes().empty())
        {
            StartCustomDrag(selectionModel()->selectedIndexes(), supportedActions);
        }
    }

    void StyledTreeView::StartCustomDrag(const QModelIndexList& indexList, Qt::DropActions supportedActions)
    {
        StartCustomDragInternal(this, indexList, supportedActions);
    }

    void StyledTreeView::StartCustomDragInternal(QAbstractItemView* itemView, const QModelIndexList& indexList, Qt::DropActions supportedActions)
    {
        QMimeData* mimeData = itemView->model()->mimeData(indexList);
        if (mimeData)
        {
            QDrag* drag = new QDrag(itemView);
            drag->setPixmap(QPixmap::fromImage(CreateDragImage(itemView, indexList)));
            drag->setMimeData(mimeData);

            Qt::DropAction defDropAction = Qt::IgnoreAction;
            if (itemView->defaultDropAction() != Qt::IgnoreAction && (supportedActions & itemView->defaultDropAction()))
            {
                defDropAction = itemView->defaultDropAction();
            }
            else if (supportedActions & Qt::CopyAction && itemView->dragDropMode() != QAbstractItemView::InternalMove)
            {
                defDropAction = Qt::CopyAction;
            }

            drag->exec(supportedActions, defDropAction);
        }
    }

    QImage StyledTreeView::CreateDragImage(QAbstractItemView* itemView, const QModelIndexList& indexList)
    {
        // Generate a drag image of the item icon and text, normally done internally, and inaccessible
        QRect rect(0, 0, 0, 0);
        for (const auto& index : indexList)
        {
            if (index.column() != 0)
            {
                continue;
            }

            QRect itemRect = itemView->visualRect(index);
            rect.setHeight(rect.height() + itemRect.height());
            rect.setWidth(AZStd::GetMax(rect.width(), itemRect.width()));
        }

        QImage dragImage(rect.size(), QImage::Format_ARGB32_Premultiplied);

        QPainter dragPainter(&dragImage);
        dragPainter.setCompositionMode(QPainter::CompositionMode_Source);
        dragPainter.fillRect(dragImage.rect(), Qt::transparent);
        dragPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        dragPainter.setOpacity(0.35f);
        dragPainter.fillRect(rect, QColor("#222222"));
        dragPainter.setOpacity(1.0f);

        int imageY = 0;
        for (const auto& index : indexList)
        {
            if (index.column() != 0)
            {
                continue;
            }

            QRect itemRect = itemView->visualRect(index);
            dragPainter.drawPixmap(QPoint(4, imageY + 4),
                itemView->model()->data(index, Qt::DecorationRole).value<QIcon>().pixmap(QSize(16, 16)));
            dragPainter.setPen(
                itemView->model()->data(index, Qt::ForegroundRole).value<QBrush>().color());
            dragPainter.setFont(
                itemView->font());
            dragPainter.drawText(QRect(24, imageY + 4, rect.width() - 20, rect.height()),
                itemView->model()->data(index, Qt::DisplayRole).value<QString>());
            imageY += itemRect.height();
        }

        dragPainter.end();
        return dragImage;
    }

    StyledTreeWidget::StyledTreeWidget(QWidget* parent)
        : QTreeWidget(parent)
    {
    }

    void StyledTreeWidget::startDrag(Qt::DropActions supportedActions)
    {
        if (!selectionModel()->selectedIndexes().empty())
        {
            StyledTreeView::StartCustomDragInternal(this, selectionModel()->selectedIndexes(), supportedActions);
        }
    }

} // namespace AzQtComponents
#include <Components/Widgets/moc_TreeView.cpp>
