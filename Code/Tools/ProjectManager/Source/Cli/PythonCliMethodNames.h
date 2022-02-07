/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>

namespace O3DE::ProjectManager
{
    namespace PyCli
    {
        constexpr static const char* RegisterMethod                 = "register";
        constexpr static const char* GetEngineJsonMethod            = "get_engine_json_data";
        constexpr static const char* LoadManifestMethod             = "load_o3de_manifest";
        constexpr static const char* GetGemsFolderMethod            = "get_o3de_gems_folder";
        constexpr static const char* GetProjectsFolderMethod        = "get_o3de_projects_folder";
        constexpr static const char* GetRestrictedFolderMethod      = "get_o3de_restricted_folder";
        constexpr static const char* GetTemplatesFolderMethod       = "get_o3de_templates_folder";
        constexpr static const char* GetThirdPartyFolderMethod      = "get_o3de_third_party_folder";
        constexpr static const char* GetManifestEnginesMethod       = "get_manifest_engines";
        constexpr static const char* GetThisEnginePathMethod        = "get_this_engine_path";
        constexpr static const char* GetRegisterEnginePathMethod    = "get_registered";
        constexpr static const char* EditEngineMethod               = "edit_engine_props";
        constexpr static const char* GetEngineGemsMethod            = "get_engine_gems";
        constexpr static const char* GetAllGemsMethod               = "get_all_gems";
        constexpr static const char* GetGemsCmakeFilePathMethod     = "get_enabled_gem_cmake_file";
        constexpr static const char* GetEnabledGemNamesMethod       = "get_enabled_gems";
        constexpr static const char* CreateProjectMethod            = "create_project";
        constexpr static const char* GetGemJsonMethod               = "get_gem_json_data";
        constexpr static const char* GetProjectJsonMethod           = "get_project_json_data";
        constexpr static const char* GetManifestProjectsMethod      = "get_manifest_projects";
        constexpr static const char* GetEngineProjectsMethod        = "get_engine_projects";
        constexpr static const char* EnableProjectGemMethod         = "enable_gem_in_project";
        constexpr static const char* DisableProjectGemMethod        = "disable_gem_in_project";
        constexpr static const char* RemoveInvalidProjectsMethod    = "remove_invalid_o3de_projects";
        constexpr static const char* EditProjectMethod              = "edit_project_props";
        constexpr static const char* GetTemplateJsonMethod          = "get_template_json_data";
        constexpr static const char* GetTemplatesMethod             = "get_templates_for_project_creation";
        constexpr static const char* RefreshRepoMethod              = "refresh_repo";
        constexpr static const char* RefreshReposMethod             = "refresh_repos";
        constexpr static const char* GetRepoJsonMethod              = "get_repo_json_data";
        constexpr static const char* GetRepoPathMethod              = "get_repo_path";
        constexpr static const char* GetReposUrisMethod             = "get_manifest_repos";
        constexpr static const char* GetCachedGemJsonPathsMethod    = "get_gem_json_paths_from_cached_repo";
        constexpr static const char* GetAllCachedGemJsonPathsMethod = "get_gem_json_paths_from_all_cached_repos";
        constexpr static const char* DownloadGemMethod              = "download_gem";
        constexpr static const char* IsGemUpdateAvaliableMethod     = "is_o3de_gem_update_available";
    }
} // namespace O3DE::ProjectManager
