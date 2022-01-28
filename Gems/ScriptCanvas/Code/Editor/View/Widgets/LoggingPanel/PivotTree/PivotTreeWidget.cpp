/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>

#include <ScriptCanvas/Bus/RequestBus.h>

#include <Editor/View/Widgets/LoggingPanel/PivotTree/PivotTreeWidget.h>
// Disable warnings in moc code
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <Editor/View/Widgets/LoggingPanel/PivotTree/ui_PivotTreeWidget.h>
AZ_POP_DISABLE_WARNING

namespace ScriptCanvasEditor
{
    //////////////////
    // PivotTreeItem
    //////////////////

    PivotTreeItem::PivotTreeItem()
        : m_isPivotElement(false)
    {
    }

    PivotTreeItem::~PivotTreeItem()
    {
    }

    const LoggingDataId& PivotTreeItem::GetLoggingDataId() const
    {
        return m_loggingDataId;
    }

    int PivotTreeItem::GetColumnCount() const
    {
        return Column::Count;
    }

    Qt::ItemFlags PivotTreeItem::Flags([[maybe_unused]] const QModelIndex& index) const
    {
        Qt::ItemFlags flags = Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable;

        if (m_isPivotElement)
        {
            flags |= Qt::ItemFlag::ItemIsAutoTristate;
        }

        return flags;
    }

    QVariant PivotTreeItem::Data(const QModelIndex& index, int role) const
    {
        switch (index.column())
        {
        case Column::Name:
        {
            if (role == Qt::DisplayRole
                || role == Qt::ToolTip)
            {
                return QString(GetDisplayName().c_str());
            }
            else if (role == Qt::CheckStateRole)
            {
                return GetCheckState();
            }
        }
        break;
        default:
            break;
        }

        return QVariant();
    }

    bool PivotTreeItem::SetData(const QModelIndex& index, const QVariant& value, int role)
    {
        switch (index.column())
        {
        case Column::Name:
        {
            if (role == Qt::CheckStateRole)
            {
                Qt::CheckState checkState = value.value<Qt::CheckState>();

                // Never want to let the user interaction set it to 
                if (checkState == Qt::CheckState::PartiallyChecked)
                {
                    if (GetCheckState() == Qt::CheckState::Unchecked)
                    {
                        checkState = Qt::CheckState::Checked;
                    }
                    else
                    {
                        checkState = Qt::CheckState::Unchecked;
                    }
                }

                SetCheckState(checkState);
            }
        }
            break;
        default:
            break;
        }

        return false;
    }

    void PivotTreeItem::OnChildAdded(GraphCanvasTreeItem* treeItem)
    {
        if (m_loggingDataId.IsValid())
        {
            static_cast<PivotTreeItem*>(treeItem)->SetLoggingDataId(m_loggingDataId);
        }
    }

    void PivotTreeItem::OnLoggingDataIdSet()
    {
        
    }

    void PivotTreeItem::SetLoggingDataId(const LoggingDataId& dataId)
    {
        if (dataId != m_loggingDataId)
        {
            m_loggingDataId = dataId;

            for (int i = 0; i < GetChildCount(); ++i)
            {
                PivotTreeItem* treeItem = static_cast<PivotTreeItem*>(FindChildByRow(i));
                treeItem->SetLoggingDataId(dataId);
            }

            OnLoggingDataIdSet();
        }
    }

    void PivotTreeItem::SetIsPivotedElement(bool isPivotElement)
    {
        m_isPivotElement = isPivotElement;
    }

    ////////////////////////
    // PivotTreeEntityItem
    ////////////////////////

    PivotTreeEntityItem::PivotTreeEntityItem(const AZ::NamedEntityId& namedEntityId)
        : m_namedEntityId(namedEntityId)
    {
    }

    const AZ::NamedEntityId& PivotTreeEntityItem::GetNamedEntityId() const
    {
        return m_namedEntityId;
    }

    AZStd::string PivotTreeEntityItem::GetDisplayName() const
    {
        return m_namedEntityId.ToString();
    }

    const AZ::EntityId& PivotTreeEntityItem::GetEntityId() const
    {
        return m_namedEntityId;
    }

