/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        ProjectGemCatalog,
        Projects,
        UpdateProject,
        UpdateProjectSettings,
        Engine,
        EngineSettings,
        GemRepos,
        GemsGemRepos,
        CreateGem,
        EditGem
    };

    static QHash<QString, ProjectManagerScreen> s_ProjectManagerStringNames = {
        { "Empty", ProjectManagerScreen::Empty},
        { "CreateProject", ProjectManagerScreen::CreateProject},
        { "NewProjectSettings", ProjectManagerScreen::NewProjectSettings},
        { "GemCatalog", ProjectManagerScreen::GemCatalog},
        { "ProjectGemCatalog", ProjectManagerScreen::ProjectGemCatalog},
        { "Projects", ProjectManagerScreen::Projects},
        { "UpdateProject", ProjectManagerScreen::UpdateProject},
        { "UpdateProjectSettings", ProjectManagerScreen::UpdateProjectSettings},
        { "Engine", ProjectManagerScreen::Engine},
        { "EngineSettings", ProjectManagerScreen::EngineSettings},
        { "GemRepos", ProjectManagerScreen::GemRepos},
        { "GemsGemRepos", ProjectManagerScreen::GemsGemRepos},
        { "CreateGem", ProjectManagerScreen::CreateGem },
        { "EditGem", ProjectManagerScreen::EditGem }
    };

    // need to define qHash for ProjectManagerScreen when using scoped enums
    inline uint qHash(ProjectManagerScreen key, uint seed)
    {
        return ::qHash(static_cast<uint>(key), seed);
    }
} // namespace O3DE::ProjectManager
