/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QString>

namespace O3DE::ProjectManager
{
    inline constexpr static int ProjectPreviewImageWidth = 210;
    inline constexpr static int ProjectPreviewImageHeight = 280;

    static const QString ProjectBuildPathPostfix = "build/windows_vs2019";
    static const QString ProjectBuildPathCmakeFiles = "CMakeFiles";
    static const QString ProjectBuildErrorLogName = "CMakeProjectBuildError.log";
    static const QString ProjectPreviewImagePath = "preview.png";
} // namespace O3DE::ProjectManager
