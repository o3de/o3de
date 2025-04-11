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
        AZ::Outcome<QVector<EngineInfo>> GetAllEngineInfos() override;
        AZ::Outcome<EngineInfo> GetEngineInfo() override;
        AZ::Outcome<EngineInfo> GetEngineInfo(const QString& engineName) override;
        AZ::Outcome<EngineInfo> GetProjectEngine(const QString& projectPath) override;
        DetailedOutcome SetEngineInfo(const EngineInfo& engineInfo, bool force = false) override;

        // Remote sources
        bool ValidateRepository(const QString& repoUri) override;

        // Gem
        AZ::Outcome<GemInfo> CreateGem(const QString& templatePath, const GemInfo& gemInfo, bool registerGem = true) override;
        AZ::Outcome<GemInfo> EditGem(const QString& oldGemName, const GemInfo& newGemInfo) override;
        AZ::Outcome<GemInfo> GetGemInfo(const QString& path, const QString& projectPath = {}) override;
        AZ::Outcome<QVector<GemInfo>, AZStd::string> GetAllGemInfos(const QString& projectPath) override;
        AZ::Outcome<QHash<QString /*gem name with specifier*/, QString /* gem path */>, AZStd::string> GetEnabledGems(
            const QString& projectPath, bool includeDependencies) const override;
        AZ::Outcome<void, AZStd::string> RegisterGem(const QString& gemPath, const QString& projectPath = {}) override;
        AZ::Outcome<void, AZStd::string> UnregisterGem(const QString& gemPath, const QString& projectPath = {}) override;

        // Project
        AZ::Outcome<ProjectInfo, IPythonBindings::ErrorPair> CreateProject(const QString& projectTemplatePath, const ProjectInfo& projectInfo, bool registerProject = true) override;
        AZ::Outcome<ProjectInfo> GetProject(const QString& path) override;
        AZ::Outcome<QVector<ProjectInfo>> GetProjects() override;
        AZ::Outcome<QVector<ProjectInfo>, AZStd::string> GetProjectsForRepo(const QString& repoUri, bool enabledOnly = true) override;
        AZ::Outcome<QVector<ProjectInfo>, AZStd::string> GetProjectsForAllRepos(bool enabledOnly = true) override;
        DetailedOutcome AddProject(const QString& path, bool force = false) override;
        DetailedOutcome RemoveProject(const QString& path) override;
        AZ::Outcome<void, AZStd::string> UpdateProject(const ProjectInfo& projectInfo) override;
        AZ::Outcome<QStringList, AZStd::string> GetIncompatibleProjectGems(
            const QStringList& gemPaths, const QStringList& gemNames, const QString& projectPath) override;
        AZ::Outcome<QStringList, IPythonBindings::ErrorPair> GetProjectEngineIncompatibleObjects(
            const QString& projectPath, const QString& enginePath = "") override;
        DetailedOutcome AddGemsToProject(
            const QStringList& gemPaths, const QStringList& gemNames, const QString& projectPath, bool force = false) override;
        AZ::Outcome<void, AZStd::string> RemoveGemFromProject(const QString& gemName, const QString& projectPath) override;
        bool RemoveInvalidProjects() override;

        // Remote Repos
        AZ::Outcome<void, AZStd::string> RefreshGemRepo(const QString& repoUri, bool downloadMissingOnly = false) override;
        bool RefreshAllGemRepos(bool downloadMissingOnly = false) override;
        DetailedOutcome AddGemRepo(const QString& repoUri) override;
        bool RemoveGemRepo(const QString& repoUri) override;
        bool SetRepoEnabled(const QString& repoUri, bool enabled) override;
        AZ::Outcome<QVector<GemRepoInfo>, AZStd::string> GetAllGemRepoInfos() override;
        AZ::Outcome<QVector<GemInfo>, AZStd::string> GetGemInfosForRepo(const QString& repoUri, bool enabledOnly = true) override;
        AZ::Outcome<QVector<GemInfo>, AZStd::string> GetGemInfosForAllRepos(const QString& projectPath, bool enabledOnly = true) override;
        DetailedOutcome DownloadGem(
            const QString& gemName, const QString& path, std::function<void(int, int)> gemProgressCallback, bool force = false) override;
        DetailedOutcome DownloadProject(
            const QString& projectName, const QString& path, std::function<void(int, int)> projectProgressCallback, bool force = false) override;
        DetailedOutcome DownloadTemplate(
            const QString& templateName, const QString& path, std::function<void(int, int)> templateProgressCallback, bool force = false) override;
        void CancelDownload() override;
        bool IsGemUpdateAvaliable(const QString& gemName, const QString& lastUpdated) override;

        // Templates
        AZ::Outcome<QVector<ProjectTemplateInfo>> GetProjectTemplates() override;
        AZ::Outcome<QVector<ProjectTemplateInfo>> GetProjectTemplatesForRepo(const QString& repoUri, bool enabledOnly = true) const override;
        AZ::Outcome<QVector<ProjectTemplateInfo>> GetProjectTemplatesForAllRepos(bool enabledOnly = true) const override;
        AZ::Outcome<QVector<TemplateInfo>> GetGemTemplates() override;

        void AddErrorString(AZStd::string errorString) override;

    protected:
        static void OnStdOut(const char* msg); 
        static void OnStdError(const char* msg); 

    private:
        AZ_DISABLE_COPY_MOVE(PythonBindings);

        AZ::Outcome<void, AZStd::string> ExecuteWithLockErrorHandling(AZStd::function<void()> executionCallback) const;
        bool ExecuteWithLock(AZStd::function<void()> executionCallback) const;
        EngineInfo EngineInfoFromPath(pybind11::handle enginePath);
        GemInfo GemInfoFromPath(pybind11::handle path, pybind11::handle pyProjectPath);
        GemRepoInfo GetGemRepoInfo(pybind11::handle repoUri);
        ProjectInfo ProjectInfoFromPath(pybind11::handle path);
        ProjectInfo ProjectInfoFromDict(pybind11::handle projectData, const QString& path = {});
        ProjectTemplateInfo ProjectTemplateInfoFromPath(pybind11::handle path) const;
        ProjectTemplateInfo ProjectTemplateInfoFromDict(pybind11::handle templateData, const QString& path = {}) const;
        TemplateInfo TemplateInfoFromPath(pybind11::handle path) const;
        TemplateInfo TemplateInfoFromDict(pybind11::handle templateData, const QString& path = {}) const;
        AZ::Outcome<void, AZStd::string> GemRegistration(const QString& gemPath, const QString& projectPath, bool remove = false);
        bool StopPython();
        IPythonBindings::ErrorPair GetErrorPair();
        bool ExtendSysPath(const AZStd::vector<AZ::IO::Path>& extendPaths);
    
        bool m_pythonStarted = false;

        AZ::IO::FixedMaxPath m_enginePath;
        mutable AZStd::recursive_mutex m_lock;

        pybind11::handle m_gemProperties;
        pybind11::handle m_engineTemplate;
        pybind11::handle m_engineProperties;
        pybind11::handle m_register;
        pybind11::handle m_manifest;
        pybind11::handle m_enableGemProject;
        pybind11::handle m_disableGemProject;
        pybind11::handle m_editProjectProperties;
        pybind11::handle m_projectManagerInterface;
        pybind11::handle m_download;
        pybind11::handle m_repo;
        pybind11::handle m_pathlib;

        bool m_requestCancelDownload = false;
        mutable AZStd::vector<AZStd::string> m_pythonErrorStrings;
    };
} // namespace O3DE::ProjectManager
