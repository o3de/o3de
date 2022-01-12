/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Outcome/Outcome.h>

#include <EngineInfo.h>
#include <GemCatalog/GemInfo.h>
#include <ProjectInfo.h>
#include <ProjectTemplateInfo.h>
#include <GemRepo/GemRepoInfo.h>

namespace O3DE::ProjectManager
{
    //! Interface used to interact with the o3de cli python functions
    class IPythonBindings
    {
    public:
        AZ_RTTI(O3DE::ProjectManager::IPythonBindings, "{C2B72CA4-56A9-4601-A584-3B40E83AA17C}");
        AZ_DISABLE_COPY_MOVE(IPythonBindings);

        IPythonBindings() = default;
        virtual ~IPythonBindings() = default;

        //! First string in pair is general error, second is detailed
        using ErrorPair = AZStd::pair<AZStd::string, AZStd::string>;
        using DetailedOutcome = AZ::Outcome<void, ErrorPair>;

        /**
         * Get whether Python was started or not.  All Python functionality will fail if Python
         * failed to start. 
         * @return true if Python was started successfully, false on failure 
         */
        virtual bool PythonStarted() = 0;

        /**
         * Attempt to start Python. Normally, Python is started when the bindings are created,
         * but this method allows you to attempt to retry starting Python in case the configuration
         * has changed.
         * @return true if Python was started successfully, false on failure 
         */
        virtual bool StartPython() = 0;

        // Engine

        /**
         * Get info about the current engine 
         * @return an outcome with EngineInfo on success
         */
        virtual AZ::Outcome<EngineInfo> GetEngineInfo() = 0;

        /**
         * Get info about an engine by name
         * @param engineName The name of the engine to get info about
         * @return an outcome with EngineInfo on success
         */
        virtual AZ::Outcome<EngineInfo> GetEngineInfo(const QString& engineName) = 0;

        /**
         * Set info about the engine 
         * @param force True to force registration even if an engine with the same name is already registered
         * @param engineInfo an EngineInfo object 
         * @return a detailed error outcome on failure.
         */
        virtual DetailedOutcome SetEngineInfo(const EngineInfo& engineInfo, bool force = false) = 0;

        // Gems

        /**
         * Get info about a Gem.
         * @param path The absolute path to the Gem
         * @param projectPath (Optional) The absolute path to the Gem project
         * @return an outcome with GemInfo on success 
         */
        virtual AZ::Outcome<GemInfo> GetGemInfo(const QString& path, const QString& projectPath = {}) = 0;

        /**
         * Get all available gem infos. This concatenates gems registered by the engine and the project.
         * @param projectPath The absolute path to the project.
         * @return A list of gem infos.
         */
        virtual AZ::Outcome<QVector<GemInfo>, AZStd::string> GetAllGemInfos(const QString& projectPath) = 0;

        /**
        * Get engine gem infos.
        * @return A list of all registered gem infos.
        */
        virtual AZ::Outcome<QVector<GemInfo>, AZStd::string> GetEngineGemInfos() = 0;

        /**
         * Get a list of all enabled gem names for a given project.
         * @param[in] projectPath Absolute file path to the project.
         * @return A list of gem names of all the enabled gems for a given project or a error message on failure.
         */
        virtual AZ::Outcome<QVector<AZStd::string>, AZStd::string> GetEnabledGemNames(const QString& projectPath) = 0;

        /**
         * Registers the gem to the specified project, or to the o3de_manifest.json if no project path is given
         * @param gemPath the path to the gem
         * @param projectPath the path to the project. If empty, will register the external path in o3de_manifest.json
         * @return An outcome with the success flag as well as an error message in case of a failure.
         */
        virtual AZ::Outcome<void, AZStd::string> RegisterGem(const QString& gemPath, const QString& projectPath = {}) = 0;

        /**
         * Unregisters the gem from the specified project, or from the o3de_manifest.json if no project path is given
         * @param gemPath the path to the gem
         * @param projectPath the path to the project. If empty, will unregister the external path in o3de_manifest.json
         * @return An outcome with the success flag as well as an error message in case of a failure.
         */
        virtual AZ::Outcome<void, AZStd::string> UnregisterGem(const QString& gemPath, const QString& projectPath = {}) = 0;


        // Projects 

        /**
         * Create a project 
         * @param projectTemplatePath the path to the project template to use 
         * @param projectInfo the project info to use 
         * @return an outcome with ProjectInfo on success 
         */
        virtual AZ::Outcome<ProjectInfo> CreateProject(const QString& projectTemplatePath, const ProjectInfo& projectInfo) = 0;
        
        /**
         * Get info about a project 
         * @param path the absolute path to the project 
         * @return an outcome with ProjectInfo on success 
         */
        virtual AZ::Outcome<ProjectInfo> GetProject(const QString& path) = 0;

