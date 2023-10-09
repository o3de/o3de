/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzAssetBrowser/AzAssetBrowserMultiWindow.h>

#include "QtViewPaneManager.h"

using namespace AzToolsFramework::AssetBrowser;

static constexpr int MaxWindowAmount = 10;

AzAssetBrowserWindow* AzAssetBrowserMultiWindow::OpenNewAssetBrowserWindow()
{
    int id = 1;

    while (id < MaxWindowAmount)
    {
        QString candidateName = QString("%1 (%2)").arg(LyViewPane::AssetBrowser).arg(id);
        if (id == 1)
        {
            // Special case, no trailing id.
            candidateName = QString("%1").arg(LyViewPane::AssetBrowser);
        }

        QtViewPane* pane = QtViewPaneManager::instance()->GetPane(candidateName);
        if (pane && !pane->IsVisible())
        {
            // The pane has been registered but is not in use: use that one.
            return qobject_cast<AzAssetBrowserWindow*>(QtViewPaneManager::instance()->OpenPane(candidateName)->Widget());
        }
        else if (!pane)
        {
            // All currently registered panes are visible. Register another one and use that.
            AzToolsFramework::ViewPaneOptions options;
            options.preferedDockingArea = Qt::BottomDockWidgetArea;
            options.showInMenu = false;
            AzToolsFramework::RegisterViewPane<AzAssetBrowserWindow>(qPrintable(candidateName), LyViewPane::CategoryTools, options);
            return qobject_cast<AzAssetBrowserWindow*>(QtViewPaneManager::instance()->OpenPane(candidateName)->Widget());
        }

        id++;
    }

    return nullptr;
}

bool AzAssetBrowserMultiWindow::IsAnyAssetBrowserWindowOpen()
{
    int id = 1;

    while (id < MaxWindowAmount)
    {
        QString candidateName = QString("%1 (%2)").arg(LyViewPane::AssetBrowser).arg(id);
        if (id == 1)
        {
            // Special case, no trailing id.
            candidateName = QString("%1").arg(LyViewPane::AssetBrowser);
        }

        QtViewPane* pane = QtViewPaneManager::instance()->GetPane(candidateName);
        if (pane && pane->IsVisible())
        {
            return true;
        }
    }

    return false;
}
 
