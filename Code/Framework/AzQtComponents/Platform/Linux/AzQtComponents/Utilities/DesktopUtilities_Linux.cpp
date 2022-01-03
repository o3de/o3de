/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/DesktopUtilities.h>

#include <QDir>
#include <QProcess>

namespace AzQtComponents
{
    void ShowFileOnDesktop(const QString& path)
    {
        const char* defaultNautilusPath = "/usr/bin/nautilus";
        const char* defaultXdgOpenPath = "/usr/bin/xdg-open";

        // Determine if Nautilus (for Gnome Desktops) is available because it supports opening the file manager 
        // and selecting a specific file
        bool nautilusAvailable = QFileInfo(defaultNautilusPath).exists();

        QFileInfo pathInfo(path);
        if (pathInfo.isDir())
        {
            QProcess::startDetached(defaultXdgOpenPath, { path });
        }
        else
        {
            if (nautilusAvailable)
            {
                QProcess::startDetached(defaultNautilusPath, { "--select", path });
            }
            else
            {
                QDir parentDir { pathInfo.dir() };
                QProcess::startDetached(defaultXdgOpenPath, { parentDir.path() });
            }
        }
    }

    QString fileBrowserActionName()
    {
        const char* exploreActionName = "Open in file browser";
        return QObject::tr(exploreActionName);
    }
}
