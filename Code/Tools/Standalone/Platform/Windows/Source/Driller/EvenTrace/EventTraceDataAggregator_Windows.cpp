/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QtCore/QString>
#include <QtCore/QProcess>

namespace Driller
{
    namespace Platform
    {
        void LaunchExplorerSelect(const QString& filePath)
        {
            QString windowsPath = filePath;
            windowsPath.replace('/', "\\");
            QProcess process;
            process.start("explorer", { " /select," + windowsPath });
            process.waitForFinished();
        }    
    }
}
