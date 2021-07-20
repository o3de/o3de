/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentDataModel.h"
#include "FavoriteComponentList.h"
#include "FilteredComponentList.h"

#include "CryCommon/MathConversion.h"
#include "Editor/IEditor.h"
#include "Editor/ViewManager.h"
#include <Editor/CryEditDoc.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QHeaderView>

void FilteredComponentList::Init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setDragDropMode(QAbstractItemView::DragDropMode::DragOnly);
    setDragEnabled(true);

    setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    setStyleSheet("QTreeWidget { selection-background-color: rgba(255,255,255,0.2); }");
    setGridStyle(Qt::PenStyle::NoPen);
    verticalHeader()->hide();
    horizontalHeader()->hide();
    setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    setAcceptDrops(false);

    m_componentDataModel = new ComponentDataModel(this);
    ComponentDataProxyModel* componentDataProxyModel = new ComponentDataProxyModel(this);
    componentDataProxyModel->setSourceModel(m_componentDataModel);
    setModel(componentDataProxyModel);

    QHeaderView* horizontalHeaderView = horizontalHeader();
    horizontalHeaderView->setSectionResizeMode(ComponentDataModel::ColumnIndex::Icon, QHeaderView::ResizeToContents);
    horizontalHeaderView->setSectionResizeMode(ComponentDataModel::ColumnIndex::Name, QHeaderView::Stretch);

    setColumnWidth(ComponentDataModel::ColumnIndex::Icon, 32);
    setShowGrid(false);

    setColumnWidth(ComponentDataModel::ColumnIndex::Name, 90);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    sortByColumn(ComponentDataModel::ColumnIndex::Name, Qt::AscendingOrder);
    hideColumn(ComponentDataModel::ColumnIndex::Category);

    connect(model(), &QAbstractItemModel::rowsInserted, this, &FilteredComponentList::rowsInserted);
    connect(model(), &QAbstractItemModel::rowsRemoved, this, &FilteredComponentList::rowsAboutToBeRemoved);

    connect(model(), SIGNAL(modelReset()), SLOT(modelReset()));


    // Context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &FilteredComponentList::ShowContextMenu);
}

void FilteredComponentList::ContextMenu_NewEntity()
{
    AZ::EntityId entityId;

    auto proxyDataModel = qobject_cast<ComponentDataProxyModel*>(model());
    if (proxyDataModel)
    {
        entityId = proxyDataModel->NewEntityFromSelection(selectedIndexes());
    }
    else
    {
        auto dataModel = qobject_cast<ComponentDataModel*>(model());
        if (dataModel)
        {
            entityId = dataModel->NewEntityFromSelection(selectedIndexes());
        }
    }
}


void FilteredComponentList::ContextMenu_AddToFavorites()
{
    AZStd::vector<const AZ::SerializeContext::ClassData*> componentsToAdd;
    for (auto index : selectedIndexes())
    {
        QVariant classDataVariant = index.data(ComponentDataModel::ClassDataRole);
        if (classDataVariant.isValid())
        {
            auto classData = reinterpret_cast<const AZ::SerializeContext::ClassData*>(classDataVariant.value<void*>());
            componentsToAdd.push_back(classData);
        }
    }

    if (!componentsToAdd.empty())
    {
        EBUS_EVENT(FavoriteComponentListRequestBus, AddFavorites, componentsToAdd);
    }
}

void FilteredComponentList::ContextMenu_AddToSelectedEntities()
{
    ComponentDataUtilities::AddComponentsToSelectedEntities(selectedIndexes(), model());
}

