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
        AZ::Outcome<QProcessEnvironment, QString> GetCommandLineProcessEnvironment()
        {
            QProcessEnvironment currentEnvironment(QProcessEnvironment::systemEnvironment());
            QString pathValue = currentEnvironment.value("PATH");
            pathValue += ":/usr/local/bin";
            currentEnvironment.insert("PATH", pathValue);
            return AZ::Success(currentEnvironment);
        }

        AZ::Outcome<QString, QString> FindSupportedCompilerForPlatform()
        {
            QProcessEnvironment currentEnvironment(QProcessEnvironment::systemEnvironment());
            QString pathValue = currentEnvironment.value("PATH");
            pathValue += ":/usr/local/bin";
            currentEnvironment.insert("PATH", pathValue);

            // Validate that we have cmake installed first
            auto queryCmakeInstalled = ExecuteCommandResult("which",QStringList {"cmake"}, currentEnvironment);
            if (!queryCmakeInstalled.IsSuccess())
            {
                return AZ::Failure(QObject::tr("Unable to detect CMake on this host."));
            }
            QString cmakeInstalledPath = queryCmakeInstalled.GetValue().split("\n")[0];

            // Query the version of the installed cmake
            auto queryCmakeVersionQuery = ExecuteCommandResult(cmakeInstalledPath, QStringList {"-version"}, currentEnvironment);
            if (!queryCmakeVersionQuery.IsSuccess())
            {
                return AZ::Failure(QObject::tr("Unable to detect CMake on this host."));
            }
            AZ_TracePrintf("Project Manager", "Cmake version %s detected.", queryCmakeVersionQuery.GetValue().split("\n")[0].toUtf8().constData());

            // Query for the version of xcodebuild (if installed)
            auto queryXcodeBuildVersion = ExecuteCommandResult("xcodebuild",QStringList {"-version"}, currentEnvironment);
            if (!queryCmakeInstalled.IsSuccess())
            {
                return AZ::Failure(QObject::tr("Unable to detect XCodeBuilder on this host."));
            }
            QString xcodeBuilderVersionNumber = queryXcodeBuildVersion.GetValue().split("\n")[0];
            AZ_TracePrintf("Project Manager", "XcodeBuilder version %s detected.", xcodeBuilderVersionNumber.toUtf8().constData());


            return AZ::Success(xcodeBuilderVersionNumber);
        }       
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
