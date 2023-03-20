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
    bool RunEditorWithArg(const QString editor, const QString arg)
    {
        return QProcess::execute(QString("/usr/bin/open"), { "-a", gSettings.textureEditor, editor }) == 0;
    }

    QString GetDefaultEditor(const Common::EditFileType fileType)
    {
        switch (fileType)
        {
        case Common::EditFileType::FILE_TYPE_BSPACE://Drop through
        case Common::EditFileType::FILE_TYPE_SCRIPT:
        case Common::EditFileType::FILE_TYPE_SHADER:
            // Try to use a platform default editor rather than the exe assigned to open the appropriate filetype, as that's not necessarily an editor, e.g. Python.
            return "TextEdit";
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
        return pathToEdit;
    }

    bool CreatePath(const QString& strPath)
    {
        bool pathCreated = true;

        QString cleanPath = QDir::cleanPath(strPath);
        QDir path(cleanPath);
        if (!path.exists())
        {
            pathCreated = path.mkpath(cleanPath);
        }

        return pathCreated;
    }
    
    const char* GetLuaCompilerName()
    {
        return "lua";
    }
}
