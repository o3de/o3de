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
        bool CopyProject(const QString& origPath, const QString& newPath);
        bool DeleteProjectFiles(const QString& path, bool force = false);
        bool MoveProject(const QString& origPath, const QString& newPath, QWidget* parent = nullptr);

        bool IsVS2019Installed();

        ProjectManagerScreen GetProjectManagerScreen(const QString& screen);
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
