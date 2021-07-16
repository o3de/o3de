/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CategoriesList.h"

ComponentCategoryList::ComponentCategoryList(QWidget* parent /*= nullptr*/) 
    : QTreeWidget(parent)
{
}

void ComponentCategoryList::Init()
{
    setColumnCount(1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setDragDropMode(QAbstractItemView::DragDropMode::DragOnly);
    setDragEnabled(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAllColumnsShowFocus(true);
    setStyleSheet("QTreeWidget { selection-background-color: rgba(255,255,255,0.2); }");

    QStringList headers;
    headers << tr("Categories");
    setHeaderLabels(headers);

    const QString parentCategoryIconPath = QString("Icons/PropertyEditor/Browse_on.png");
    const QString categoryIconPath = QString("Icons/PropertyEditor/Browse.png");

    QTreeWidgetItem* allCategory = new QTreeWidgetItem(this);
    allCategory->setText(0, "All");
    allCategory->setIcon(0, QIcon(categoryIconPath));

    // Need this briefly to collect the list of available categories.
    ComponentDataModel dataModel(this);
    for (const auto& cat : dataModel.GetCategories())
    {
        QString categoryString = QString(cat.c_str());
        QStringList categories = categoryString.split('/', Qt::SkipEmptyParts);

        QTreeWidgetItem* parent = nullptr;
        QTreeWidgetItem* categoryWidget = nullptr;

        for (const auto& categoryName : categories)
        {
            if (parent)
            {
                categoryWidget = new QTreeWidgetItem(parent);
                categoryWidget->setIcon(0, QIcon(categoryIconPath));
                
                // Store the full category path in a user role because we'll need it to locate the actual category
                categoryWidget->setData(0, Qt::UserRole, QVariant::fromValue(categoryString));
            }
            else
            {
                auto existingCategory = findItems(categoryName, Qt::MatchExactly);
                if (existingCategory.empty())
                {
                    categoryWidget = new QTreeWidgetItem(this);
                    categoryWidget->setIcon(0, QIcon(parentCategoryIconPath));
                }
                else
                {
                    categoryWidget = static_cast<QTreeWidgetItem*>(existingCategory.first());
                    categoryWidget->setIcon(0, QIcon(parentCategoryIconPath));
                }
            }

            parent = categoryWidget;

            categoryWidget->setText(0, categoryName);
        }
    }

    expandAll();

    connect(this, &QTreeWidget::itemClicked, this, &ComponentCategoryList::OnItemClicked);
}

void ComponentCategoryList::OnItemClicked(QTreeWidgetItem* item, int /*column*/)
{
    QVariant userData = item->data(0, Qt::UserRole);
    if (userData.isValid())
    {
        // Send in the full category path, not just the child category name
        emit OnCategoryChange(userData.value<QString>().toStdString().c_str());
    }
    else
    {
        emit OnCategoryChange(item->text(0).toStdString().c_str());
    }
}

#include <UI/ComponentPalette/moc_CategoriesList.cpp>
