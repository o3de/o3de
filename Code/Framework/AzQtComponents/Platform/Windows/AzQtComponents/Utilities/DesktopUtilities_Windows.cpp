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
        // Launch explorer at the path provided
        QStringList args;
        if (!QFileInfo(path).isDir())
        {
            // Folders are just opened, files are selected
            args << "/select,";
        }
        args << QDir::toNativeSeparators(path);

        QProcess::startDetached("explorer", args);
    }

    QString fileBrowserActionName()
    {
        const char* exploreActionName = "Open in Explorer";
        return QObject::tr(exploreActionName);
    }
}
