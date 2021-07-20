/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SliceFavorites_precompiled.h"

#include "ComponentSliceFavoritesWindow.h"
#include "SliceFavoritesWidget.h"
#include "SliceFavoritesSystemComponentBus.h"

#include <QVBoxLayout>
#include <QMenuBar>


ComponentSliceFavoritesWindow::ComponentSliceFavoritesWindow(QWidget* parent)
    : QMainWindow(parent)
{
    Init();
}

ComponentSliceFavoritesWindow::~ComponentSliceFavoritesWindow()
{
}

void ComponentSliceFavoritesWindow::Init()
{
    menuBar()->hide();

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(QMargins());

    SliceFavorites::FavoriteDataModel* dataModel = nullptr;
    
    SliceFavorites::SliceFavoritesSystemComponentRequestBus::BroadcastResult(dataModel, &SliceFavorites::SliceFavoritesSystemComponentRequests::GetSliceFavoriteDataModel);

    layout->addWidget(new SliceFavorites::SliceFavoritesWidget(dataModel));

    QWidget* window = new QWidget();
    window->setLayout(layout);
    setCentralWidget(window);
}

#include <Source/moc_ComponentSliceFavoritesWindow.cpp>