    ///////////////////////
    // PivotTreeGraphItem
    ///////////////////////

    PivotTreeGraphItem::PivotTreeGraphItem(const AZ::Data::AssetId& assetId)
        : m_assetId(assetId)
    {
        // Determine the file name for our asset
        AZStd::string fullPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(fullPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_assetId);

        AZStd::size_t indexOf = fullPath.find_last_of('/');

        if (indexOf == AZStd::string::npos)
        {
            m_assetName = fullPath;
            m_assetPath = "";
        }
        else
        {
            m_assetPath = fullPath.substr(0, indexOf);
            m_assetName = fullPath.substr(indexOf + 1);
        }
    }

    AZStd::string PivotTreeGraphItem::GetDisplayName() const
    {
        return m_assetName;
    }

    const AZ::Data::AssetId& PivotTreeGraphItem::GetAssetId() const
    {
        return m_assetId;
    }

    AZStd::string_view PivotTreeGraphItem::GetAssetPath() const
    {
        return m_assetPath;
    }

    //////////////////
    // PivotTreeRoot
    //////////////////

    void PivotTreeRoot::SwitchDataSource(const LoggingDataId& aggregateDataSource)
    {
        SetLoggingDataId(aggregateDataSource);

        OnDataSourceChanged(aggregateDataSource);
    }

    Qt::CheckState PivotTreeRoot::GetCheckState() const
    {
        return Qt::CheckState::Unchecked;
    }

    void PivotTreeRoot::SetCheckState(Qt::CheckState checkState)
    {
        AZ_UNUSED(checkState);
    }

    AZStd::string PivotTreeRoot::GetDisplayName() const
    {
        return "";
    }

    ////////////////////////////
    // PivotTreeSortProxyModel
    ////////////////////////////

    bool PivotTreeSortProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        if (m_filter.isEmpty())
        {
            return true;
        }

        QAbstractItemModel* model = sourceModel();
        QModelIndex index = model->index(sourceRow, PivotTreeItem::Column::Name, sourceParent);

        QString test = model->data(index, Qt::DisplayRole).toString();

        bool showRow = test.lastIndexOf(m_filterRegex) >= 0;

        // Handle showing ourselves if a child is being displayed
        if (!showRow && sourceModel()->hasChildren(index))
        {
            for (int i = 0; i < sourceModel()->rowCount(index); ++i)
            {
                if (filterAcceptsRow(i, index))
                {
                    showRow = true;
                    break;
                }
            }
        }

        // We also want to display ourselves if any of our parents match the filter
        QModelIndex parentIndex = sourceModel()->parent(index);
        while (!showRow && parentIndex.isValid())
        {
            QString test2 = model->data(parentIndex).toString();
            showRow = test2.contains(m_filterRegex);

            parentIndex = sourceModel()->parent(parentIndex);
        }

