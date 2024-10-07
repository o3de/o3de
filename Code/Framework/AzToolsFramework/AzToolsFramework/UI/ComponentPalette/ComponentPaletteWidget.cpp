/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "ComponentPaletteModel.hxx"
#include "ComponentPaletteUtil.hxx"
#include "ComponentPaletteWidget.hxx"

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Debug/Profiler.h>
#include <AzFramework/Components/DeprecatedComponentsBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <AzQtComponents/Components/Widgets/LineEdit.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QAction>
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QStandardItem>
#include <QStandardItem>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QToolButton>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    ComponentPaletteWidget::ComponentPaletteWidget(QWidget* parent, bool enableSearch)
        : QFrame(parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
        setFrameShadow(QFrame::Shadow::Raised);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setAcceptDrops(false);

        auto outerLayout = new QVBoxLayout(this);
        outerLayout->setSizeConstraint(QLayout::SetNoConstraint);
        setLayout(outerLayout);

        // Search filter
        m_searchFrame = new QFrame(this);
        m_searchFrame->setObjectName("SearchFrame");
        m_searchFrame->setVisible(enableSearch);

        auto searchLayout = new QHBoxLayout(m_searchFrame);
        searchLayout->setSizeConstraint(QLayout::SetMinimumSize);

        m_searchText = new QLineEdit(m_searchFrame);
        m_searchText->setObjectName("SearchText");
        m_searchText->setText("");
        m_searchText->setPlaceholderText("Search...");
        m_searchText->setClearButtonEnabled(true);
        AzQtComponents::LineEdit::applySearchStyle(m_searchText);

        m_searchRegExp = QRegExp("", Qt::CaseInsensitive, QRegExp::RegExp);

        searchLayout->addWidget(m_searchText);
        m_searchFrame->setLayout(searchLayout);
        outerLayout->addWidget(m_searchFrame);

        m_componentModel = new ComponentPaletteModel(this);

        m_componentTree = new QTreeView(this);
        m_componentTree->setObjectName("Tree");
        m_componentTree->setModel(m_componentModel);
        m_componentTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
        outerLayout->addWidget(m_componentTree);

        //hide header for dropdown-style, single-column, tree
        m_componentTree->header()->hide();

        connect(m_searchText, &QLineEdit::textChanged, this, &ComponentPaletteWidget::QueueUpdateSearch);
        QToolButton* clearButton = AzQtComponents::LineEdit::getClearButton(m_searchText);
        assert(clearButton);
        connect(clearButton, &QToolButton::clicked, this, &ComponentPaletteWidget::ClearSearch);

        connect(m_componentTree, &QTreeView::activated, this, &ComponentPaletteWidget::ActivateSelection);
        connect(m_componentTree, &QTreeView::clicked, this, &ComponentPaletteWidget::ActivateSelection);
        connect(m_componentTree, &QTreeView::doubleClicked, this, &ComponentPaletteWidget::ActivateSelection);
        connect(m_componentTree, &QTreeView::expanded, this, &ComponentPaletteWidget::ExpandCategory);
        connect(m_componentTree, &QTreeView::collapsed, this, &ComponentPaletteWidget::CollapseCategory);

        auto actionToHideWindow = new QAction(tr("Hide Window"), this);
        actionToHideWindow->setShortcut(QKeySequence::Cancel);
        actionToHideWindow->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(actionToHideWindow, &QAction::triggered, this, &ComponentPaletteWidget::OnAddComponentCancel);
        connect(actionToHideWindow, &QAction::triggered, this, &ComponentPaletteWidget::hide);
        addAction(actionToHideWindow);

        auto actionToFocusSearchBox = new QAction(tr("Focus Search Box"), this);
        actionToFocusSearchBox->setShortcut(QKeySequence::Find);
        actionToFocusSearchBox->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(actionToFocusSearchBox, &QAction::triggered, this, &ComponentPaletteWidget::FocusSearchBox);
        addAction(actionToFocusSearchBox);

        installEventFilter(this);
    }

    void ComponentPaletteWidget::Populate(
        AZ::SerializeContext* serializeContext,
        const AzToolsFramework::EntityIdList& selectedEntityIds,
        const AzToolsFramework::ComponentFilter& componentFilter,
        AZStd::span<const AZ::ComponentServiceType> serviceFilter,
        AZStd::span<const AZ::ComponentServiceType> incompatibleServiceFilter)
    {
        m_serializeContext = serializeContext;
        m_selectedEntityIds = selectedEntityIds;
        m_componentFilter = componentFilter;
        m_serviceFilter.assign_range(serviceFilter);
        m_incompatibleServiceFilter.assign_range(incompatibleServiceFilter);

        UpdateContent();
        Present();
    }

    void ComponentPaletteWidget::UpdateContent()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        m_componentModel->clear();

        bool applyRegExFilter = !m_searchRegExp.isEmpty();

        // Gather all components that match our filter and group by category.
        ComponentPaletteUtil::ComponentDataTable componentDataTable;
        ComponentPaletteUtil::ComponentIconTable componentIconTable;
        ComponentPaletteUtil::BuildComponentTables(
            m_serializeContext, 
            m_componentFilter, 
            m_serviceFilter, 
            m_incompatibleServiceFilter, 
            componentDataTable, 
            componentIconTable);

        AzFramework::Components::DeprecatedComponentsList deprecatedList;
        AzFramework::Components::DeprecatedComponentsRequestBus::Broadcast(&AzFramework::Components::DeprecatedComponentsRequestBus::Events::EnumerateDeprecatedComponents, deprecatedList);

        AZ::Entity::ComponentArrayType componentsOnEntity;

        // Get all components on all selected entities so we can display a count of used components by type
        AZStd::unordered_set<AZ::Component*> allComponentsOnSelectedEntities;
        for (AZ::EntityId entityId : m_selectedEntityIds)
        {
            componentsOnEntity.clear();
            AzToolsFramework::GetAllComponentsForEntity(entityId, componentsOnEntity);
            allComponentsOnSelectedEntities.insert(componentsOnEntity.begin(), componentsOnEntity.end());
        }

        // Populate the context menu.
        AZStd::map<QString, QStandardItem*> categoryItemMap;

        for (const auto& categoryPair : componentDataTable)
        {
            //get the full category name/path and split it by separators for iteration
            const QString& categoryPath = categoryPair.first;
            const QStringList& categoryPathSegments = categoryPath.split('/', Qt::SkipEmptyParts);
            QString categoryPathBuilder;

            //for every segment of the category path, create an expandable header
            auto parentItem = m_componentModel->invisibleRootItem();
            for (const QString& categoryName : categoryPathSegments)
            {
                categoryPathBuilder += categoryName + "/";

                QStandardItem* categoryItem = nullptr;
                auto categoryItemItr = categoryItemMap.find(categoryPathBuilder);
                if (categoryItemItr == categoryItemMap.end())
                {
                    categoryItem = new QStandardItem(categoryName);
                    categoryItem->setCheckable(false);
                    categoryItem->setEditable(false);
                    categoryItem->setSelectable(true);
                    categoryItem->setData((qulonglong)nullptr, Qt::ItemDataRole::UserRole + 1);

                    //make groups bold
                    QFont font = categoryItem->font();
                    font.setBold(true);
                    categoryItem->setFont(font);

                    parentItem->appendRow(categoryItem);

                    categoryItemMap[categoryPathBuilder] = categoryItem;
                }
                else
                {
                    categoryItem = categoryItemItr->second;
                }

                parentItem = categoryItem;
            }
        }

        for (const auto& [categoryName, componentMap] : componentDataTable)
        {
            auto categoryItemItr = categoryItemMap.find(categoryName + "/");
            auto parentItem = categoryItemItr != categoryItemMap.end() ? categoryItemItr->second : m_componentModel->invisibleRootItem();

            for (const auto& componentPair : componentMap)
            {
                auto componentClass = componentPair.second;
                const QString& componentName = componentPair.first;
                const QString& componentIconName = componentIconTable[componentClass];
                auto deprecatedInfo = deprecatedList.find(componentClass->m_typeId);
                bool componentIsDeprecated = deprecatedInfo != deprecatedList.end();
                if ((!applyRegExFilter || categoryName.contains(m_searchRegExp) || componentName.contains(m_searchRegExp))
                    && (!componentIsDeprecated || !deprecatedInfo->second.m_hideComponent))
                {
                    //count the number of components on selected entities that match this type
                    auto componentCount = AZStd::count_if(allComponentsOnSelectedEntities.begin(), allComponentsOnSelectedEntities.end(), [componentClass](const AZ::Component* component) {
                        return componentClass->m_typeId == component->GetUnderlyingComponentType();
                    });

                    //generate the display name for the component
                    QString displayName = componentName;
                    if (componentCount) //<append count if count > 0
                    {
                        displayName += QObject::tr(" (%1)").arg(componentCount);
                    }
                    if (componentIsDeprecated) //< append deprecation strings
                    {
                        displayName += deprecatedInfo->second.m_deprecationString.c_str();
                    }

                    auto componentItem = new QStandardItem(QIcon(componentIconName), displayName);
                    componentItem->setToolTip(componentClass->m_editData->m_description);
                    componentItem->setCheckable(false);
                    componentItem->setEditable(false);
                    componentItem->setSelectable(true);
                    componentItem->setData((qulonglong)componentClass, Qt::ItemDataRole::UserRole + 1);
                    parentItem->appendRow(componentItem);
                }
            }
        }

        // If we have removed component items from the visible tree we need to prune the hanging titles.
        // We also need to auto expand all the items that contain children.
        for (int i = m_componentModel->rowCount() - 1; i >= 0; --i)
        {
            if (BranchHasNoChildren(m_componentModel->item(i)))
            {
                m_componentModel->removeRow(i);
            }
            else
            {
                SetExpanded(m_componentModel->index(i, 0));
            }
        }
        m_componentModel->sort(0);
    }

    bool ComponentPaletteWidget::BranchHasNoChildren(QStandardItem* item)
    {
        bool returnVal = true;

        // We have to check all items to completely remove all non-parent items. Hence no early-exit.
        for (int i = item->rowCount() - 1; i >= 0 ; --i)
        {
            QStandardItem* it = item->child(i);
            // Check in reverse so removing a row doesn't affect remaining items.
            if (it->data(Qt::ItemDataRole::UserRole + 1).toULongLong())
            {
                // if this node has data then the parent has children.
                returnVal = false;
            }
            else
            {
                // If this item has no children, prune it.
                if (BranchHasNoChildren(it))
                {
                    item->removeRow(i);
                }
                else
                {
                    SetExpanded(it->index());
                    returnVal = false;
                }
            }
        }
        return returnVal;
    }

    void ComponentPaletteWidget::SetExpanded(QModelIndex itemIndex)
    {
        auto label = m_componentModel->data(itemIndex, Qt::ItemDataRole::DisplayRole).toString();
        auto stateItr = m_categoryExpandedState.find(label);
        auto expand = stateItr == m_categoryExpandedState.end() || stateItr->second;
        m_componentTree->setExpanded(itemIndex, expand);
    }


    void ComponentPaletteWidget::Present()
    {
        layout()->setEnabled(true);
        layout()->update();
        layout()->activate();

        raise();
        show();

        FocusSearchBox();
    }

    void ComponentPaletteWidget::QueueUpdateSearch()
    {
        QTimer::singleShot(1, this, &ComponentPaletteWidget::UpdateSearch);
    }

    void ComponentPaletteWidget::UpdateSearch()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        m_searchRegExp = QRegExp(m_searchText->text(), Qt::CaseInsensitive, QRegExp::RegExp);
        m_searchText->setFocus();
        UpdateContent();
    }

    void ComponentPaletteWidget::ClearSearch()
    {
        m_searchText->setText("");
        QueueUpdateSearch();
    }

    void ComponentPaletteWidget::ActivateSelection(const QModelIndex& index)
    {
        if (index.isValid())
        {
            auto componentClass = reinterpret_cast<const AZ::SerializeContext::ClassData*>(m_componentModel->data(index, Qt::ItemDataRole::UserRole + 1).toULongLong());
            if (componentClass)
            {
                emit OnAddComponentBegin();

                EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::AddComponentsToEntities, m_selectedEntityIds, AZ::ComponentTypeList{ componentClass->m_typeId });

                emit OnAddComponentEnd();

                ClearSearch();
                hide();
            }
        }
    }

    void ComponentPaletteWidget::ExpandCategory(const QModelIndex& index)
    {
        if (index.isValid())
        {
            auto label = m_componentModel->data(index, Qt::ItemDataRole::DisplayRole).toString();
            m_categoryExpandedState[label] = true;
        }
    }

    void ComponentPaletteWidget::CollapseCategory(const QModelIndex& index)
    {
        if (index.isValid())
        {
            auto label = m_componentModel->data(index, Qt::ItemDataRole::DisplayRole).toString();
            m_categoryExpandedState[label] = false;
        }
    }

    void ComponentPaletteWidget::focusOutEvent(QFocusEvent *event)
    {
        hide();
        QFrame::focusOutEvent(event);
    }

    void ComponentPaletteWidget::FocusSearchBox()
    {
        if (m_searchFrame->isVisible())
        {
            if (!m_searchText->hasFocus())
            {
                m_searchText->setFocus();
            }
        }
        else
        {
            FocusComponentTree();
        }
    }

    void ComponentPaletteWidget::FocusComponentTree()
    {
        if (!m_componentTree->hasFocus())
        {
            m_componentTree->setFocus();
            // Focus the first actual component (leaf node)
            QModelIndex indexToSelect = m_componentModel->index(0, 0);
            while (indexToSelect.isValid() && m_componentModel->rowCount(indexToSelect) > 0)
            {
                indexToSelect = indexToSelect.model()->index(0, 0, indexToSelect);
            }
            m_componentTree->setCurrentIndex(indexToSelect);
        }
    }

    //overridden to intercept key events to move between filter and tree
    bool ComponentPaletteWidget::eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() != QEvent::KeyPress)
        {
            return false;
        }

        if (object != this &&
            object != m_searchText &&
            object != m_componentTree)
        {
            return false;
        }

        if (!hasFocus() &&
            !m_searchText->hasFocus() &&
            !m_componentTree->hasFocus())
        {
            return false;
        }

        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Down ||
            keyEvent->key() == Qt::Key_Return ||
            keyEvent->key() == Qt::Key_Enter)
        {
            FocusComponentTree();
            return false;
        }
        if (keyEvent->key() == Qt::Key_Up)
        {
            if (!m_componentTree->hasFocus() ||
                m_componentTree->selectionModel()->selectedIndexes().empty() ||
                m_componentTree->selectionModel()->selectedIndexes().front().row() == 0)
            {
                FocusSearchBox();
                return false;
            }
        }
        return false;
    }
}

#include "UI/ComponentPalette/moc_ComponentPaletteWidget.cpp"
