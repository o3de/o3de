/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Cli/PythonBindings.h>

#include <ProjectManagerDefs.h>

// Qt defines slots, which interferes with the use here.
#pragma push_macro("slots")
#undef slots
#include <pybind11/functional.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/stl.h>
#pragma pop_macro("slots")

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/numeric.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <QDir>

#define Py_To_String(obj) pybind11::str(obj).cast<std::string>().c_str()
#define Py_To_String_Optional(dict, key, default_string) dict.contains(key) ? Py_To_String(dict[key]) : default_string
#define Py_To_Int(obj) obj.cast<int>()
#define Py_To_Int_Optional(dict, key, default_int) dict.contains(key) ? Py_To_Int(dict[key]) : default_int
#define QString_To_Py_String(value) pybind11::str(value.toStdString())
#define QString_To_Py_Path(value) m_cliBindings->PathLib().attr("Path")(value.toStdString())


namespace O3DE::ProjectManager
{
    PythonBindings::PythonBindings(O3deCliBindings* cliBindings)
        : m_cliBindings(cliBindings)
    {
    }

    PythonBindings::~PythonBindings()
    {
        m_cliBindings.reset();
    }

    bool PythonBindings::StartPython()
    {
        return m_cliBindings->StartPython();
    }

    bool PythonBindings::PythonStarted()
    {
        return m_cliBindings->PythonStarted();
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::ExecuteWithLockErrorHandling(AZStd::function<void()> executionCallback)
    {
        if (!m_cliBindings->PythonStarted())
        {
            return AZ::Failure<AZStd::string>("Python is not initialized");
        }

        AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
        pybind11::gil_scoped_release release;
        pybind11::gil_scoped_acquire acquire;

        ClearErrorStrings();

        try
        {
            executionCallback();
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("PythonBindings", false, "Python exception %s", e.what());
            return AZ::Failure<AZStd::string>(e.what());
        }

        return AZ::Success();
    }

    bool PythonBindings::ExecuteWithLock(AZStd::function<void()> executionCallback)
    {
        return ExecuteWithLockErrorHandling(executionCallback).IsSuccess();
    }

    EngineInfo PythonBindings::EngineInfoFromPath(pybind11::handle enginePath)
    {
        EngineInfo engineInfo;
        try
        {
            pybind11::object engineData = m_cliBindings->GetEngineJson(enginePath);
            if (pybind11::isinstance<pybind11::dict>(engineData))
            {
                engineInfo.m_version = Py_To_String_Optional(engineData, "O3DEVersion", "0.0.0.0");
                engineInfo.m_name = Py_To_String_Optional(engineData, "engine_name", "O3DE");
                engineInfo.m_path = Py_To_String(enginePath);
            }

            pybind11::object o3deData = m_cliBindings->LoadO3deManifest();
            if (pybind11::isinstance<pybind11::dict>(o3deData))
            {
                engineInfo.m_defaultGemsFolder =
                    Py_To_String_Optional(o3deData, "default_gems_folder", Py_To_String(m_cliBindings->GetGemsFolder()));

                engineInfo.m_defaultProjectsFolder =
                    Py_To_String_Optional(o3deData, "default_projects_folder", Py_To_String(m_cliBindings->GetProjectsFolder()));

                engineInfo.m_defaultRestrictedFolder =
                    Py_To_String_Optional(o3deData, "default_restricted_folder", Py_To_String(m_cliBindings->GetRestrictedFolder()));

                engineInfo.m_defaultTemplatesFolder =
                    Py_To_String_Optional(o3deData, "default_templates_folder", Py_To_String(m_cliBindings->GetTemplatesFolder()));

                engineInfo.m_thirdPartyPath =
                    Py_To_String_Optional(o3deData, "default_third_party_folder", Py_To_String(m_cliBindings->GetThirdPartyFolder()));
            }

            // check if engine path is registered
            pybind11::object allEngines = m_cliBindings->GetManifestEngines();
            if (pybind11::isinstance<pybind11::list>(allEngines))
            {
                const AZ::IO::FixedMaxPath enginePathFixed(Py_To_String(enginePath));
                for (auto engine : allEngines)
                {
                    AZ::IO::FixedMaxPath otherEnginePath(Py_To_String(engine));
                    if (otherEnginePath.Compare(enginePathFixed) == 0)
                    {
                        engineInfo.m_registered = true;
                        break;
                    }
                }
            }
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("PythonBindings", false, "Failed to get EngineInfo from %s", Py_To_String(enginePath));
        }
        return engineInfo;
    }

    AZ::Outcome<EngineInfo> PythonBindings::GetEngineInfo()
    {
        EngineInfo engineInfo;

        bool result = ExecuteWithLock(
            [&]
        {
            auto enginePath = m_cliBindings->GetThisEnginePath();
            engineInfo = EngineInfoFromPath(enginePath);
        });

        if (!result || !engineInfo.IsValid())
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(engineInfo));
        }
    }

    AZ::Outcome<EngineInfo> PythonBindings::GetEngineInfo(const QString& engineName)
    {
        EngineInfo engineInfo;
        bool result = ExecuteWithLock([&] {
            auto enginePathResult = m_cliBindings->GetRegisterEnginePath(QString_To_Py_String(engineName));

            // if a valid registered object is not found None is returned
            if (!pybind11::isinstance<pybind11::none>(enginePathResult))
            {
                engineInfo = EngineInfoFromPath(enginePathResult);

                // it is possible an engine is registered in o3de_manifest.json but the engine.json is
                // missing or corrupt in which case we do not consider it a registered engine
            }
        });

        if (!result || !engineInfo.IsValid())
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(engineInfo));
        }
    }

    IPythonBindings::DetailedOutcome PythonBindings::SetEngineInfo(const EngineInfo& engineInfo, bool force)
    {
        int registrationExitCode = -1;
        bool pythonSuccess = ExecuteWithLock([&] {

            EngineInfo currentEngine = EngineInfoFromPath(QString_To_Py_Path(engineInfo.m_path));

            // be kind to source control and avoid needlessly updating engine.json
            if (currentEngine.IsValid() &&
                (currentEngine.m_name.compare(engineInfo.m_name) != 0 || currentEngine.m_version.compare(engineInfo.m_version) != 0))
            {
                int enginePropsResult = m_cliBindings->EditEngine(
                    QString_To_Py_Path(engineInfo.m_path),
                    QString_To_Py_String(engineInfo.m_name),
                    QString_To_Py_String(engineInfo.m_version)
                );

                if (enginePropsResult != 0)
                {
                    // do not proceed with registration
                    return;
                }
            }

            registrationExitCode = m_cliBindings->RegisterEngine(
                QString_To_Py_Path(engineInfo.m_path),
                QString_To_Py_Path(engineInfo.m_defaultProjectsFolder),
                QString_To_Py_Path(engineInfo.m_defaultGemsFolder),
                QString_To_Py_Path(engineInfo.m_defaultTemplatesFolder),
                QString_To_Py_Path(engineInfo.m_thirdPartyPath),
                force
            );
        });

        if (pythonSuccess && registrationExitCode != 0)
        {
            return AZ::Success();
        }

        return AZ::Failure<ErrorPair>(GetErrorPair());
    }

    AZ::Outcome<GemInfo> PythonBindings::GetGemInfo(const QString& path, const QString& projectPath)
    {
        GemInfo gemInfo = GemInfoFromPath(QString_To_Py_String(path), QString_To_Py_Path(projectPath));
        if (gemInfo.IsValid())
        {
            return AZ::Success(AZStd::move(gemInfo));
        }
        else
        {
            return AZ::Failure();
        }
    }

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetEngineGemInfos()
    {
        QVector<GemInfo> gems;

        auto result = ExecuteWithLockErrorHandling([&]
        {
            for (auto path : m_cliBindings->GetEngineGems())
            {
                gems.push_back(GemInfoFromPath(path, pybind11::none()));
            }
        });
        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }

        std::sort(gems.begin(), gems.end());
        return AZ::Success(AZStd::move(gems));
    }

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetAllGemInfos(const QString& projectPath)
    {
        QVector<GemInfo> gems;

        auto result = ExecuteWithLockErrorHandling([&]
        {
            pybind11::str pyProjectPath = QString_To_Py_Path(projectPath);
            for (auto path : m_cliBindings->GetAllGems((pyProjectPath)))
            {
                GemInfo gemInfo = GemInfoFromPath(path, pyProjectPath);
                // Mark as downloaded because this gem was registered with an existing directory
                gemInfo.m_downloadStatus = GemInfo::DownloadStatus::Downloaded;

                gems.push_back(AZStd::move(gemInfo));
            }
        });
        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }

        std::sort(gems.begin(), gems.end());
        return AZ::Success(AZStd::move(gems));
    }

    AZ::Outcome<QVector<AZStd::string>, AZStd::string> PythonBindings::GetEnabledGemNames(const QString& projectPath)
    {
        // Retrieve the path to the cmake file that lists the enabled gems.
        pybind11::object enabledGemsFilename;
        auto result = ExecuteWithLockErrorHandling([&]
        {
            enabledGemsFilename = m_cliBindings->GetGemsCmakeFilePath(QString_To_Py_Path(projectPath));
        });
        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }

        // Retrieve the actual list of names from the cmake file.
        QVector<AZStd::string> gemNames;
        result = ExecuteWithLockErrorHandling([&]
        {
            for (auto gemName : m_cliBindings->GetEnabledGemNames(enabledGemsFilename))
            {
                gemNames.push_back(Py_To_String(gemName));
            }
        });
        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }

        return AZ::Success(AZStd::move(gemNames));
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::GemRegistration(const QString& gemPath, const QString& projectPath, bool remove)
    {
        int registrationExitCode = -1;
        auto result = ExecuteWithLockErrorHandling(
            [&]
        {
            pybind11::object externalProjectPath = projectPath.isEmpty() ? pybind11::none() : QString_To_Py_Path(projectPath);
            registrationExitCode = m_cliBindings->RegisterGem(QString_To_Py_Path(gemPath), externalProjectPath, remove);
        });

        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }
        else if (registrationExitCode != 0)
        {
            return AZ::Failure<AZStd::string>(AZStd::string::format(
                "Failed to %s gem path %s", remove ? "unregister" : "register", gemPath.toUtf8().constData()));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::RegisterGem(const QString& gemPath, const QString& projectPath)
    {
        return GemRegistration(gemPath, projectPath);
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::UnregisterGem(const QString& gemPath, const QString& projectPath)
    {
        return GemRegistration(gemPath, projectPath, /*remove*/true);
    }

    bool PythonBindings::AddProject(const QString& path)
    {
        int registrationExitCode = -1;
        bool result = ExecuteWithLock(
            [&]
        {
            registrationExitCode = m_cliBindings->RegisterProject(QString_To_Py_Path(path), /*remove*/ false);
        });

        return result && registrationExitCode == 0;
    }

    bool PythonBindings::RemoveProject(const QString& path)
    {
        int registrationExitCode = -1;
        bool result = ExecuteWithLock(
            [&]
        {
            registrationExitCode = m_cliBindings->RegisterProject(QString_To_Py_Path(path), /*remove*/ true);
        });

        return result && registrationExitCode == 0;
    }

    AZ::Outcome<ProjectInfo> PythonBindings::CreateProject(const QString& projectTemplatePath, const ProjectInfo& projectInfo)
    {
        ProjectInfo createdProjectInfo;
        bool result = ExecuteWithLock([&]
        {
            pybind11::object projectPath = QString_To_Py_Path(projectInfo.m_path);
            int createProjectResult = m_cliBindings->CreateProject(projectPath, QString_To_Py_String(projectInfo.m_projectName), QString_To_Py_Path(projectTemplatePath));
            if (createProjectResult == 0)
            {
                createdProjectInfo = ProjectInfoFromPath(projectPath);
            }
        });

        if (!result || !createdProjectInfo.IsValid())
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(createdProjectInfo));
        }
    }

    AZ::Outcome<ProjectInfo> PythonBindings::GetProject(const QString& path)
    {
        ProjectInfo projectInfo = ProjectInfoFromPath(QString_To_Py_Path(path));
        if (projectInfo.IsValid())
        {
            return AZ::Success(AZStd::move(projectInfo));
        }
        else
        {
            return AZ::Failure();
        }
    }

    GemInfo PythonBindings::GemInfoFromPath(pybind11::handle path, pybind11::handle pyProjectPath)
    {
        GemInfo gemInfo;
        gemInfo.m_path = Py_To_String(path);
        gemInfo.m_directoryLink = gemInfo.m_path;

        pybind11::object data = m_cliBindings->GetGemJson(path, pyProjectPath);
        if (pybind11::isinstance<pybind11::dict>(data))
        {
            try
            {
                // required
                gemInfo.m_name = Py_To_String(data["gem_name"]);

                // optional
                gemInfo.m_displayName = Py_To_String_Optional(data, "display_name", gemInfo.m_name);
                gemInfo.m_summary = Py_To_String_Optional(data, "summary", "");
                gemInfo.m_version = Py_To_String_Optional(data, "version", gemInfo.m_version);
                gemInfo.m_lastUpdatedDate = Py_To_String_Optional(data, "last_updated", gemInfo.m_lastUpdatedDate);
                gemInfo.m_binarySizeInKB = Py_To_Int_Optional(data, "binary_size", gemInfo.m_binarySizeInKB);
                gemInfo.m_requirement = Py_To_String_Optional(data, "requirements", "");
                gemInfo.m_creator = Py_To_String_Optional(data, "origin", "");
                gemInfo.m_documentationLink = Py_To_String_Optional(data, "documentation_url", "");
                gemInfo.m_licenseText = Py_To_String_Optional(data, "license", "Unspecified License");
                gemInfo.m_licenseLink = Py_To_String_Optional(data, "license_url", "");
                gemInfo.m_repoUri = Py_To_String_Optional(data, "repo_uri", "");

                if (gemInfo.m_creator.contains("Open 3D Engine"))
                {
                    gemInfo.m_gemOrigin = GemInfo::GemOrigin::Open3DEngine;
                }
                else if (gemInfo.m_creator.contains("Amazon Web Services"))
                {
                    gemInfo.m_gemOrigin = GemInfo::GemOrigin::Local;
                }
                else if (data.contains("origin"))
                {
                    gemInfo.m_gemOrigin = GemInfo::GemOrigin::Remote;
                }
                // If no origin was provided this cannot be remote and would be specified if O3DE so it should be local
                else
                {
                    gemInfo.m_gemOrigin = GemInfo::GemOrigin::Local;
                }

                // As long Base Open3DEngine gems are installed before first startup non-remote gems will be downloaded
                if (gemInfo.m_gemOrigin != GemInfo::GemOrigin::Remote)
                {
                    gemInfo.m_downloadStatus = GemInfo::DownloadStatus::Downloaded;
                }

                if (data.contains("user_tags"))
                {
                    for (auto tag : data["user_tags"])
                    {
                        gemInfo.m_features.push_back(Py_To_String(tag));
                    }
                }

                if (data.contains("dependencies"))
                {
                    for (auto dependency : data["dependencies"])
                    {
                        gemInfo.m_dependencies.push_back(Py_To_String(dependency));
                    }
                }

                QString gemType = Py_To_String_Optional(data, "type", "");
                if (gemType == "Asset")
                {
                    gemInfo.m_types |= GemInfo::Type::Asset;
                }
                if (gemType == "Code")
                {
                    gemInfo.m_types |= GemInfo::Type::Code;
                }
                if (gemType == "Tool")
                {
                    gemInfo.m_types |= GemInfo::Type::Tool;
                }
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Warning("PythonBindings", false, "Failed to get GemInfo for gem %s", Py_To_String(path));
            }
        }

        return gemInfo;
    }

    ProjectInfo PythonBindings::ProjectInfoFromPath(pybind11::handle path)
    {
        ProjectInfo projectInfo;
        projectInfo.m_path = Py_To_String(path);
        projectInfo.m_needsBuild = false;

        pybind11::object projectData = m_cliBindings->GetProjectJson(path);
        if (pybind11::isinstance<pybind11::dict>(projectData))
        {
            try
            {
                projectInfo.m_projectName = Py_To_String(projectData["project_name"]);
                projectInfo.m_displayName = Py_To_String_Optional(projectData, "display_name", projectInfo.m_projectName);
                projectInfo.m_id = Py_To_String_Optional(projectData, "project_id", projectInfo.m_id);
                projectInfo.m_origin = Py_To_String_Optional(projectData, "origin", projectInfo.m_origin);
                projectInfo.m_summary = Py_To_String_Optional(projectData, "summary", projectInfo.m_summary);
                projectInfo.m_iconPath = Py_To_String_Optional(projectData, "icon", ProjectPreviewImagePath);
                if (projectData.contains("user_tags"))
                {
                    for (auto tag : projectData["user_tags"])
                    {
                        projectInfo.m_userTags.append(Py_To_String(tag));
                    }
                }
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Warning("PythonBindings", false, "Failed to get ProjectInfo for project %s", Py_To_String(path));
            }
        }

        return projectInfo;
    }

    AZ::Outcome<QVector<ProjectInfo>> PythonBindings::GetProjects()
    {
        QVector<ProjectInfo> projects;

        bool result = ExecuteWithLock([&] {
            // external projects
            for (auto path : m_cliBindings->GetManifestProjects())
            {
                projects.push_back(ProjectInfoFromPath(path));
            }

            // projects from the engine
            for (auto path : m_cliBindings->GetEngineProjects())
            {
                projects.push_back(ProjectInfoFromPath(path));
            }
        });

        if (!result)
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(projects));
        }
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::AddGemToProject(const QString& gemPath, const QString& projectPath)
    {
        return ExecuteWithLockErrorHandling([&]
        {
            m_cliBindings->EnableProjectGem(QString_To_Py_Path(gemPath), QString_To_Py_Path(projectPath));
        });
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::RemoveGemFromProject(const QString& gemPath, const QString& projectPath)
    {
        return ExecuteWithLockErrorHandling([&]
        {
            m_cliBindings->DisableProjectGem(QString_To_Py_Path(gemPath), QString_To_Py_Path(projectPath));
        });
    }

    bool PythonBindings::RemoveInvalidProjects()
    {
        int removalResult = -1;
        bool result = ExecuteWithLock(
            [&]
            {
                removalResult = m_cliBindings->RemoveInvalidProjects();
            });

        return result && removalResult == 0;
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::UpdateProject(const ProjectInfo& projectInfo)
    {
        int updateProjectExitCode = -1;
        auto result = ExecuteWithLockErrorHandling([&]
            {
                std::list<std::string> newTags;
                for (const auto& i : projectInfo.m_userTags)
                {
                    newTags.push_back(i.toStdString());
                }

                updateProjectExitCode = m_cliBindings->EditProject(
                    QString_To_Py_Path(projectInfo.m_path),
                    QString_To_Py_String(projectInfo.m_projectName),
                    QString_To_Py_String(projectInfo.m_id),
                    QString_To_Py_String(projectInfo.m_origin),
                    QString_To_Py_String(projectInfo.m_displayName),
                    QString_To_Py_String(projectInfo.m_summary),
                    QString_To_Py_String(projectInfo.m_iconPath),
                    pybind11::list(pybind11::cast(newTags)));
            });

        if (!result.IsSuccess())
        {
            return result;
        }
        else if (updateProjectExitCode != 0)
        {
            return AZ::Failure<AZStd::string>("Failed to update project.");
        }

        return AZ::Success();
    }

    ProjectTemplateInfo PythonBindings::ProjectTemplateInfoFromPath(pybind11::handle path, pybind11::handle pyProjectPath)
    {
        ProjectTemplateInfo templateInfo;
        templateInfo.m_path = Py_To_String(path);

        pybind11::object data = m_cliBindings->GetTemplateJson(path, pyProjectPath);
        if (pybind11::isinstance<pybind11::dict>(data))
        {
            try
            {
                // required
                templateInfo.m_displayName = Py_To_String(data["display_name"]);
                templateInfo.m_name = Py_To_String(data["template_name"]);
                templateInfo.m_summary = Py_To_String(data["summary"]);

                // optional
                if (data.contains("canonical_tags"))
                {
                    for (auto tag : data["canonical_tags"])
                    {
                        templateInfo.m_canonicalTags.push_back(Py_To_String(tag));
                    }
                }
                if (data.contains("user_tags"))
                {
                    for (auto tag : data["user_tags"])
                    {
                        templateInfo.m_canonicalTags.push_back(Py_To_String(tag));
                    }
                }

                QString templateProjectPath = QDir(templateInfo.m_path).filePath("Template");
                auto enabledGemNames = GetEnabledGemNames(templateProjectPath);
                if (enabledGemNames)
                {
                    for (auto gem : enabledGemNames.GetValue())
                    {
                        // Exclude the template ${Name} placeholder for the list of included gems
                        // That Gem gets created with the project
                        if (!gem.contains("${Name}"))
                        {
                            templateInfo.m_includedGems.push_back(Py_To_String(gem.c_str()));
                        }
                    }
                }
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Warning("PythonBindings", false, "Failed to get ProjectTemplateInfo for %s", Py_To_String(path));
            }
        }

        return templateInfo;
    }

    AZ::Outcome<QVector<ProjectTemplateInfo>> PythonBindings::GetProjectTemplates(const QString& projectPath)
    {
        QVector<ProjectTemplateInfo> templates;

        bool result = ExecuteWithLock([&] {
            for (auto path : m_cliBindings->GetTemplates())
            {
                templates.push_back(ProjectTemplateInfoFromPath(path, QString_To_Py_Path(projectPath)));
            }
        });

        if (!result)
        {
            return AZ::Failure();
        }
        else
        {
            return AZ::Success(AZStd::move(templates));
        }
    }

    AZ::Outcome<void, AZStd::string> PythonBindings::RefreshGemRepo(const QString& repoUri)
    {
        int refreshExitCode = -1;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                pybind11::str pyUri = QString_To_Py_String(repoUri);
                refreshExitCode = m_cliBindings->RefreshRepo(pyUri);
            });

        if (!result.IsSuccess())
        {
            return result;
        }
        else if (refreshExitCode != 0)
        {
            return AZ::Failure<AZStd::string>("Failed to refresh repo.");
        }

        return AZ::Success();
    }

    bool PythonBindings::RefreshAllGemRepos()
    {
        int refreshExitCode = -1;
        bool result = ExecuteWithLock(
            [&]
            {
                refreshExitCode = m_cliBindings->RefreshRepos();
            });

        return result && refreshExitCode == 0;
    }

    IPythonBindings::DetailedOutcome PythonBindings::AddGemRepo(const QString& repoUri)
    {
        int registrationExitCode = -1;
        bool result = ExecuteWithLock(
            [&]
            {
                pybind11::str pyUri = QString_To_Py_String(repoUri);
                registrationExitCode = m_cliBindings->RegisterRepo(pyUri, /*remove*/ false);
            });

        if (!result || registrationExitCode != 0)
        {
            return AZ::Failure<IPythonBindings::ErrorPair>(GetErrorPair());
        }

        return AZ::Success();
    }

    bool PythonBindings::RemoveGemRepo(const QString& repoUri)
    {
        int registrationExitCode = -1;
        bool result = ExecuteWithLock(
            [&]
            {
                pybind11::str pyUri = QString_To_Py_String(repoUri);
                registrationExitCode = m_cliBindings->RegisterRepo(pyUri, /*remove*/ true);
            });

        return result && registrationExitCode == 0;
    }

    GemRepoInfo PythonBindings::GetGemRepoInfo(pybind11::handle repoUri)
    {
        GemRepoInfo gemRepoInfo;
        gemRepoInfo.m_repoUri = Py_To_String(repoUri);

        pybind11::object data = m_cliBindings->GetRepoJson(repoUri);
        if (pybind11::isinstance<pybind11::dict>(data))
        {
            try
            {
                // required
                gemRepoInfo.m_repoUri = Py_To_String(data["repo_uri"]);
                gemRepoInfo.m_name = Py_To_String(data["repo_name"]);
                gemRepoInfo.m_creator = Py_To_String(data["origin"]);

                // optional
                gemRepoInfo.m_summary = Py_To_String_Optional(data, "summary", "No summary provided.");
                gemRepoInfo.m_additionalInfo = Py_To_String_Optional(data, "additional_info", "");

                pybind11::object repoPath = m_cliBindings->GetRepoPath(repoUri);
                gemRepoInfo.m_path = gemRepoInfo.m_directoryLink = Py_To_String(repoPath);

                QString lastUpdated = Py_To_String_Optional(data, "last_updated", "");
                gemRepoInfo.m_lastUpdated = QDateTime::fromString(lastUpdated, RepoTimeFormat);

                if (data.contains("enabled"))
                {
                    gemRepoInfo.m_isEnabled = data["enabled"].cast<bool>();
                }
                else
                {
                    gemRepoInfo.m_isEnabled = false;
                }

                if (data.contains("gems"))
                {
                    for (auto gemPath : data["gems"])
                    {
                        gemRepoInfo.m_includedGemUris.push_back(Py_To_String(gemPath));
                    }
                }
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                AZ_Warning("PythonBindings", false, "Failed to get GemRepoInfo for repo %s", Py_To_String(repoUri));
            }
        }

        return gemRepoInfo;
    }

    AZ::Outcome<QVector<GemRepoInfo>, AZStd::string> PythonBindings::GetAllGemRepoInfos()
    {
        QVector<GemRepoInfo> gemRepos;

        auto result = ExecuteWithLockErrorHandling(
            [&]
            {
                for (auto repoUri : m_cliBindings->GetReposUris())
                {
                    gemRepos.push_back(GetGemRepoInfo(repoUri));
                }
            });
        if (!result.IsSuccess())
        {
            return AZ::Failure<AZStd::string>(result.GetError().c_str());
        }

        std::sort(gemRepos.begin(), gemRepos.end());
        return AZ::Success(AZStd::move(gemRepos));
    }

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetGemInfosForRepo(const QString& repoUri)
    {
        QVector<GemInfo> gemInfos;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                auto pyUri = QString_To_Py_String(repoUri);
                pybind11::object gemPaths = m_cliBindings->GetCachedGemJsonPaths(pyUri);

                if (pybind11::isinstance<pybind11::set>(gemPaths))
                {
                    for (auto path : gemPaths)
                    {
                        GemInfo gemInfo = GemInfoFromPath(path, pybind11::none());
                        gemInfo.m_downloadStatus = GemInfo::DownloadStatus::NotDownloaded;
                        gemInfos.push_back(gemInfo);
                    }
                }
            });

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError());
        }

        return AZ::Success(AZStd::move(gemInfos));
    }

    AZ::Outcome<QVector<GemInfo>, AZStd::string> PythonBindings::GetGemInfosForAllRepos()
    {
        QVector<GemInfo> gemInfos;
        AZ::Outcome<void, AZStd::string> result = ExecuteWithLockErrorHandling(
            [&]
            {
                pybind11::object gemPaths = m_cliBindings->GetAllCachedGemJsonPaths();

                if (pybind11::isinstance<pybind11::set>(gemPaths))
                {
                    for (auto path : gemPaths)
                    {
                        GemInfo gemInfo = GemInfoFromPath(path, pybind11::none());
                        gemInfo.m_downloadStatus = GemInfo::DownloadStatus::NotDownloaded;
                        gemInfos.push_back(gemInfo);
                    }
                }
            });

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError());
        }

        return AZ::Success(AZStd::move(gemInfos));
    }

    IPythonBindings::DetailedOutcome  PythonBindings::DownloadGem(
        const QString& gemName, std::function<void(int, int)> gemProgressCallback, bool force)
    {
        // This process is currently limited to download a single gem at a time.
        int downloadExitCode = -1;

        m_requestCancelDownload = false;
        auto result = ExecuteWithLockErrorHandling(
            [&]
            {
                downloadExitCode = m_cliBindings->DownloadGem(
                    QString_To_Py_String(gemName), // gem name
                    pybind11::cpp_function(
                        [this, gemProgressCallback](int bytesDownloaded, int totalBytes)
                        {
                            gemProgressCallback(bytesDownloaded, totalBytes);

                            return m_requestCancelDownload;
                        }), // Callback for download progress and cancelling
                    force);
            });


        if (!result.IsSuccess())
        {
            IPythonBindings::ErrorPair pythonRunError(result.GetError(), result.GetError());
            return AZ::Failure<IPythonBindings::ErrorPair>(AZStd::move(pythonRunError));
        }
        else if (downloadExitCode != 0)
        {
            return AZ::Failure<IPythonBindings::ErrorPair>(GetErrorPair());
        }

        return AZ::Success();
    }

    void PythonBindings::CancelDownload()
    {
        m_requestCancelDownload = true;
    }

    bool PythonBindings::IsGemUpdateAvaliable(const QString& gemName, const QString& lastUpdated)
    {
        int updateAvaliableExitCode = -1;
        bool result = ExecuteWithLock(
            [&]
            {
                pybind11::str pyGemName = QString_To_Py_String(gemName);
                pybind11::str pyLastUpdated = QString_To_Py_String(lastUpdated);
                updateAvaliableExitCode = m_cliBindings->IsGemUpdateAvaliable(pyGemName, pyLastUpdated);
            });

        return result && updateAvaliableExitCode == 0;
    }

    IPythonBindings::ErrorPair PythonBindings::GetErrorPair()
    {
        size_t errorSize = m_pythonErrorStrings.size();
        if (errorSize > 0)
        {
            AZStd::string detailedString =
                errorSize == 1 ? "" : AZStd::accumulate(m_pythonErrorStrings.begin(), m_pythonErrorStrings.end(), AZStd::string(""));

            return IPythonBindings::ErrorPair(m_pythonErrorStrings.front(), detailedString);
        }
        // If no error was found
        else
        {
            return IPythonBindings::ErrorPair(AZStd::string("Unknown Error"), AZStd::string(""));
        }
    }

    void PythonBindings::AddErrorString(AZStd::string errorString)
    {
        m_pythonErrorStrings.push_back(errorString);
    }

    void PythonBindings::ClearErrorStrings()
    {
        m_pythonErrorStrings.clear();
    }
}