        return showRow;
    }

    bool PivotTreeSortProxyModel::HasFilter() const
    {
        return !m_filter.isEmpty();
    }

    void PivotTreeSortProxyModel::SetFilter(const QString& filter)
    {
        m_filter = filter;
        m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);

        invalidateFilter();
    }

    void PivotTreeSortProxyModel::ClearFilter()
    {
        if (HasFilter())
        {
            SetFilter("");
        }
    }

    ////////////////////
    // PivotTreeWidget
    ////////////////////

    PivotTreeWidget::PivotTreeWidget(PivotTreeRoot* pivotRoot, const AZ::Crc32& savingId, QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::PivotTreeWidget())
    {
        m_ui->setupUi(this);

        m_pivotRoot = pivotRoot;

        m_treeModel = aznew GraphCanvas::GraphCanvasTreeModel(pivotRoot);

        m_proxyModel = aznew PivotTreeSortProxyModel();
        m_proxyModel->ClearFilter();

        m_proxyModel->setSourceModel(m_treeModel);
        m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

        m_ui->pivotTreeView->setModel(m_proxyModel);
        m_ui->pivotTreeView->sortByColumn(PivotTreeItem::Column::Name, Qt::SortOrder::AscendingOrder);

        m_ui->pivotTreeView->header()->setHidden(true);

        m_ui->pivotTreeView->header()->setStretchLastSection(false);
        m_ui->pivotTreeView->header()->setSectionResizeMode(PivotTreeItem::Column::Name, QHeaderView::ResizeMode::Stretch);

        m_ui->pivotTreeView->header()->setSectionResizeMode(PivotTreeItem::Column::QT_NEEDS_A_SECOND_COLUMN_FOR_THIS_MODEL_TO_WORK_FOR_SOME_REASON, QHeaderView::ResizeMode::Fixed);
        m_ui->pivotTreeView->header()->resizeSection(PivotTreeItem::Column::QT_NEEDS_A_SECOND_COLUMN_FOR_THIS_MODEL_TO_WORK_FOR_SOME_REASON, 1);

        QObject::connect(m_ui->filterWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &PivotTreeWidget::OnFilterChanged);

        m_ui->filterWidget->SetFilterInputInterval(AZStd::chrono::milliseconds(250));

        m_ui->pivotTreeView->InitializeTreeViewSaving(savingId);
        m_ui->pivotTreeView->PauseTreeViewSaving();

        QObject::connect(m_ui->pivotTreeView, &QTreeView::doubleClicked, this, &PivotTreeWidget::OnItemDoubleClicked);
    }

    PivotTreeWidget::~PivotTreeWidget()
    {
    }

    void PivotTreeWidget::DisplayTree()
    {
        OnTreeDisplayed();
    }

    void PivotTreeWidget::SwitchDataSource(const LoggingDataId& aggregateDataSource)
    {
        {
            QSignalBlocker signalBlocker(m_ui->filterWidget);
            m_ui->filterWidget->ClearTextFilter();
            OnFilterChanged("");
        }

        m_pivotRoot->SwitchDataSource(aggregateDataSource);
    }

    void PivotTreeWidget::OnFilterChanged(const QString& activeTextFilter)
    {
        bool hadFilter = m_proxyModel->HasFilter();

        if (!m_proxyModel->HasFilter() && !activeTextFilter.isEmpty())
        {
            m_ui->pivotTreeView->UnpauseTreeViewSaving();
            m_ui->pivotTreeView->CaptureTreeViewSnapshot();
            m_ui->pivotTreeView->PauseTreeViewSaving();
        }

        m_proxyModel->SetFilter(activeTextFilter);

        if (hadFilter && !m_proxyModel->HasFilter())
        {
            m_ui->pivotTreeView->UnpauseTreeViewSaving();
            m_ui->pivotTreeView->ApplyTreeViewSnapshot();
            m_ui->pivotTreeView->PauseTreeViewSaving();
        }
        else if (m_proxyModel->HasFilter())
        {
            m_ui->pivotTreeView->expandAll();
        }
    }

    PivotTreeRoot* PivotTreeWidget::GetTreeRoot()
    {
        return m_pivotRoot;
    }

    void PivotTreeWidget::OnTreeDisplayed()
    {
    }

    void PivotTreeWidget::OnItemDoubleClicked(const QModelIndex& modelIndex)
    {        
        QModelIndex sourceIndex = modelIndex;

        QSortFilterProxyModel* proxyModel = qobject_cast<QSortFilterProxyModel*>(m_ui->pivotTreeView->model());

        if (proxyModel)
        {
            sourceIndex = proxyModel->mapToSource(modelIndex);
        }

        if (PivotTreeItem* pivotTreeItem = static_cast<PivotTreeItem*>(sourceIndex.internalPointer()))
        {
            if (PivotTreeGraphItem* graphItem = azrtti_cast<PivotTreeGraphItem*>(pivotTreeItem))
            {
                GeneralRequestBus::Broadcast
                    ( &GeneralRequests::OpenScriptCanvasAssetId
                    , SourceHandle(nullptr, graphItem->GetAssetId().m_guid, "")
                    , Tracker::ScriptCanvasFileState::UNMODIFIED);
            }
        }
    }

#include <Editor/View/Widgets/LoggingPanel/PivotTree/moc_PivotTreeWidget.cpp>
}
