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

#include <QString>

namespace O3DE::ProjectManager
{
    inline constexpr static int ProjectPreviewImageWidth = 210;
    inline constexpr static int ProjectPreviewImageHeight = 280;

    static const QString ProjectBuildPathPostfix = "windows_vs2019";
    static const QString ProjectBuildErrorLogPathPostfix = "CMakeFiles/CMakeProjectBuildError.log";
    static const QString ProjectPreviewImagePath = "preview.png";
} // namespace O3DE::ProjectManager
