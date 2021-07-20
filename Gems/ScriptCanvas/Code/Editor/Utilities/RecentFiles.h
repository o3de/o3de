/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/string/string.h>

#include <QStringList>
#include <QString>

const size_t c_scriptCanvasEditorSettingsRecentFilesCountMax = 10;

namespace ScriptCanvasEditor
{
    QStringList ReadRecentFiles();
    void WriteRecentFiles(const QStringList& filenames);
    void AddRecentFile(const QString& filename);
    void ClearRecentFile();
}
