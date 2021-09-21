/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>

#include <QProcessEnvironment>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        AZ::Outcome<QString, QString> FindSupportedCompilerForPlatform()
        {
            // Validate that cmake is installed and is in the command line
            if (QProcess::execute("which", QStringList{ "cmake" }) != 0)
            {
                return AZ::Failure(QString("Unable to resolve cmake in the command line. Make sure that cmake is installed."));
            }

            QStringList supportedClangCommands = { "clang-12", "clang-6" };
            for (const QString& supportClangCommand : supportedClangCommands)
            {

                auto whichClangResult = ProjectUtils::ExecuteCommandResult("which", QStringList{ supportClangCommand }, QProcessEnvironment::systemEnvironment());
                if (whichClangResult.IsSuccess())
                {
                    return AZ::Success(supportClangCommand);
                }
            }
            return AZ::Failure(QString("Unable to resolve clang. Make sure that clang-6.0 or clang-12 is installed."));
        }
        
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