        /**
         * Get info about all known projects
         * @return an outcome with ProjectInfos on success 
         */
        virtual AZ::Outcome<QVector<ProjectInfo>> GetProjects() = 0;
        
        /**
         * Adds existing project on disk
         * @param path the absolute path to the project
         * @return true on success, false on failure
         */
        virtual bool AddProject(const QString& path) = 0;

        /**
         * Adds existing project on disk
         * @param path the absolute path to the project
         * @return true on success, false on failure
         */
        virtual bool RemoveProject(const QString& path) = 0;

        /**
         * Update a project
         * @param projectInfo the info to use to update the project 
         * @return true on success, false on failure
         */
        virtual AZ::Outcome<void, AZStd::string> UpdateProject(const ProjectInfo& projectInfo) = 0;

        /**
         * Add a gem to a project
         * @param gemPath the absolute path to the gem 
         * @param projectPath the absolute path to the project
         * @return An outcome with the success flag as well as an error message in case of a failure.
         */
        virtual AZ::Outcome<void, AZStd::string> AddGemToProject(const QString& gemPath, const QString& projectPath) = 0;

        /**
         * Remove gem to a project
         * @param gemPath the absolute path to the gem 
         * @param projectPath the absolute path to the project
         * @return An outcome with the success flag as well as an error message in case of a failure.
         */
        virtual AZ::Outcome<void, AZStd::string> RemoveGemFromProject(const QString& gemPath, const QString& projectPath) = 0;

        /**
         * Removes invalid projects from the manifest
         */
        virtual bool RemoveInvalidProjects() = 0;


        // Project Templates

        /**
         * Get info about all known project templates
         * @return an outcome with ProjectTemplateInfos on success 
         */
        virtual AZ::Outcome<QVector<ProjectTemplateInfo>> GetProjectTemplates(const QString& projectPath = {}) = 0;

        // Gem Repos

        /**
         * Refresh gem repo in the current engine.
         * @param repoUri the absolute filesystem path or url to the gem repo.
         * @return An outcome with the success flag as well as an error message in case of a failure.
         */
        virtual AZ::Outcome<void, AZStd::string> RefreshGemRepo(const QString& repoUri) = 0;

        /**
         * Refresh all gem repos in the current engine.
         * @return true on success, false on failure.
         */
        virtual bool RefreshAllGemRepos() = 0;

        /**
         * Registers this gem repo with the current engine.
         * @param repoUri the absolute filesystem path or url to the gem repo.
         * @return an outcome with a pair of string error and detailed messages on failure.
         */
        virtual DetailedOutcome AddGemRepo(const QString& repoUri) = 0;

        /**
         * Unregisters this gem repo with the current engine.
         * @param repoUri the absolute filesystem path or url to the gem repo.
         * @return true on success, false on failure.
         */
        virtual bool RemoveGemRepo(const QString& repoUri) = 0;

        /**
         * Get all available gem repo infos. Gathers all repos registered with the engine.
         * @return A list of gem repo infos.
         */
        virtual AZ::Outcome<QVector<GemRepoInfo>, AZStd::string> GetAllGemRepoInfos() = 0;

        /**
         * Gathers all gem infos from the provided repo
         * @param repoUri the absolute filesystem path or url to the gem repo.
         * @return A list of gem infos.
         */
        virtual AZ::Outcome<QVector<GemInfo>, AZStd::string> GetGemInfosForRepo(const QString& repoUri) = 0;

        /**
         * Gathers all gem infos for all gems registered from repos.
         * @return A list of gem infos.
         */
        virtual AZ::Outcome<QVector<GemInfo>, AZStd::string> GetGemInfosForAllRepos() = 0;

        /**
         * Downloads and registers a Gem.
         * @param gemName the name of the Gem to download.
         * @param gemProgressCallback a callback function that is called with an int percentage download value.
         * @param force should we forcibly overwrite the old version of the gem.
         * @return an outcome with a pair of string error and detailed messages on failure.
         */
        virtual DetailedOutcome DownloadGem(
            const QString& gemName, std::function<void(int, int)> gemProgressCallback, bool force = false) = 0;

        /**
         * Cancels the current download.
         */
        virtual void CancelDownload() = 0;

        /**
         * Checks if there is an update avaliable for a gem on a repo.
         * @param gemName the name of the gem to check.
         * @param lastUpdated last time the gem was update.
         * @return true if update is avaliable, false if not.
         */
        virtual bool IsGemUpdateAvaliable(const QString& gemName, const QString& lastUpdated) = 0;

        /**
         * Add an error string to be returned when the current python call is complete.
         * @param The error string to be displayed.
         */
        virtual void AddErrorString(AZStd::string errorString) = 0;

        /**
         * Clears the current list of error strings.
         */
        virtual void ClearErrorStrings() = 0;
    };

    using PythonBindingsInterface = AZ::Interface<IPythonBindings>;
} // namespace O3DE::ProjectManager
