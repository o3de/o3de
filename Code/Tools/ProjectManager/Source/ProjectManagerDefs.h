/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QString>

namespace O3DE::ProjectManager
{
    inline constexpr static int MinWindowWidth = 1200;
    inline constexpr static int ProjectPreviewImageWidth = 210;
    inline constexpr static int ProjectPreviewImageHeight = 280;
    inline constexpr static int ProjectTemplateImageWidth = 92;
    inline constexpr static int GemPreviewImageWidth = 70;
    inline constexpr static int GemPreviewImageHeight = 40;
    inline constexpr static int ProjectCommandLineTimeoutSeconds = 30;

    static const QString ProjectBuildDirectoryName = "build";
    extern const QString ProjectBuildPathPostfix;
    extern const QString GetPythonScriptPath;
    static const QString ProjectBuildPathCmakeFiles = "CMakeFiles";
    static const QString ProjectBuildErrorLogName = "CMakeProjectBuildError.log";
    static const QString ProjectExportErrorLogName = "ExportError.log";
    static const QString ProjectCacheDirectoryName = "Cache";
    static const QString ProjectPreviewImagePath = "preview.png";
    
    static const QString ProjectCMakeCommand = "cmake";
    static const QString ProjectCMakeBuildTargetEditor = "Editor";

    static const QString RepoTimeFormat = "MM/dd/yyyy hh:mmap";
    static const QString CanonicalRepoUri = "https://canonical.o3de.org";

} // namespace O3DE::ProjectManager
