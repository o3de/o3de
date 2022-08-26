/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PythonBindings.h>
#include <gmock/gmock.h>

namespace O3DE::ProjectManager
{
    class MockPythonBindings : public PythonBindings
    {
    public:
        // Python
        MOCK_METHOD0(PythonStarted, bool());
        MOCK_METHOD0(StopPython, bool());

        // Engine
        MOCK_METHOD0(GetEngineInfo, AZ::Outcome<EngineInfo>());
        MOCK_METHOD1(GetEngineInfo, AZ::Outcome<EngineInfo>(const QString&));
        MOCK_METHOD2(SetEngineInfo, DetailedOutcome(const EngineInfo&, bool force));

        // Gem
        MOCK_METHOD2(GetGemInfo, AZ::Outcome<GemInfo>(const QString&, const QString&));
        MOCK_METHOD0(GetEngineGemInfos, AZ::Outcome<QVector<GemInfo>, AZStd::string>());
        MOCK_METHOD1(GetAllGemInfos, AZ::Outcome<QVector<GemInfo>, AZStd::string>(const QString&));
        MOCK_METHOD1(GetEnabledGemNames, AZ::Outcome<QVector<AZStd::string>, AZStd::string>(const QString&));
        MOCK_METHOD2(RegisterGem, AZ::Outcome<void, AZStd::string>(const QString&, const QString&));
        MOCK_METHOD2(UnregisterGem, AZ::Outcome<void, AZStd::string>(const QString&, const QString&));

        // Project
        MOCK_METHOD3(CreateProject, AZ::Outcome<ProjectInfo>(const QString&, const ProjectInfo&, bool));
        MOCK_METHOD1(GetProject, AZ::Outcome<ProjectInfo>(const QString&));
        MOCK_METHOD0(GetProjects, AZ::Outcome<QVector<ProjectInfo>>());
        MOCK_METHOD1(AddProject, bool(const QString&));
        MOCK_METHOD1(RemoveProject, bool(const QString&));
        MOCK_METHOD1(UpdateProject, AZ::Outcome<void, AZStd::string>(const ProjectInfo&));
        MOCK_METHOD2(AddGemToProject, AZ::Outcome<void, AZStd::string>(const QString&, const QString&));
        MOCK_METHOD2(RemoveGemFromProject, AZ::Outcome<void, AZStd::string>(const QString&, const QString&));
        MOCK_METHOD0(RemoveInvalidProjects, bool());

        // ProjectTemplate
        MOCK_METHOD0(GetProjectTemplates, AZ::Outcome<QVector<ProjectTemplateInfo>>());
        MOCK_METHOD0(GetProjectTemplatesForAllRepos, AZ::Outcome<QVector<ProjectTemplateInfo>>());
        MOCK_METHOD0(GetGemTemplates, AZ::Outcome<QVector<TemplateInfo>>());

        // Gem Repos
        MOCK_METHOD1(RefreshGemRepo, AZ::Outcome<void, AZStd::string>(const QString&));
        MOCK_METHOD0(RefreshAllGemRepos, bool());
        MOCK_METHOD1(AddGemRepo, DetailedOutcome(const QString&));
        MOCK_METHOD1(RemoveGemRepo, bool(const QString&));
        MOCK_METHOD0(GetAllGemRepoInfos, AZ::Outcome<QVector<GemRepoInfo>, AZStd::string>());
        MOCK_METHOD1(GetGemInfosForRepo, AZ::Outcome<QVector<GemInfo>, AZStd::string>(const QString&));
        MOCK_METHOD0(GetGemInfosForAllRepos, AZ::Outcome<QVector<GemInfo>, AZStd::string>());
        MOCK_METHOD3(DownloadGem, DetailedOutcome(const QString&, std::function<void(int, int)>, bool));
        MOCK_METHOD0(CancelDownload, void());
        MOCK_METHOD2(IsGemUpdateAvaliable, bool(const QString&, const QString&));

        // Errors
        MOCK_METHOD1(AddErrorString, void(AZStd::string));
    };
} // namespace UnitTest
