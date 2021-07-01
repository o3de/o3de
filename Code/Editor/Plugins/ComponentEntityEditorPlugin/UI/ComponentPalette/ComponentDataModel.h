/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>

#include <AzQtComponents/Buses/DragAndDrop.h>
#endif

namespace ComponentDataUtilities
{
    // Given a list of selected components, use the provided model to get the components to add to any selected entities.
    void AddComponentsToSelectedEntities(const QModelIndexList& selectedComponents, QAbstractItemModel* model);
}

class CViewport;

//! ComponentDataModel
//! Holds the data required to display components in a table, this includes component name, categories, icons.
class ComponentDataModel 
    : public QAbstractTableModel
    , protected AzQtComponents::DragAndDropEventsBus::Handler // its okay if more than one of these is installed, the first one gets it.
{
    Q_OBJECT

public:
    AZ_CLASS_ALLOCATOR(ComponentDataModel, AZ::SystemAllocator, 0);

    using ComponentClassList = AZStd::vector<const AZ::SerializeContext::ClassData*>;
    using ComponentCategorySet = AZStd::set<AZStd::string>;
    using ComponentClassMap = AZStd::unordered_map<AZStd::string, AZStd::vector<const AZ::SerializeContext::ClassData*>>;
    using ComponentIconMap = AZStd::unordered_map<AZ::Uuid, QIcon>;

    enum ColumnIndex
    {
        Icon,
        Category,
        Name,
        Count
    };

    enum CustomRoles
    {
        ClassDataRole = Qt::UserRole + 1
    };

    ComponentDataModel(QObject* parent = nullptr);
    ~ComponentDataModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    const AZ::SerializeContext::ClassData* GetClassData(const QModelIndex&) const;

    AZ::EntityId NewEntityFromSelection(const QModelIndexList& selection);

    static const char* GetCategory(const AZ::SerializeContext::ClassData* classData);

    ComponentClassList& GetComponents() { return m_componentList; }
    ComponentCategorySet& GetCategories() { return m_categories; }

protected:
    //////////////////////////////////////////////////////////////////////////
    // AzQtComponents::DragAndDropEventsBus::Handler
    //////////////////////////////////////////////////////////////////////////
    void DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
    void DragMove(QDragMoveEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
    void DragLeave(QDragLeaveEvent* event) override;
    void Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
    bool CanAcceptDragAndDropEvent(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) const;

    ComponentClassList m_componentList;
    ComponentClassMap m_componentMap;
    ComponentIconMap m_componentIcons;
    ComponentCategorySet m_categories;
};

//! ComponentDataProxyModel
//! FilterProxy for the ComponentDataModel is used along with the search criteria to filter the 
//! list of components based on tags and/or selected category.
class ComponentDataProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    AZ_CLASS_ALLOCATOR(ComponentDataProxyModel, AZ::SystemAllocator, 0);

    ComponentDataProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}

    // Creates a new entity and adds the selected components to it.
    // It is specialized here to ensure it uses the correct indices according to the sorted data.
    AZ::EntityId NewEntityFromSelection(const QModelIndexList& selection);

    // Filters rows according to the specifed tags and/or selected category
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    // Set the category to filter by.
    void SetSelectedCategory(const AZStd::string& category);
    void ClearSelectedCategory();

protected:

    AZStd::string m_selectedCategory;
};
