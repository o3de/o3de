/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QBoxLayout>
#include <QEvent>
#include <QKeyEvent>
#include <QCompleter>
#include <QCoreApplication>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QSignalBlocker>
#include <QScrollBar>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <StaticLib/GraphCanvas/Widgets/NodePalette/ui_NodePaletteWidget.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/Model/NodePaletteSortFilterProxyModel.h>

namespace GraphCanvas
{
    ////////////////////////////
    // NodePaletteTreeDelegate
    ////////////////////////////
    NodePaletteTreeDelegate::NodePaletteTreeDelegate(QWidget* parent)
        : IconDecoratedNameDelegate(parent)
    {
    }

    void NodePaletteTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        if (index.column() == NodePaletteTreeItem::Column::Name)
        {
            painter->save();

#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
            QStyleOptionViewItemV4 options = option;
#else
            QStyleOptionViewItem options = option;
#endif
            initStyleOption(&options, index);

            QModelIndex sourceIndex = static_cast<const NodePaletteSortFilterProxyModel*>(index.model())->mapToSource(index);
            NodePaletteTreeItem* treeItem = static_cast<NodePaletteTreeItem*>(sourceIndex.internalPointer());

            if (treeItem)
            {
                // Make the text slightly transparent if the item is disabled
                if (!treeItem->IsEnabled())
                {
                    QVariant roleColor = index.data(Qt::ForegroundRole);
                    QColor textColor = (roleColor.type() == QVariant::Type::Color)
                        ? roleColor.value<QColor>() : options.palette.color(QPalette::Text);

                    int fontAlpha = aznumeric_cast<int>(textColor.alpha() * 0.5f);
                    fontAlpha = AZStd::min(AZStd::min(fontAlpha, 127), textColor.alpha());

                    textColor.setAlpha(fontAlpha);

                    options.palette.setColor(QPalette::Text, textColor);
                }
            }

            // paint the original node item
            IconDecoratedNameDelegate::paint(painter, options, index);

            const int textMargin = options.widget->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, options.widget) + 1;
            QRect textRect = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options);
            textRect = textRect.adjusted(textMargin, 0, -textMargin, 0);

            if (treeItem && treeItem->HasHighlight())
            {
                // pos, len
                const AZStd::pair<int, int>& highlight = treeItem->GetHighlight();
                QString preSelectedText = options.text.left(highlight.first);
                int preSelectedTextLength = options.fontMetrics.horizontalAdvance(preSelectedText);
                QString selectedText = options.text.mid(highlight.first, highlight.second);
                int selectedTextLength = options.fontMetrics.horizontalAdvance(selectedText);
                
                int leftSpot = textRect.left() + preSelectedTextLength;

                // Only need to do the draw if we actually are doing to be highlighting the text.                
                if (leftSpot < textRect.right())
                {
                    int visibleLength = AZStd::GetMin(selectedTextLength, textRect.right() - leftSpot);
                    QRect highlightRect(textRect.left() + preSelectedTextLength + 4, textRect.top(), visibleLength, textRect.height());

                    // paint the highlight rect
                    painter->fillRect(highlightRect, options.palette.highlight());
                }
            }

            painter->restore();
        }
        else
        {
            IconDecoratedNameDelegate::paint(painter, option, index);
        }
    }

    //////////////////////
    // NodePaletteWidget
    //////////////////////
    NodePaletteWidget::NodePaletteWidget(QWidget* parent)
        : QWidget(parent)        
        , m_ui(new Ui::NodePaletteWidget())
        , m_itemDelegate(nullptr)        
        , m_model(nullptr)
        , m_isInContextMenu(false)
        , m_searchFieldSelectionChange(false)
    {        
    }

    NodePaletteWidget::~NodePaletteWidget()
    {
        GraphCanvasTreeModelRequestBus::Handler::BusDisconnect();
    }

    void NodePaletteWidget::SetupNodePalette(const NodePaletteConfig& paletteConfig)
    {
        m_editorId = paletteConfig.m_editorId;
        m_mimeType = paletteConfig.m_mimeType;

        m_isInContextMenu = paletteConfig.m_isInContextMenu;

        m_model = aznew NodePaletteSortFilterProxyModel(this);

        m_saveIdentifier = AZStd::string::format("NodePalette_%s", paletteConfig.m_saveIdentifier.data());

        m_filterTimer.setInterval(250);
        m_filterTimer.setSingleShot(true);
        m_filterTimer.stop();

        QObject::connect(&m_filterTimer, &QTimer::timeout, this, &NodePaletteWidget::UpdateFilter);

        m_ui->setupUi(this);

        m_ui->searchFilter->setClearButtonEnabled(true);
        QObject::connect(m_ui->searchFilter, &QLineEdit::textChanged, this, &NodePaletteWidget::OnFilterTextChanged);
        QObject::connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &NodePaletteWidget::OnRowsAboutToBeRemoved);

        if (paletteConfig.m_allowArrowKeyNavigation)
        {
            m_ui->searchFilter->installEventFilter(this);
        }

        GraphCanvasTreeModel* sourceModel = aznew GraphCanvasTreeModel(paletteConfig.m_rootTreeItem, this);
        sourceModel->setMimeType(paletteConfig.m_mimeType);

        GraphCanvasTreeModelRequestBus::Handler::BusConnect(sourceModel);

        m_model->setSourceModel(sourceModel);
        m_model->PopulateUnfilteredModel();

        m_ui->treeView->setModel(m_model);

        if (m_isInContextMenu)
        {
            m_ui->searchFilter->setCompleter(m_model->GetCompleter());
        }

        SetItemDelegate(aznew NodePaletteTreeDelegate(this));

        QObject::connect(m_ui->treeView->verticalScrollBar(), &QScrollBar::valueChanged, this, &NodePaletteWidget::OnScrollChanged);

        if (!m_isInContextMenu)
        {
            QObject::connect(m_ui->searchFilter, &QLineEdit::returnPressed, this, &NodePaletteWidget::UpdateFilter);

            if (paletteConfig.m_clearSelectionOnSceneChange)
            {
                AssetEditorNotificationBus::Handler::BusConnect(m_editorId);
            }
        }
        else
        {
            QObject::connect(m_ui->searchFilter, &QLineEdit::returnPressed, this, &NodePaletteWidget::TrySpawnItem);

            // If the widget is in a context menu, reapply the Editor stylesheet
            AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral("style:Editor.qss"));
        }

        QObject::connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NodePaletteWidget::OnSelectionChanged);

        m_ui->treeView->InitializeTreeViewSaving(AZ::Crc32(m_saveIdentifier.c_str()));

        if (!m_isInContextMenu)
        {
            m_ui->treeView->ApplyTreeViewSnapshot();
        }

        m_ui->treeView->PauseTreeViewSaving();

        m_ui->m_categoryLabel->SetElideMode(Qt::ElideLeft);

        GetTreeView()->header()->setStretchLastSection(false);
        GetTreeView()->header()->setSectionResizeMode(GraphCanvas::NodePaletteTreeItem::Column::Name, QHeaderView::ResizeMode::Stretch);
        GetTreeView()->header()->setSectionResizeMode(GraphCanvas::NodePaletteTreeItem::Column::Customization, QHeaderView::ResizeMode::ResizeToContents);
    }

    void NodePaletteWidget::FocusOnSearchFilter()
    {
        m_ui->searchFilter->setFocus(Qt::FocusReason::MouseFocusReason);
    }

    void NodePaletteWidget::ResetDisplay()
    {
        m_contextMenuCreateEvent = nullptr;

        {
            QSignalBlocker blocker(m_ui->searchFilter);
            m_ui->searchFilter->clear();

            m_model->ClearFilter();
            m_model->invalidate();
        }

        {
            QSignalBlocker blocker(m_ui->treeView->selectionModel());
            m_ui->treeView->clearSelection();
            m_ui->treeView->selectionModel()->clearSelection();
            m_ui->treeView->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::SelectionFlag::ClearAndSelect);
        }

        m_ui->treeView->collapseAll();
        m_ui->m_categoryLabel->setText("");

        setVisible(true);
    }

    GraphCanvasMimeEvent* NodePaletteWidget::GetContextMenuEvent() const
    {
        return m_contextMenuCreateEvent.get();
    }

    void NodePaletteWidget::ResetSourceSlotFilter()
    {
        m_model->ResetSourceSlotFilter();
        m_ui->searchFilter->setCompleter(m_model->GetCompleter());
    }

    void NodePaletteWidget::FilterForSourceSlot(const AZ::EntityId& sceneId, const AZ::EntityId& sourceSlotId)
    {
        m_model->FilterForSourceSlot(sceneId, sourceSlotId);
        m_ui->searchFilter->setCompleter(m_model->GetCompleter());
    }

    void NodePaletteWidget::SetItemDelegate(NodePaletteTreeDelegate* itemDelegate)
    {
        m_ui->treeView->setItemDelegate(itemDelegate);

        delete m_itemDelegate;
        m_itemDelegate = itemDelegate;
    }

    void NodePaletteWidget::AddSearchCustomizationWidget(QWidget* widget)
    {
        m_ui->searchCustomization->layout()->addWidget(widget);
    }

    void NodePaletteWidget::ConfigureSearchCustomizationMargins(const QMargins& margins, int elementSpacing)
    {
        m_ui->searchCustomization->layout()->setContentsMargins(margins);
        m_ui->searchCustomization->layout()->setSpacing(elementSpacing);
    }

    void NodePaletteWidget::PreOnActiveGraphChanged()
    {
        ClearSelection();

        /*
        if (!m_model->HasFilter())
        {
        m_ui->treeView->UnpauseTreeViewSaving();
        m_ui->treeView->CaptureTreeViewSnapshot();
        m_ui->treeView->PauseTreeViewSaving();
        }
        */

        m_ui->treeView->model()->layoutAboutToBeChanged();
    }

    void NodePaletteWidget::PostOnActiveGraphChanged()
    {
        m_ui->treeView->model()->layoutChanged();


        /*if (!m_model->HasFilter())
        {
        m_ui->treeView->UnpauseTreeViewSaving();
        m_ui->treeView->ApplyTreeViewSnapshot();
        m_ui->treeView->PauseTreeViewSaving();
        }
        else*/
        if (m_model->HasFilter())
        {
            UpdateFilter();
        }
    }

    void NodePaletteWidget::ClearSelection()
    {
        m_ui->treeView->selectionModel()->clearSelection();
    }

    const GraphCanvasTreeItem* NodePaletteWidget::GetTreeRoot() const
    {
        return static_cast<GraphCanvasTreeModel*>(m_model->sourceModel())->GetTreeRoot();
    }

    NodePaletteTreeView* NodePaletteWidget::GetTreeView() const
    {
        return m_ui->treeView;
    }

    QLineEdit* NodePaletteWidget::GetSearchFilter() const
    {
        return m_ui->searchFilter;
    }

    bool NodePaletteWidget::eventFilter(QObject* /*object*/, QEvent* qEvent)
    {
        if (qEvent->type() == QEvent::KeyPress)
        {            
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(qEvent);

            if (keyEvent->key() == Qt::Key_Down)
            {
                if (m_model->rowCount() == 0)
                {
                    return true;
                }

                m_searchFieldSelectionChange = true;       

                QModelIndex baseIndex = m_ui->treeView->selectionModel()->currentIndex();
                QModelIndex searchIndex = baseIndex.parent();

                bool exploreNextIndex = true;
                int nextRow = baseIndex.row() + 1;

                // If we don't have a selection, select the first item.
                if (!baseIndex.isValid())
                {
                    searchIndex = QModelIndex();
                    exploreNextIndex = false;
                    nextRow = 0;
                }                

                while (exploreNextIndex)
                {
                    int rowCount = m_model->rowCount(searchIndex);                    

                    if (nextRow >= rowCount)
                    {
                        // If our parent isn't valid this means we reached the top of our list. So we'll want to reset our next row count.
                        if (!searchIndex.isValid())
                        {
                            nextRow = 0;
                            break;
                        }
                        else
                        {
                            nextRow = searchIndex.row() + 1;
                            searchIndex = searchIndex.parent();                            
                            continue;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                QModelIndex childIndex = m_model->index(nextRow, 0, searchIndex);

                bool hasChildren = m_model->hasChildren(childIndex);

                while (hasChildren)
                {
                    childIndex = m_model->index(0, 0, childIndex);
                    hasChildren = m_model->hasChildren(childIndex);
                }

                AZStd::vector< QModelIndex > expandableIndexes;
                QModelIndex expandedIndex = childIndex.parent();

                while (expandedIndex.isValid() && !m_ui->treeView->isExpanded(expandedIndex))
                {
                    expandableIndexes.push_back(expandedIndex);
                    expandedIndex = expandedIndex.parent();
                }

                for (int i = static_cast<int>(expandableIndexes.size()) - 1; i > 0; i--)
                {
                    m_ui->treeView->expand(expandableIndexes[i]);
                }

                m_ui->treeView->selectionModel()->setCurrentIndex(childIndex, QItemSelectionModel::SelectionFlag::ClearAndSelect);

                m_searchFieldSelectionChange = false;

                return true;
            }
            else if (keyEvent->key() == Qt::Key_Up)
            {
                if (m_model->rowCount() == 0)
                {
                    return true;
                }

                m_searchFieldSelectionChange = true;

                QModelIndex baseIndex = m_ui->treeView->selectionModel()->currentIndex();
                QModelIndex searchIndex = baseIndex.parent();
                
                bool exploreNextIndex = true;
                int nextRow = baseIndex.row() - 1;

                // If we don't have a selection, select the first item.
                if (!baseIndex.isValid())
                {
                    searchIndex = QModelIndex();
                    exploreNextIndex = false;
                    nextRow = m_model->rowCount() - 1;
                }

                while (exploreNextIndex)
                {
                    if (nextRow < 0)
                    {
                        // If our parent isn't valid this means we reached the top of our list. So we'll want to reset our next row count.
                        if (!searchIndex.isValid())
                        {
                            nextRow = m_model->rowCount() - 1;
                            break;
                        }
                        else
                        {
                            nextRow = searchIndex.row() - 1;
                            searchIndex = searchIndex.parent();
                            continue;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                QModelIndex childIndex = m_model->index(nextRow, 0, searchIndex);

                bool hasChildren = m_model->hasChildren(childIndex);

                while (hasChildren)
                {
                    int rowCount = m_model->rowCount(childIndex) - 1;
                    childIndex = m_model->index(rowCount, 0, childIndex);
                    hasChildren = m_model->hasChildren(childIndex);
                }

                AZStd::vector< QModelIndex > expandableIndexes;
                QModelIndex expandedIndex = childIndex.parent();

                while (expandedIndex.isValid() && !m_ui->treeView->isExpanded(expandedIndex))
                {
                    expandableIndexes.push_back(expandedIndex);
                    expandedIndex = expandedIndex.parent();
                }

                for (int i = static_cast<int>(expandableIndexes.size()) - 1; i > 0; i--)
                {
                    m_ui->treeView->expand(expandableIndexes[i]);
                }

                m_ui->treeView->selectionModel()->setCurrentIndex(childIndex, QItemSelectionModel::SelectionFlag::ClearAndSelect);
                
                m_searchFieldSelectionChange = false;

                return true;
            }
        }

        return false;
    }

    NodePaletteSortFilterProxyModel* NodePaletteWidget::GetFilterModel()
    {
        return m_model;
    }

    GraphCanvasTreeItem* NodePaletteWidget::FindItemWithName(QString name)
    {
        GraphCanvasTreeItem* foundItem = nullptr;

        GraphCanvas::GraphCanvasTreeItem* item = ModTreeRoot();

        AZStd::unordered_set< GraphCanvas::GraphCanvasTreeItem* > unexploredItems = { item };

        while (!unexploredItems.empty())
        {
            GraphCanvasTreeItem* currentItem = (*unexploredItems.begin());
            unexploredItems.erase(unexploredItems.begin());

            NodePaletteTreeItem* treeItem = azrtti_cast<NodePaletteTreeItem*>(currentItem);

            if (treeItem == nullptr)
            {
                continue;
            }

            if (treeItem->GetName().compare(name, Qt::CaseInsensitive) == 0)
            {
                foundItem = treeItem;
                break;
            }

            for (int i = 0; i < treeItem->GetChildCount(); ++i)
            {
                GraphCanvasTreeItem* childItem = treeItem->FindChildByRow(i);

                if (childItem)
                {
                    unexploredItems.insert(childItem);
                }
            }
        }

        return foundItem;
    }

    GraphCanvasTreeItem* NodePaletteWidget::ModTreeRoot()
    {
        return static_cast<GraphCanvasTreeModel*>(m_model->sourceModel())->ModTreeRoot();
    }

    GraphCanvasTreeItem* NodePaletteWidget::CreatePaletteRoot() const
    {
        return nullptr;
    }

    void NodePaletteWidget::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
    {
        if (selected.indexes().empty())
        {
            emit OnSelectionCleared();
            return;
        }

        auto index = selected.indexes().first();

        if (!index.isValid())
        {
            // Nothing to do.
            return;
        }

        // IMPORTANT: mapToSource() is NECESSARY. Otherwise the internalPointer
        // in the index is relative to the proxy model, NOT the source model.
        QModelIndex sourceModelIndex = m_model->mapToSource(index);

        NodePaletteTreeItem* nodePaletteItem = static_cast<NodePaletteTreeItem*>(sourceModelIndex.internalPointer());

        if (m_searchFieldSelectionChange)
        {
            m_ui->searchFilter->setText(nodePaletteItem->GetName());
            m_ui->searchFilter->selectAll();

            // Cancel the update time just in case there was one queued we don't want to mess with the filtering wihle we are manually scrubbing through the entries.
            m_filterTimer.stop();
        }

        HandleSelectedItem(nodePaletteItem);
    }

    void NodePaletteWidget::OnScrollChanged(int /*scrollPosition*/)
    {
        RefreshFloatingHeader();
    }

    void NodePaletteWidget::RefreshFloatingHeader()
    {
        // TODO: Fix slight visual hiccup with the sizing of the header when labels vanish.
        // Seems to remain at size for one frame.
        QModelIndex proxyIndex = m_ui->treeView->indexAt(QPoint(0, 0));
        QModelIndex modelIndex = m_model->mapToSource(proxyIndex);
        NodePaletteTreeItem* currentItem = static_cast<NodePaletteTreeItem*>(modelIndex.internalPointer());

        QString fullPathString;

        bool needsSeparator = false;

        while (currentItem && currentItem->GetParent() != nullptr)
        {
            currentItem = static_cast<NodePaletteTreeItem*>(currentItem->GetParent());

            // This is the root element which is invisible. We don't want to display that.
            if (currentItem->GetParent() == nullptr)
            {
                break;
            }

            if (needsSeparator)
            {
                fullPathString.prepend("/");
            }

            fullPathString.prepend(currentItem->GetName());
            needsSeparator = true;
        }

        m_ui->m_categoryLabel->setText(fullPathString);
    }

    void NodePaletteWidget::OnFilterTextChanged(const QString &text)
    {
        if(text.isEmpty())
        {
            //If filter was cleared, update immediately
            UpdateFilter();
            return;
        }
        if (!m_searchFieldSelectionChange)
        {
            m_filterTimer.stop();
            m_filterTimer.start();
        }
    }

    void NodePaletteWidget::UpdateFilter()
    {
        if (!m_model->HasFilter())
        {
            m_ui->treeView->UnpauseTreeViewSaving();
            m_ui->treeView->CaptureTreeViewSnapshot();
            m_ui->treeView->PauseTreeViewSaving();
        }

        QString text = m_ui->searchFilter->userInputText();

        m_model->SetFilter(text);
        m_model->invalidate();

        if (!m_model->HasFilter())
        {
            m_ui->treeView->UnpauseTreeViewSaving();
            m_ui->treeView->ApplyTreeViewSnapshot();
            m_ui->treeView->PauseTreeViewSaving();

            m_ui->treeView->clearSelection();
        }
        else
        {
            m_ui->treeView->expandAll();
        }
    }

    void NodePaletteWidget::ClearFilter()
    {
        {
            QSignalBlocker blocker(m_ui->searchFilter);
            m_ui->searchFilter->setText("");
        }

        UpdateFilter();
    }

    void NodePaletteWidget::OnRowsAboutToBeRemoved(const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
    {
        m_ui->treeView->clearSelection();
    }

    void NodePaletteWidget::TrySpawnItem()
    {
        if (m_ui->treeView->selectionModel()->hasSelection())
        {
            QModelIndex sourceIndex = m_model->mapToSource(m_ui->treeView->selectionModel()->currentIndex());

            GraphCanvasTreeItem* treeItem = static_cast<GraphCanvasTreeItem*>(sourceIndex.internalPointer());

            if (treeItem)
            {
                HandleSelectedItem(treeItem);
            }
        }
        else
        {
            QCompleter* completer = m_ui->searchFilter->completer();
            QModelIndex modelIndex = completer->currentIndex();

            if (!m_ui->searchFilter->text().isEmpty() && modelIndex.isValid())
            {
                // The docs say this is fine. So here's hoping.
                QAbstractProxyModel* proxyModel = qobject_cast<QAbstractProxyModel*>(completer->completionModel());

                if (proxyModel)
                {
                    QModelIndex sourceIndex = proxyModel->mapToSource(modelIndex);

                    if (sourceIndex.isValid())
                    {
                        NodePaletteAutoCompleteModel* autoCompleteModel = qobject_cast<NodePaletteAutoCompleteModel*>(proxyModel->sourceModel());

                        const GraphCanvasTreeItem* treeItem = autoCompleteModel->FindItemForIndex(sourceIndex);

                        if (treeItem)
                        {
                            HandleSelectedItem(treeItem);
                        }
                    }
                }
            }
            else
            {
                emit OnCreateSelection();
            }
        }
    }

    void NodePaletteWidget::HandleSelectedItem(const GraphCanvasTreeItem* treeItem)
    {
        if (m_isInContextMenu && !m_searchFieldSelectionChange)
        {
            m_contextMenuCreateEvent.reset(treeItem->CreateMimeEvent());

            if (m_contextMenuCreateEvent)
            {
                emit OnCreateSelection();
            }
        }

        emit OnTreeItemSelected(treeItem);
    }

    void NodePaletteWidget::ResetModel(GraphCanvasTreeItem* rootItem)
    {
        GraphCanvasTreeModelRequestBus::Handler::BusDisconnect();

        GraphCanvasTreeModel* sourceModel = aznew GraphCanvasTreeModel(rootItem ? rootItem : CreatePaletteRoot(), this);
        sourceModel->setMimeType(m_mimeType.c_str());

        delete m_model;
        m_model = aznew NodePaletteSortFilterProxyModel(this);
        m_model->setSourceModel(sourceModel);
        m_model->PopulateUnfilteredModel();

        GraphCanvasTreeModelRequestBus::Handler::BusConnect(sourceModel);

        m_ui->treeView->setModel(m_model);

        ResetDisplay();
    }

#include <StaticLib/GraphCanvas/Widgets/NodePalette/moc_NodePaletteWidget.cpp>
}
