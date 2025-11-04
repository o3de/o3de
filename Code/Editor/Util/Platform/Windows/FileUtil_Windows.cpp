/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "../../FileUtil_Common.h"

#include <Shellapi.h>


namespace Platform
{
    bool RunCommandWithArguments(const QString& command, const QStringList& argsList)
    {
        bool success = false;
        QString args;
        if (!argsList.isEmpty())
        {
            args = argsList.join(" ");
        }

        HINSTANCE hInst = ShellExecuteW( nullptr,
            L"open",
            command.toStdWString().c_str(),
            args.isEmpty() ? NULL : args.toStdWString().c_str(),
            NULL, SW_SHOWNORMAL);

        success = ((DWORD_PTR)hInst > 32);

        return success;
    }

    bool OpenUri(const QUrl& uri)
    {
        return RunCommandWithArguments(uri.toString(), {});
    }

    bool RunEditorWithArg(const QString& editor, const QString& arg)
    {
        // Use the Win32API calls as they aren't limited to running items in the path.
        QString fullTexturePathFixedForWindows = QString(arg.data()).replace('/', '\\');
        return RunCommandWithArguments(editor, { fullTexturePathFixedForWindows });
    }

    QString GetDefaultEditor(const Common::EditFileType fileType)
    {
        switch (fileType)
        {
        case Common::EditFileType::FILE_TYPE_BSPACE://Drop through
        case Common::EditFileType::FILE_TYPE_SCRIPT:
        case Common::EditFileType::FILE_TYPE_SHADER:
            // Try to use a platform default editor rather than the exe assigned to open the appropriate filetype, as that's not necessarily an editor, e.g. Python.
            return "notepad";
        case Common::EditFileType::FILE_TYPE_TEXTURE:
            return "photoshop";
        case Common::EditFileType::FILE_TYPE_ANIMATION:
            return "";
        default:
            AZ_Assert(false, "Unknown file type.");
            return "";
        }
    }

    QString MakePlatformFileEditString(QString pathToEdit, int lineToEdit)
    {
        QString platformPath(pathToEdit);
        platformPath.replace('/', '\\');
        if (lineToEdit != 0)
        {
            platformPath = QStringLiteral("%1/%2/0").arg(pathToEdit).arg(lineToEdit);
        }
        return platformPath;
    }

    bool CreatePath(const QString& strPath)
    {
        QString strDriveLetter;
        QString strDirectory;
        QString strFilename;
        QString strExtension;
        QString strCurrentDirectoryPath;
        QStringList cstrDirectoryQueue;
        int nCurrentPathQueue(0);
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
        return "LuaCompiler.exe";
    }

}
