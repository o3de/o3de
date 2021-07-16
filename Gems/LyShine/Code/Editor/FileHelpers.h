/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
