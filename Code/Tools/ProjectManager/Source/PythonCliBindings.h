/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Cli/O3deCliInterface.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/mutex.h>

// Qt defines slots, which interferes with the use here.
#pragma push_macro("slots")
#undef slots
#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/eval.h>
#pragma pop_macro("slots")


namespace O3DE::ProjectManager
{
    class PythonCliBindings
    {
    public:
        PythonCliBindings() = default;
        PythonCliBindings(const AZ::IO::PathView& enginePath);
        ~PythonCliBindings();

        bool StartPython();
        bool PythonStarted();

        pybind11::handle PathLib();

        pybind11::object GetEngineJson(pybind11::handle enginePath);
        pybind11::object LoadO3deManifest();
        pybind11::object GetGemsFolder();
        pybind11::object GetProjectsFolder();
        pybind11::object GetRestrictedFolder();
        pybind11::object GetTemplatesFolder();
        pybind11::object GetThirdPartyFolder();
        pybind11::object GetManifestEngines();
        pybind11::object GetThisEnginePath();
        pybind11::object GetRegisterEnginePath(pybind11::str engineName);
        int EditEngine(pybind11::handle enginePath, pybind11::str engineName, pybind11::str engineVersion);
        int RegisterEngine(
            pybind11::handle enginePath,
            pybind11::handle projectsFolderPath,
            pybind11::handle gemsFolderPath,
            pybind11::handle templatesFolderPath,
            pybind11::handle thirdPartyPath,
            bool force);
        pybind11::object GetEngineGems();
        pybind11::object GetAllGems(pybind11::handle projectPath);
        pybind11::object GetGemsCmakeFilePath(pybind11::handle projectPath);
        pybind11::object GetEnabledGemNames(pybind11::handle cmakeFilePath);
        int RegisterGem(pybind11::handle gemPath, pybind11::handle externalProjectPath, bool remove);
        int RegisterProject(pybind11::handle projectPath, bool remove);
        int CreateProject(pybind11::handle projectPath, pybind11::str projectName, pybind11::handle templatePath);
        pybind11::object GetGemJson(pybind11::handle gemPath, pybind11::handle projectPath);
        pybind11::object GetProjectJson(pybind11::handle projectPath);
        pybind11::object GetManifestProjects();
        pybind11::object GetEngineProjects();
        int EnableProjectGem(pybind11::handle gemPath, pybind11::handle projectPath);
        int DisableProjectGem(pybind11::handle gemPath, pybind11::handle projectPath);
        int RemoveInvalidProjects();
        int EditProject(
            pybind11::handle projectPath,
            pybind11::str projectName,
            pybind11::str id,
            pybind11::str origin,
            pybind11::str displayName,
            pybind11::str summary,
            pybind11::str iconPath,
            pybind11::list tags);
        pybind11::object GetTemplateJson(pybind11::handle templatePath, pybind11::handle projectPath);
        pybind11::object GetTemplates();
        int RefreshRepo(pybind11::str repoUri);
        int RefreshRepos();
        int RegisterRepo(pybind11::str repoUri, bool remove);
        pybind11::object GetRepoJson(pybind11::handle repoUri);
        pybind11::object GetRepoPath(pybind11::handle repoUri);
        pybind11::object GetReposUris();
        pybind11::object GetCachedGemJsonPaths(pybind11::str repoUri);
        pybind11::object GetAllCachedGemJsonPaths();
        int DownloadGem(pybind11::str gemName, pybind11::cpp_function callback, bool force);
        bool IsGemUpdateAvaliable(pybind11::str gemName, pybind11::str lastUpdated);

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
