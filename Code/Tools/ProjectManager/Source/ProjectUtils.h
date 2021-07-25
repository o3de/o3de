/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScreenDefs.h>
#include <ProjectInfo.h>

#include <QWidget>
#include <QDesktopWidget>
#include <QFrame>
#include <AzCore/Outcome/Outcome.h>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        bool AddProjectDialog(QWidget* parent = nullptr);
        bool RegisterProject(const QString& path);
        bool UnregisterProject(const QString& path);
        bool CopyProjectDialog(const QString& origPath, ProjectInfo& newProjectInfo, QWidget* parent = nullptr);
        bool CopyProject(const QString& origPath, const QString& newPath, QWidget* parent, bool skipRegister = false);
        bool DeleteProjectFiles(const QString& path, bool force = false);
        bool MoveProject(QString origPath, QString newPath, QWidget* parent, bool skipRegister = false);

        bool ReplaceFile(const QString& origFile, const QString& newFile, QWidget* parent = nullptr, bool interactive = true);

        bool FindSupportedCompiler(QWidget* parent = nullptr);
        AZ::Outcome<void, QString> FindSupportedCompilerForPlatform();

        ProjectManagerScreen GetProjectManagerScreen(const QString& screen);

        /// <summary>
        /// Fit a frame into center of screen
        /// This method can resize a frame if is greater than screen size
        /// </summary>
        /// <param name="frame">Target Frame</param>
        /// <param name="desktopWidget">Desktop Widget</param>
        void FitFrameOnScreen(QFrame* frame, bool relativeToMouse = false);
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
