/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScreenDefs.h>
#include <QWidget>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        bool AddProjectDialog(QWidget* parent = nullptr);
        bool RegisterProject(const QString& path);
        bool UnregisterProject(const QString& path);
        bool CopyProjectDialog(const QString& origPath, QWidget* parent = nullptr);
        bool CopyProject(const QString& origPath, const QString& newPath, QWidget* parent);
        bool DeleteProjectFiles(const QString& path, bool force = false);
        bool MoveProject(QString origPath, QString newPath, QWidget* parent = nullptr, bool ignoreRegister = false);

        bool ReplaceFile(const QString& origFile, const QString& newFile, QWidget* parent = nullptr, bool interactive = true);

        bool IsVS2019Installed();

        ProjectManagerScreen GetProjectManagerScreen(const QString& screen);
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
