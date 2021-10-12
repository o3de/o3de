/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PythonBindingsInterface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/parallel/semaphore.h>

// Qt defines slots, which interferes with the use here.
#pragma push_macro("slots")
#undef slots
#include <Python.h>
#include <pybind11/pybind11.h>
#pragma pop_macro("slots")


namespace O3DE::ProjectManager
{
    class PythonBindings
        : public PythonBindingsInterface::Registrar
    {
    public:
        PythonBindings() = default;
        PythonBindings(const AZ::IO::PathView& enginePath);
        ~PythonBindings() override;

        // PythonBindings overrides
        bool PythonStarted() override;
        bool StartPython() override;

        // Engine
        AZ::Outcome<EngineInfo> GetEngineInfo() override;
        bool SetEngineInfo(const EngineInfo& engineInfo) override;

        // Gem
        AZ::Outcome<GemInfo> GetGemInfo(const QString& path, const QString& projectPath = {}) override;
        AZ::Outcome<QVector<GemInfo>, AZStd::string> GetEngineGemInfos() override;
        AZ::Outcome<QVector<GemInfo>, AZStd::string> GetAllGemInfos(const QString& projectPath) override;
        AZ::Outcome<QVector<AZStd::string>, AZStd::string> GetEnabledGemNames(const QString& projectPath) override;

        // Project
        AZ::Outcome<ProjectInfo> CreateProject(const QString& projectTemplatePath, const ProjectInfo& projectInfo) override;
        AZ::Outcome<ProjectInfo> GetProject(const QString& path) override;
        AZ::Outcome<QVector<ProjectInfo>> GetProjects() override;
        bool AddProject(const QString& path) override;
        bool RemoveProject(const QString& path) override;
        AZ::Outcome<void, AZStd::string> UpdateProject(const ProjectInfo& projectInfo) override;
        AZ::Outcome<void, AZStd::string> AddGemToProject(const QString& gemPath, const QString& projectPath) override;
        AZ::Outcome<void, AZStd::string> RemoveGemFromProject(const QString& gemPath, const QString& projectPath) override;
        bool RemoveInvalidProjects() override;

        // ProjectTemplate
        AZ::Outcome<QVector<ProjectTemplateInfo>> GetProjectTemplates(const QString& projectPath = {}) override;

        // Gem Repos
        AZ::Outcome<void, AZStd::string> AddGemRepo(const QString& repoUri) override;
        AZ::Outcome<QVector<GemRepoInfo>, AZStd::string> GetAllGemRepoInfos() override;

    private:
        AZ_DISABLE_COPY_MOVE(PythonBindings);

        AZ::Outcome<void, AZStd::string> ExecuteWithLockErrorHandling(AZStd::function<void()> executionCallback);
        bool ExecuteWithLock(AZStd::function<void()> executionCallback);
        GemInfo GemInfoFromPath(pybind11::handle path, pybind11::handle pyProjectPath);
        GemRepoInfo GetGemRepoInfo(pybind11::handle repoUri);
        ProjectInfo ProjectInfoFromPath(pybind11::handle path);
        ProjectTemplateInfo ProjectTemplateInfoFromPath(pybind11::handle path, pybind11::handle pyProjectPath);
        bool RegisterThisEngine();
        bool StopPython();


        bool m_pythonStarted = false;

        AZ::IO::FixedMaxPath m_enginePath;
        AZStd::recursive_mutex m_lock;

        pybind11::handle m_engineTemplate;
        pybind11::handle m_cmake;
        pybind11::handle m_register;
        pybind11::handle m_manifest;
        pybind11::handle m_enableGemProject;
        pybind11::handle m_disableGemProject;
        pybind11::handle m_editProjectProperties;
        pybind11::handle m_pathlib;
    };
}
