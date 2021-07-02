/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QtCore/QString>
#include <QtCore/QProcess>
#include <QtCore/QDir>

namespace Driller
{
    namespace Platform
    {
        void LaunchExplorerSelect(const QString& filePath)
        {
            QProcess::startDetached("/usr/bin/osascript", {"-e",
                QStringLiteral("tell application \"Finder\" to reveal POSIX file \"%1\"").arg(QDir::toNativeSeparators(filePath))});
            QProcess::startDetached("/usr/bin/osascript", {"-e",
                QStringLiteral("tell application \"Finder\" to activate")});
        }
    }
}
