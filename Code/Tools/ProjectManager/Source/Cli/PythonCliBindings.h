/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Cli/O3deCliBindings.h>
#include <Cli/O3deCliInterface.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/parallel/mutex.h>

namespace O3DE::ProjectManager
{
    class PythonCliBindings
        : public O3deCliBindings
    {
    public:
        PythonCliBindings() = default;
        PythonCliBindings(const AZ::IO::PathView& enginePath);
        ~PythonCliBindings() override;

        bool StartPython() override;
        bool PythonStarted() override;

        pybind11::handle PathLib() override;

        pybind11::object GetEngineJson(pybind11::handle enginePath) override;
        pybind11::object LoadO3deManifest() override;
        pybind11::object GetGemsFolder() override;
        pybind11::object GetProjectsFolder() override;
        pybind11::object GetRestrictedFolder() override;
        pybind11::object GetTemplatesFolder() override;
        pybind11::object GetThirdPartyFolder() override;
        pybind11::object GetManifestEngines() override;
        pybind11::object GetThisEnginePath() override;
        pybind11::object GetRegisterEnginePath(pybind11::str engineName) override;
        int EditEngine(pybind11::handle enginePath, pybind11::str engineName, pybind11::str engineVersion) override;
        int RegisterEngine(
            pybind11::handle enginePath,
            pybind11::handle projectsFolderPath,
            pybind11::handle gemsFolderPath,
            pybind11::handle templatesFolderPath,
            pybind11::handle thirdPartyPath,
            bool force) override;
        pybind11::object GetEngineGems() override;
        pybind11::object GetAllGems(pybind11::handle projectPath) override;
        pybind11::object GetGemsCmakeFilePath(pybind11::handle projectPath) override;
        pybind11::object GetEnabledGemNames(pybind11::handle cmakeFilePath) override;
        int RegisterGem(pybind11::handle gemPath, pybind11::handle externalProjectPath, bool remove) override;
        int RegisterProject(pybind11::handle projectPath, bool remove) override;
        int CreateProject(pybind11::handle projectPath, pybind11::str projectName, pybind11::handle templatePath) override;
        pybind11::object GetGemJson(pybind11::handle gemPath, pybind11::handle projectPath) override;
        pybind11::object GetProjectJson(pybind11::handle projectPath) override;
        pybind11::object GetManifestProjects() override;
        pybind11::object GetEngineProjects() override;
        int EnableProjectGem(pybind11::handle gemPath, pybind11::handle projectPath) override;
        int DisableProjectGem(pybind11::handle gemPath, pybind11::handle projectPath) override;
        int RemoveInvalidProjects() override;
        int EditProject(
            pybind11::handle projectPath,
            pybind11::str projectName,
            pybind11::str id,
            pybind11::str origin,
            pybind11::str displayName,
            pybind11::str summary,
            pybind11::str iconPath,
            pybind11::list tags) override;
        pybind11::object GetTemplateJson(pybind11::handle templatePath, pybind11::handle projectPath) override;
        pybind11::object GetTemplates() override;
        int RefreshRepo(pybind11::str repoUri) override;
        int RefreshRepos() override;
        int RegisterRepo(pybind11::str repoUri, bool remove) override;
        pybind11::object GetRepoJson(pybind11::handle repoUri) override;
        pybind11::object GetRepoPath(pybind11::handle repoUri) override;
        pybind11::object GetReposUris() override;
        pybind11::object GetCachedGemJsonPaths(pybind11::str repoUri) override;
        pybind11::object GetAllCachedGemJsonPaths() override;
        int DownloadGem(pybind11::str gemName, pybind11::cpp_function callback, bool force) override;
        bool IsGemUpdateAvaliable(pybind11::str gemName, pybind11::str lastUpdated) override;

    private:
        AZ_DISABLE_COPY_MOVE(PythonCliBindings);

        bool StopPython();


        bool m_pythonStarted = false;

        AZ::IO::FixedMaxPath m_enginePath;
        AZStd::recursive_mutex m_lock;

        pybind11::module m_engineTemplate;
        pybind11::module m_engineProperties;
        pybind11::module m_cmake;
        pybind11::module m_register;
        pybind11::module m_manifest;
        pybind11::module m_enableGemProject;
        pybind11::module m_disableGemProject;
        pybind11::module m_editProjectProperties;
        pybind11::module m_download;
        pybind11::module m_repo;
        pybind11::module m_pathlib;
    };
}
