/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>

#include <QProcess>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        AZ::Outcome<QString, QString> FindSupportedCompilerForPlatform()
        {
            QStringList supportedClangCommands = { "clang-12", "clang-6" };

            for (QString& supportClangCommand : supportedClangCommands)
            {
                QProcess whereProcess;
                whereProcess.setProcessChannelMode(QProcess::MergedChannels);
                whereProcess.start("which", QStringList { supportClangCommand });
                if (whereProcess.waitForStarted() && whereProcess.waitForFinished())
                {
                    if (whereProcess.exitCode() == 0)
                    {
                        return AZ::Success(supportClangCommand);
                    }
                }
            }
            return AZ::Failure(QString("Unable to resolve clang. Make sure that clang-6.0 or clang-12 is installed."));
        }
        
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
