/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <QApplication>
#include <QDir>
#include <QSettings>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QJsonValue>

#include <AzCore/base.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>

#if defined(__APPLE__)
// needed for _NSGetExecutablePath
#include <mach-o/dyld.h>
#include <libgen.h>
#include <unistd.h>
#endif

#if defined(AZ_PLATFORM_LINUX)
#include <libgen.h>
#include <unistd.h>
#endif


namespace AzQtComponents
{

    // the purpose of this function is to set up the QT globals so that it finds its platform libraries and that kind of thing.
    // these paths have to be set up BEFORE you create the Qt application itself, otherwise it won't know how to work on your current platform
    // since it will be missing the plugin for your current platform (windows/osx/etc)
    void PrepareQtPaths()
    {
#if !defined(USE_DEFAULT_QT_LIBRARY_PATHS)
        char executablePath[AZ_MAX_PATH_LEN];
        AZ::Utils::GetExecutablePathReturnType result = AZ::Utils::GetExecutablePath(executablePath, AZ_MAX_PATH_LEN);
        if (result.m_pathStored == AZ::Utils::ExecutablePathResult::Success)
        {
            if (result.m_pathIncludesFilename)
            {
                char* lastSlashAddress = strrchr(executablePath, AZ_CORRECT_FILESYSTEM_SEPARATOR);
                if (lastSlashAddress == executablePath)
                {
                    executablePath[1] = '\0'; //Executable directory is root, therefore set the following character to \0
                }
                else
                {
                    *lastSlashAddress = '\0';
                }
            }
            QApplication::addLibraryPath(executablePath);
        }
        else
        {
            QApplication::addLibraryPath(".");
        }
#endif
    }
} // namespace AzQtComponents

