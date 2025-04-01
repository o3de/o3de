/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "../../FileUtil_Common.h"

#include <QProcess>

// Editor
#include "Settings.h"

namespace Platform
{
    bool RunCommandWithArguments(const QString& command, const QStringList& argsList)
    {
        return QProcess::startDetached(command, argsList);
    }

    bool RunEditorWithArg(const QString& editor, const QString& arg)
    {
        return RunCommandWithArguments(gSettings.textureEditor, { editor });
    }

    bool OpenUri(const QUrl& uri)
    {
        return RunCommandWithArguments("xdg-open", { uri.toString() });
    }

    QString GetDefaultEditor(const Common::EditFileType fileType)
    {
        switch (fileType)
        {
        case Common::EditFileType::FILE_TYPE_BSPACE://Drop through
        case Common::EditFileType::FILE_TYPE_SCRIPT:
        case Common::EditFileType::FILE_TYPE_SHADER:
            return "";
        case Common::EditFileType::FILE_TYPE_TEXTURE:
            return "";
        case Common::EditFileType::FILE_TYPE_ANIMATION:
            return "";
        default:
            AZ_Assert(false, "Unknown file type.");
            return "";
        }
    }

    QString MakePlatformFileEditString(QString pathToEdit, int lineToEdit)
    {
        return pathToEdit;
    }

    bool CreatePath(const QString& strPath)
    {
        QString strDriveLetter;
        QString strDirectory;
        QString strFilename;
        QString strExtension;
        QString strCurrentDirectoryPath;
        QStringList cstrDirectoryQueue;
        size_t nCurrentPathQueue(0);
        size_t nTotalPathQueueElements(0);
        BOOL bnLastDirectoryWasCreated(FALSE);

        if (Common::PathExists(strPath))
        {
            return true;
        }

        Path::SplitPath(strPath, strDriveLetter, strDirectory, strFilename, strExtension);
        Path::GetDirectoryQueue(strDirectory, cstrDirectoryQueue);

        if (!strDriveLetter.isEmpty())
        {
            strCurrentDirectoryPath = strDriveLetter;
            strCurrentDirectoryPath += "\\";
        }


        nTotalPathQueueElements = cstrDirectoryQueue.size();
        for (nCurrentPathQueue = 0; nCurrentPathQueue < nTotalPathQueueElements; ++nCurrentPathQueue)
        {
            strCurrentDirectoryPath += cstrDirectoryQueue[nCurrentPathQueue];
            strCurrentDirectoryPath += "\\";
            // The value which will go out of this loop is the result of the attempt to create the
            // last directory, only.

            strCurrentDirectoryPath = Path::CaselessPaths(strCurrentDirectoryPath);
            bnLastDirectoryWasCreated = QDir().mkpath(strCurrentDirectoryPath);
        }

        if (!bnLastDirectoryWasCreated)
        {
            if (!QDir(strCurrentDirectoryPath).exists())
            {
                return false;
            }
        }

        return true;
    }
    
    const char* GetLuaCompilerName()
    {
        return "lua";
    }
}
