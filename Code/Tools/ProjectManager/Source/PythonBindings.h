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

        // ProjectTemplate
        AZ::Outcome<QVector<ProjectTemplateInfo>> GetProjectTemplates(const QString& projectPath = {}) override;

    private:
        AZ_DISABLE_COPY_MOVE(PythonBindings);

        AZ::Outcome<void, AZStd::string> ExecuteWithLockErrorHandling(AZStd::function<void()> executionCallback);
        bool ExecuteWithLock(AZStd::function<void()> executionCallback);
        GemInfo GemInfoFromPath(pybind11::handle path, pybind11::handle pyProjectPath);
        ProjectInfo ProjectInfoFromPath(pybind11::handle path);
        ProjectTemplateInfo ProjectTemplateInfoFromPath(pybind11::handle path, pybind11::handle pyProjectPath);
        bool RegisterThisEngine();
        bool StartPython();
        bool StopPython();


        AZ::IO::FixedMaxPath m_enginePath;
        pybind11::handle m_engineTemplate;
        AZStd::recursive_mutex m_lock;
        pybind11::handle m_cmake;
        pybind11::handle m_register;
        pybind11::handle m_manifest;
        pybind11::handle m_enableGemProject;
        pybind11::handle m_disableGemProject;
        pybind11::handle m_editProjectProperties;
    };
}
