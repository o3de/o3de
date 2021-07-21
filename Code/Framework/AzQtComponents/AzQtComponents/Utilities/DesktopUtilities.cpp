/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#if defined(AZ_PLATFORM_WINDOWS)

        // Launch explorer at the path provided
        QStringList args;
        if (!QFileInfo(path).isDir())
        {
            // Folders are just opened, files are selected
            args << "/select,";
        }
        args << QDir::toNativeSeparators(path);

        QProcess::startDetached("explorer", args);

#else

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

#endif
    }

    QString fileBrowserActionName()
    {
#ifdef AZ_PLATFORM_WINDOWS
        const char* exploreActionName = "Open in Explorer";
#elif defined(AZ_PLATFORM_MAC)
        const char* exploreActionName = "Open in Finder";
#else
        const char* exploreActionName = "Open in file browser";
#endif
        return QObject::tr(exploreActionName);
    }
}
