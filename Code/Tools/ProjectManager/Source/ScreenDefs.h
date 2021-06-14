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

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <QHash>

namespace O3DE::ProjectManager
{
    enum class ProjectManagerScreen
    {
        Invalid = -1,
        Empty,
        CreateProject,
        NewProjectSettings,
        GemCatalog,
        Projects,
        UpdateProject,
        UpdateProjectSettings,
        EngineSettings
    };

    static QHash<QString, ProjectManagerScreen> s_ProjectManagerStringNames = {
        { "Empty", ProjectManagerScreen::Empty},
        { "CreateProject", ProjectManagerScreen::CreateProject},
        { "NewProjectSettings", ProjectManagerScreen::NewProjectSettings},
        { "GemCatalog", ProjectManagerScreen::GemCatalog},
        { "Projects", ProjectManagerScreen::Projects},
        { "UpdateProject", ProjectManagerScreen::UpdateProject},
        { "UpdateProjectSettings", ProjectManagerScreen::UpdateProjectSettings},
        { "EngineSettings", ProjectManagerScreen::EngineSettings}
    };

    // need to define qHash for ProjectManagerScreen when using scoped enums
    inline uint qHash(ProjectManagerScreen key, uint seed)
    {
        return ::qHash(static_cast<uint>(key), seed);
    }
} // namespace O3DE::ProjectManager
