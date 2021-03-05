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
#pragma once

class QWidget;

namespace FileHelpers
{
    QString GetAbsoluteDir(const char* subDir);
    QString GetAbsoluteGameDir();
    QString GetRelativePathFromEngineRoot(const QString& fullpath);

    void AppendExtensionIfNotPresent(QString& filename,
        const char* extension);

    bool FilenameHasExtension(QString& filename,
        const char* extension);

    QString GetAppDataPath();

    enum class SourceControlResult
    {
        kSourceControlResult_Ok,
        kSourceControlResult_NoSourceControl,
        kSourceControlResult_NotConnected,
        kSourceControlResult_SourceControlDown,
        kSourceControlResult_SourceControlError
    };

    SourceControlResult SourceControlAddOrEdit(const char* fullPath, QWidget* parent);
}   // namespace FileHelpers
