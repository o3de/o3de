/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "FindEntityWidget.h"
#include "FindEntityItemModel.h"
#include "FindEntitySortFilterProxyModel.h"

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteUtil.hxx>
#include <LyShine/UiBase.h>
#include <LyShine/Bus/UiCanvasBus.h>

#include <QPushButton>
#include <QDialogButtonBox>
#include <QTreeView>
#include <QBoxLayout>
#include <QScrollArea>

Q_DECLARE_METATYPE(AZ::Uuid);

namespace
{
    bool AppearsInUiComponentMenu(const AZ::SerializeContext::ClassData& classData)
    {
        return AzToolsFramework::AppearsInAddComponentMenu(classData, AZ_CRC_CE("UI"));
    }
}

FindEntityWidget::FindEntityWidget(AZ::EntityId canvasEntityId, QWidget* pParent, Qt::WindowFlags flags)
    : QWidget(pParent, flags)
{
    SetupUI();

    m_objectTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_objectTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_objectTree->setAutoScrollMargin(20);
    m_objectTree->setUniformRowHeights(true);
    m_objectTree->setHeaderHidden(true);

    m_listModel = aznew FindEntityItemModel(this);
    m_proxyModel = aznew FindEntitySortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_listModel);
    m_objectTree->setModel(m_proxyModel);

    AZ::SerializeContext* serializeContext = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

    if (serializeContext)
    {
        // Make a list of all components that are currently used in the canvas
        AZStd::unordered_set<AZ::Uuid> usedComponents;
        GetUsedComponents(canvasEntityId, usedComponents);

        AzToolsFramework::ComponentPaletteUtil::ComponentDataTable componentDataTable;
        AzToolsFramework::ComponentPaletteUtil::ComponentIconTable componentIconTable;
        AZ::ComponentDescriptor::DependencyArrayType serviceFilter;

        AzToolsFramework::ComponentPaletteUtil::BuildComponentTables(serializeContext, AppearsInUiComponentMenu, serviceFilter, componentDataTable, componentIconTable);

        for (const auto& categoryPair : componentDataTable)
        {
            const auto& componentMap = categoryPair.second;
            for (const auto& componentPair : componentMap)
            {
                auto iter = usedComponents.find(componentPair.second->m_typeId);
                if (iter != usedComponents.end())
                {
                    usedComponents.erase(iter);
                    m_searchWidget->AddTypeFilter(categoryPair.first, componentPair.first, QVariant::fromValue<AZ::Uuid>(componentPair.second->m_typeId));
                }
            }
        }
    }

    connect(m_objectTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FindEntityWidget::OnSelectionChanged);

    connect(m_objectTree, &QTreeView::doubleClicked, this, &FindEntityWidget::OnItemDblClick);

    connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &FindEntityWidget::OnSearchTextChanged);
    connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this, &FindEntityWidget::OnFilterChanged);

    connect(m_selectButton, &QPushButton::clicked, this, &FindEntityWidget::OnSelectClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &FindEntityWidget::OnCancelClicked);

    m_listModel->Initialize(canvasEntityId);
    m_objectTree->expandAll();

    // Select button starts off disabled and becomes enabled when there is a selection
    m_selectButton->setEnabled(false);
}

FindEntityWidget::~FindEntityWidget()
{
    delete m_listModel;
    delete m_proxyModel;
}

AZ::EntityId FindEntityWidget::GetEntityIdFromIndex(const QModelIndex& index) const
{
    if (index.isValid())
    {
        const QModelIndex modelIndex = m_proxyModel->mapToSource(index);
        if (modelIndex.isValid())
        {
            return m_listModel->GetEntityFromIndex(modelIndex);
        }
    }

    return AZ::EntityId();
}

QModelIndex FindEntityWidget::GetIndexFromEntityId(const AZ::EntityId& entityId) const
{
    if (entityId.IsValid())
    {
        QModelIndex modelIndex = m_listModel->GetIndexFromEntity(entityId);
        if (modelIndex.isValid())
        {
            return m_proxyModel->mapFromSource(modelIndex);
        }
    }

    return QModelIndex();
}

