/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
