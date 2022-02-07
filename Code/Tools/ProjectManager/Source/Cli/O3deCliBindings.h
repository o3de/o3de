/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// Qt defines slots, which interferes with the use here.
#pragma push_macro("slots")
#undef slots
#include <Python.h>
#include <pybind11/pybind11.h>
#pragma pop_macro("slots")

namespace O3DE::ProjectManager
{
    //! Interface used to interact with the o3de cli python functions
    class O3deCliBindings
    {
    public:
        O3deCliBindings() = default;
        virtual ~O3deCliBindings() = default;

        virtual bool StartPython() = 0;
        virtual bool PythonStarted() = 0;

        virtual pybind11::handle PathLib() = 0;

        virtual pybind11::object GetEngineJson(pybind11::handle enginePath) = 0;
        virtual pybind11::object LoadO3deManifest() = 0;
        virtual pybind11::object GetGemsFolder() = 0;
        virtual pybind11::object GetProjectsFolder() = 0;
        virtual pybind11::object GetRestrictedFolder() = 0;
        virtual pybind11::object GetTemplatesFolder() = 0;
        virtual pybind11::object GetThirdPartyFolder() = 0;
        virtual pybind11::object GetManifestEngines() = 0;
        virtual pybind11::object GetThisEnginePath() = 0;
        virtual pybind11::object GetRegisterEnginePath(pybind11::str engineName) = 0;
        virtual int EditEngine(pybind11::handle enginePath, pybind11::str engineName, pybind11::str engineVersion) = 0;
        virtual int RegisterEngine(
            pybind11::handle enginePath,
            pybind11::handle projectsFolderPath,
            pybind11::handle gemsFolderPath,
            pybind11::handle templatesFolderPath,
            pybind11::handle thirdPartyPath,
            bool force) = 0;
        virtual pybind11::object GetEngineGems() = 0;
        virtual pybind11::object GetAllGems(pybind11::handle projectPath) = 0;
        virtual pybind11::object GetGemsCmakeFilePath(pybind11::handle projectPath) = 0;
        virtual pybind11::object GetEnabledGemNames(pybind11::handle cmakeFilePath) = 0;
        virtual int RegisterGem(pybind11::handle gemPath, pybind11::handle externalProjectPath, bool remove) = 0;
        virtual int RegisterProject(pybind11::handle projectPath, bool remove) = 0;
        virtual int CreateProject(pybind11::handle projectPath, pybind11::str projectName, pybind11::handle templatePath) = 0;
        virtual pybind11::object GetGemJson(pybind11::handle gemPath, pybind11::handle projectPath) = 0;
        virtual pybind11::object GetProjectJson(pybind11::handle projectPath) = 0;
        virtual pybind11::object GetManifestProjects() = 0;
        virtual pybind11::object GetEngineProjects() = 0;
        virtual int EnableProjectGem(pybind11::handle gemPath, pybind11::handle projectPath) = 0;
        virtual int DisableProjectGem(pybind11::handle gemPath, pybind11::handle projectPath) = 0;
        virtual int RemoveInvalidProjects() = 0;
        virtual int EditProject(
            pybind11::handle projectPath,
            pybind11::str projectName,
            pybind11::str id,
            pybind11::str origin,
            pybind11::str displayName,
            pybind11::str summary,
            pybind11::str iconPath,
            pybind11::list tags) = 0;
        virtual pybind11::object GetTemplateJson(pybind11::handle templatePath, pybind11::handle projectPath) = 0;
        virtual pybind11::object GetTemplates() = 0;
        virtual int RefreshRepo(pybind11::str repoUri) = 0;
        virtual int RefreshRepos() = 0;
        virtual int RegisterRepo(pybind11::str repoUri, bool remove) = 0;
        virtual pybind11::object GetRepoJson(pybind11::handle repoUri) = 0;
        virtual pybind11::object GetRepoPath(pybind11::handle repoUri) = 0;
        virtual pybind11::object GetReposUris() = 0;
        virtual pybind11::object GetCachedGemJsonPaths(pybind11::str repoUri) = 0;
        virtual pybind11::object GetAllCachedGemJsonPaths() = 0;
        virtual int DownloadGem(pybind11::str gemName, pybind11::cpp_function callback, bool force) = 0;
        virtual bool IsGemUpdateAvaliable(pybind11::str gemName, pybind11::str lastUpdated) = 0;
    };
} // namespace O3DE::ProjectManager
