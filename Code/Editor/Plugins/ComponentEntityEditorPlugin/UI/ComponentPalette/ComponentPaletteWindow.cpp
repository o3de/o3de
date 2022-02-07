/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentPaletteWindow.h"
#include "ComponentDataModel.h"
#include "FavoriteComponentList.h"
#include "FilteredComponentList.h"
#include "CategoriesList.h"

#include <LyViewPaneNames.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzToolsFramework/UI/SearchWidget/SearchCriteriaWidget.hxx>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>

#include <QLabel>
#include <QKeyEvent>

ComponentPaletteWindow::ComponentPaletteWindow(QWidget* parent)
    : QMainWindow(parent)
{
    Init();
}

void ComponentPaletteWindow::Init()
{
    layout()->setSizeConstraint(QLayout::SetMinimumSize);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QHBoxLayout* gridLayout = new QHBoxLayout(nullptr);
    gridLayout->setSizeConstraint(QLayout::SetMaximumSize);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(0);

    m_filterWidget = new AzToolsFramework::SearchCriteriaWidget(this);
    
    QStringList tags;
    tags << tr("name");
    m_filterWidget->SetAcceptedTags(tags, tags[0]);
    layout->addLayout(gridLayout, 1);

    // Left Panel
    QVBoxLayout* leftPaneLayout = new QVBoxLayout(this);

    // Favorites
    leftPaneLayout->addWidget(new QLabel(tr("Favorites")));
    leftPaneLayout->addWidget(new QLabel(tr("Drag components here to add favorites.")));
    FavoritesList* favorites = new FavoritesList();
    favorites->Init();
    leftPaneLayout->addWidget(favorites);

    // Categories
    m_categoryListWidget = new ComponentCategoryList();
    m_categoryListWidget->Init();
    leftPaneLayout->addWidget(m_categoryListWidget);
    gridLayout->addLayout(leftPaneLayout);

    // Right Panel
    QVBoxLayout* rightPanelLayout = new QVBoxLayout(this);
    gridLayout->addLayout(rightPanelLayout);

    // Component list
    m_componentListWidget = new FilteredComponentList(this);
    m_componentListWidget->Init();

    rightPanelLayout->addWidget(new QLabel(tr("Components")));
    rightPanelLayout->addWidget(m_filterWidget, 0, Qt::AlignTop);
    rightPanelLayout->addWidget(m_componentListWidget);

    // The main window
    QWidget* window = new QWidget();
    window->setLayout(layout);
    setCentralWidget(window);

    connect(m_categoryListWidget, &ComponentCategoryList::OnCategoryChange, m_componentListWidget, &FilteredComponentList::SetCategory);
    connect(m_filterWidget, &AzToolsFramework::SearchCriteriaWidget::SearchCriteriaChanged, m_componentListWidget, &FilteredComponentList::SearchCriteriaChanged);

}

void ComponentPaletteWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier) && event->key() == Qt::Key_F)
    {
        m_filterWidget->SelectTextEntryBox();
    }
    else
    {
        QMainWindow::keyPressEvent(event);
    }
}

void ComponentPaletteWindow::RegisterViewClass()
{
    using namespace AzToolsFramework;

    ViewPaneOptions options;
    options.canHaveMultipleInstances = true;
    RegisterViewPane<ComponentPaletteWindow>("Component Palette", LyViewPane::CategoryOther, options);
}

#include <UI/ComponentPalette/moc_ComponentPaletteWindow.cpp>
