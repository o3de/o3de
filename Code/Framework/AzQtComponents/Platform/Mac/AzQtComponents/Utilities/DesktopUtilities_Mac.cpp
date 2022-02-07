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
        if (QFileInfo(path).isDir())
        {
            QProcess::startDetached("/usr/bin/osascript", { "-e",
                    QStringLiteral("tell application \"Finder\" to open(\"%1\" as POSIX file)").arg(QDir::toNativeSeparators(path)) });
        }
        else
        {
            QProcess::startDetached("/usr/bin/osascript", { "-e",
                    QStringLiteral("tell application \"Finder\" to reveal POSIX file \"%1\"").arg(QDir::toNativeSeparators(path)) });
        }

        QProcess::startDetached("/usr/bin/osascript", { "-e",
                    QStringLiteral("tell application \"Finder\" to activate") });
    }

    QString fileBrowserActionName()
    {
        const char* exploreActionName = "Open in Finder";
        return QObject::tr(exploreActionName);
    }
}