void FindEntityWidget::SetupUI()
{
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    setSizePolicy(sizePolicy);
    QVBoxLayout* verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSizeConstraint(QLayout::SetMinimumSize);
    QHBoxLayout* horizontalLayoutSearch = new QHBoxLayout();
    horizontalLayoutSearch->setSpacing(0);
    m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);

    horizontalLayoutSearch->addWidget(m_searchWidget);

    verticalLayout->addLayout(horizontalLayoutSearch);

    QScrollArea* objectList = new QScrollArea(this);
    objectList->setFocusPolicy(Qt::ClickFocus);
    objectList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    objectList->setWidgetResizable(true);
    QWidget* objectListContents = new QWidget();
    QVBoxLayout* verticalLayoutObjectListContents = new QVBoxLayout(objectListContents);
    verticalLayoutObjectListContents->setSpacing(0);
    verticalLayoutObjectListContents->setContentsMargins(0, 0, 0, 0);
    m_objectTree = new QTreeView(objectListContents);

    verticalLayoutObjectListContents->addWidget(m_objectTree);

    objectList->setWidget(objectListContents);

    verticalLayout->addWidget(objectList);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    m_selectButton = new QPushButton(tr("Select in Hierarchy"));
    m_selectButton->setToolTip(tr("Select the selected elements in the Hierarchy."));
    m_selectButton->setDefault(true);
    m_selectButton->setAutoDefault(true);
    m_selectButton->setProperty("class", "Primary");
    m_cancelButton = new QPushButton(tr("Cancel"));
    m_cancelButton->setDefault(false);
    m_cancelButton->setAutoDefault(false);
    buttonBox->addButton(m_selectButton, QDialogButtonBox::ApplyRole);
    buttonBox->addButton(m_cancelButton, QDialogButtonBox::RejectRole);

    verticalLayout->addWidget(buttonBox, 1);
}

void FindEntityWidget::GetUsedComponents(AZ::EntityId canvasEntityId, AZStd::unordered_set<AZ::Uuid>& usedComponents)
{
    LyShine::EntityArray entities;
    UiCanvasBus::Event(
        canvasEntityId,
        &UiCanvasBus::Events::FindElements,
        []([[maybe_unused]] const AZ::Entity* entity)
        {
            return true;
        },
        entities);

    for (AZ::Entity* entity : entities)
    {
        for (AZ::Component* component : entity->GetComponents())
        {
            const AZ::Uuid& componentType = azrtti_typeid(component);
            usedComponents.insert(componentType);
        }
    }
}

void FindEntityWidget::OnSelectionChanged(const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
{
    // Update select button state
    m_selectButton->setEnabled(!selected.isEmpty());
}

void FindEntityWidget::OnItemDblClick([[maybe_unused]] const QModelIndex& index)
{
    OnSelectClicked();
}

void FindEntityWidget::OnSearchTextChanged(const QString& activeTextFilter)
{
    AZStd::string filterString = activeTextFilter.toUtf8().data();

    m_listModel->SearchStringChanged(filterString);
    m_proxyModel->UpdateFilter();

    m_objectTree->expandAll();
}

void FindEntityWidget::OnFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
{
    AZStd::vector<AZ::Uuid> componentFilters;
    componentFilters.reserve(activeTypeFilters.count());

    for (auto filter : activeTypeFilters)
    {
        AZ::Uuid typeId = qvariant_cast<AZ::Uuid>(filter.metadata);
        componentFilters.push_back(typeId);
    }

    m_listModel->SearchFilterChanged(componentFilters);
    m_proxyModel->UpdateFilter();

    m_objectTree->expandAll();
}

void FindEntityWidget::OnSelectClicked()
{
    AZStd::vector<AZ::EntityId> selectedEntities;

    QModelIndexList selectedIndexes = m_objectTree->selectionModel()->selectedIndexes();
    for (const QModelIndex& index : selectedIndexes)
    {
        const AZ::EntityId entityId = GetEntityIdFromIndex(index);
        if (entityId.IsValid())
        {
            selectedEntities.push_back(entityId);
        }
    }

    emit OnFinished(selectedEntities);
}

void FindEntityWidget::OnCancelClicked()
{
    emit OnCanceled();
}

#include <moc_FindEntityWidget.cpp>