void FilteredComponentList::ShowContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Context menu"), this);

    QAction actionNewEntity(tr("Create new entity with selected components"), this);
    if (GetIEditor()->GetDocument()->IsDocumentReady())
    {
        QObject::connect(&actionNewEntity, &QAction::triggered, this, [this] { ContextMenu_NewEntity();  });
        contextMenu.addAction(&actionNewEntity);
    }

    QAction actionAddFavorite(tr("Add to favorites"), this);
    QObject::connect(&actionAddFavorite, &QAction::triggered, this, [this] { ContextMenu_AddToFavorites(); });
    contextMenu.addAction(&actionAddFavorite);

    QAction actionAddToSelection(this);
    if (GetIEditor()->GetDocument()->IsDocumentReady())
    {
        AzToolsFramework::EntityIdList selectedEntities;
        EBUS_EVENT_RESULT(selectedEntities, AzToolsFramework::ToolsApplicationRequests::Bus, GetSelectedEntities);

        if (!selectedEntities.empty())
        {
            QString addToSelection = selectedEntities.size() > 1 ? tr("Add to selected entities") : tr("Add to selected entity");

            actionAddToSelection.setText(addToSelection);
            QObject::connect(&actionAddToSelection, &QAction::triggered, this, [this] { ContextMenu_AddToSelectedEntities(); });
            contextMenu.addAction(&actionAddToSelection);
        }
    }
    // TODO: Requires information panel implementation LMBR-28174
    //QAction actionHelp(tr("Help"), this);
    //QObject::connect(&actionHelp, &QAction::triggered, this, [&] {});
    //contextMenu.addAction(&actionHelp);

    contextMenu.exec(mapToGlobal(pos));
}

void FilteredComponentList::modelReset()
{
    // Ensure that the category column is hidden
    hideColumn(ComponentDataModel::ColumnIndex::Category);
}

FilteredComponentList::FilteredComponentList(QWidget* parent /*= nullptr*/)
    : QTableView(parent)
{
}

FilteredComponentList::~FilteredComponentList()
{
}

void FilteredComponentList::SearchCriteriaChanged(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator)
{
    setUpdatesEnabled(false);

    auto dataModel = qobject_cast<ComponentDataProxyModel*>(model());
    if (dataModel)
    {
        // Go through the list of items and show/hide as needed due to filter.
        QString filter;
        for (const auto& criteria : criteriaList)
        {
            QString tag, text;
            AzToolsFramework::SearchCriteriaButton::SplitTagAndText(criteria, tag, text);
            AppendFilter(filter, text, filterOperator);
        }

        dataModel->setFilterRegExp(QRegExp(filter, Qt::CaseSensitivity::CaseInsensitive));
    }

    setUpdatesEnabled(true);
}

void FilteredComponentList::SetCategory(const char* category)
{
    auto dataModel = qobject_cast<ComponentDataProxyModel*>(model());
    if (dataModel)
    {
        if (!category || category[0] == 0 || azstricmp(category, "All") == 0)
        {
            dataModel->ClearSelectedCategory();
        }
        else
        {
            dataModel->SetSelectedCategory(category);
        }
    }

    // Note: this ensures the category column remains hidden
    hideColumn(ComponentDataModel::ColumnIndex::Category);
}

void FilteredComponentList::BuildFilter(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator)
{
    ClearFilterRegExp();

    for (const auto& criteria : criteriaList)
    {
        QString tag, text;
        AzToolsFramework::SearchCriteriaButton::SplitTagAndText(criteria, tag, text);
        if (tag.isEmpty())
        {
            tag = "null";
        }

        QString filter = m_filtersRegExp[tag.toStdString().c_str()].pattern();

        AppendFilter(filter, text, filterOperator);

        SetFilterRegExp(tag.toStdString().c_str(), QRegExp(filter, Qt::CaseInsensitive));
    }
}

void FilteredComponentList::AppendFilter(QString& filter, const QString& text, AzToolsFramework::FilterOperatorType filterOperator)
{
    if (filterOperator == AzToolsFramework::FilterOperatorType::Or)
    {
        if (filter.isEmpty())
        {
            filter = text;
        }
        else
        {
            filter += "|" + text;
        }
    }
    else if (filterOperator == AzToolsFramework::FilterOperatorType::And)
    {
        //using lookaheads to produce an "and" effect.
        filter += "(?=.*" + text + ")";
    }
}

void FilteredComponentList::SetFilterRegExp(const AZStd::string& filterType, const QRegExp& regExp)
{
    m_filtersRegExp[filterType] = regExp;
}

void FilteredComponentList::ClearFilterRegExp(const AZStd::string& filterType /*= AZStd::string()*/)
{
    if (filterType.empty())
    {
        for (auto& it : m_filtersRegExp)
        {
            it.second = QRegExp();
        }
    }
    else
    {
        m_filtersRegExp[filterType] = QRegExp();
    }
}

#include <UI/ComponentPalette/moc_FilteredComponentList.cpp>
