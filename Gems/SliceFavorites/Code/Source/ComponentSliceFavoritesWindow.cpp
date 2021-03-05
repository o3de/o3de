/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

