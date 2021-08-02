/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        AZ::Outcome<void, QString> FindSupportedCompilerForPlatform()
        {
            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            QString programFilesPath = environment.value("ProgramFiles(x86)");
            QString vsWherePath = QDir(programFilesPath).filePath("Microsoft Visual Studio/Installer/vswhere.exe");

            QFileInfo vsWhereFile(vsWherePath);
            if (vsWhereFile.exists() && vsWhereFile.isFile())
            {
                QProcess vsWhereProcess;
                vsWhereProcess.setProcessChannelMode(QProcess::MergedChannels);

                vsWhereProcess.start(
                    vsWherePath,
                    QStringList{
                        "-version",
                        "16.9.2",
                        "-latest",
                        "-requires",
                        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                        "-property",
                        "isComplete"
                    });

                if (vsWhereProcess.waitForStarted() && vsWhereProcess.waitForFinished())
                {
                    QString vsWhereOutput(vsWhereProcess.readAllStandardOutput());
                    if (vsWhereOutput.startsWith("1"))
                    {
                        return AZ::Success();
                    }
                }
            }

            return AZ::Failure(QObject::tr("Visual Studio 2019 version 16.9.2 or higher not found.\n\n"
                "Visual Studio 2019 is required to build this project."
                " Install any edition of <a href='https://visualstudio.microsoft.com/downloads/'>Visual Studio 2019</a>"
                " or update to a newer version before proceeding to the next step."
                " While installing configure Visual Studio with these <a href='https://o3de.org/docs/welcome-guide/setup/requirements/#visual-studio-configuration'>workloads</a>."));
        }
        
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
