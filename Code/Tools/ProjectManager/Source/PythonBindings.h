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
        AZ::Outcome<GemInfo> GetGem(const QString& path) override;
        AZ::Outcome<QVector<GemInfo>> GetGems() override;

        // Project
        AZ::Outcome<ProjectInfo> CreateProject(const QString& projectTemplatePath, const ProjectInfo& projectInfo) override;
        AZ::Outcome<ProjectInfo> GetProject(const QString& path) override;
        AZ::Outcome<QVector<ProjectInfo>> GetProjects() override;
        bool UpdateProject(const ProjectInfo& projectInfo) override;

        // ProjectTemplate
        AZ::Outcome<QVector<ProjectTemplateInfo>> GetProjectTemplates() override;

    private:
        AZ_DISABLE_COPY_MOVE(PythonBindings);

        bool ExecuteWithLock(AZStd::function<void()> executionCallback);
        GemInfo GemInfoFromPath(pybind11::handle path);
        ProjectInfo ProjectInfoFromPath(pybind11::handle path);
        ProjectTemplateInfo ProjectTemplateInfoFromPath(pybind11::handle path);
        bool StartPython();
        bool StopPython();

        AZ::IO::FixedMaxPath m_enginePath;
        pybind11::handle m_engineTemplate;
        AZStd::recursive_mutex m_lock;
        pybind11::handle m_registration;
    };
}
