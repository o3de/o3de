/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
