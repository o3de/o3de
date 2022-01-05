/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ProjectInfo.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace O3DE::ProjectManager
{
    //! Interface used to interact with the settings functions
    class ISettings
    {
    public:
        AZ_RTTI(O3DE::ProjectManager::ISettings, "{95D87D95-0E04-462F-8B0B-ED15C0A9F090}");
        AZ_DISABLE_COPY_MOVE(ISettings);

        static constexpr char ProjectManagerKeyPrefix[] = "/O3DE/ProjectManager";
        static constexpr char ExternalLinkWarningKey[] = "/O3DE/ProjectManager/SkipExternalLinkWarning";
        static constexpr char ProjectsBuiltSuccessfullyKey[] = "/O3DE/ProjectManager/SuccessfulBuildPaths";

        ISettings() = default;
        virtual ~ISettings() = default;

        /**
         * Get the value for a string settings key
         * @param result Store string result in this variable
         * @param settingsKey The key to get the value in
         * @return true if all calls to settings registry were successful
         */
        virtual bool Get(QString& result, const QString& settingsKey) = 0;
        /**
         * Get the value for a bool settings key
         * @param result Store bool result in this variable
         * @param settingsKey The key to get the value in
         * @return true if all calls to settings registry were successful
         */
        virtual bool Get(bool& result, const QString& settingsKey) = 0;

        /**
         * Set the value for a string settings key
         * @param settingsKey The key to set the value in
         * @param settingsValue String value to set key to
         * @return true if all calls to settings registry were successful
         */
        virtual bool Set(const QString& settingsKey, const QString& settingsValue) = 0;
        /**
         * Set the value for a bool settings key
         * @param settingsKey The key to set the value in
         * @param settingsValue Bool value to set key to
         * @return true if all calls to settings registry were successful
         */
        virtual bool Set(const QString& settingsKey, bool settingsValue) = 0;

        /**
         * Remove settings key
         * @param settingsKey The key to remove
         * @return true if all calls to settings registry were successful
         */
        virtual bool Remove(const QString& settingsKey) = 0;

        /**
         * Copy the string settings value from one key to another
         * @param settingsKeyOrig The original key to copy from
         * @param settingsKeyDest The destination key to copy to
         * @param removeOrig(Optional) Delete the original key if true
         * @return true if all calls to settings registry were successful
         */
        virtual bool Copy(const QString& settingsKeyOrig, const QString& settingsKeyDest, bool removeOrig = false) = 0;

        /**
         * Generate prefix for project settings key
         * @param projectInfo Project for settings key
         * @return QString Prefix for project specific settings key
         */
        virtual QString GetProjectKey(const ProjectInfo& projectInfo) = 0;

        /**
         * Get the build status for a project
         * @param result Store bool build status in this variable
         * @param projectInfo Project to check built status for
         * @return true if all calls to settings registry were successful
         */
        virtual bool GetProjectBuiltSuccessfully(bool& result, const ProjectInfo& projectInfo) = 0;
        /**
         * Set the build status for a project
         * @param projectInfo Project to set built status for
         * @param successfullyBuilt Bool value to set build status to
         * @return true if all calls to settings registry were successful
         */
        virtual bool SetProjectBuiltSuccessfully(const ProjectInfo& projectInfo, bool successfullyBuilt) = 0;
    };

    using SettingsInterface = AZ::Interface<ISettings>;
} // namespace O3DE::ProjectManager
