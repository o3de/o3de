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
        static constexpr char ProjectsBuiltSuccessfullyKey[] = "/O3DE/ProjectManager/SkipExternalLinkWarning";

        ISettings() = default;
        virtual ~ISettings() = default;

        /**
         * This checks if Settings is in a usable state
         * @return true Settings is ready to be used, false otherwise
         */
        virtual bool IsInitialized() = 0;

        /**
         * Get info about a Gem.
         * @param path The absolute path to the Gem
         * @param projectPath (Optional) The absolute path to the Gem project
         * @return an outcome with GemInfo on success
         */
        virtual bool Get(QString& result, const QString& settingsKey) = 0;
        virtual bool Get(bool& result, const QString& settingsKey) = 0;
        virtual bool Set(const QString& settingsKey, const QString& settingsValue) = 0;
        virtual bool Set(const QString& settingsKey, bool settingsValue) = 0;
        virtual bool Remove(const QString& settingsKey) = 0;
        virtual bool Copy(const QString& settingsKeyOrig, const QString& settingsKeyDest, bool removeOrig = false) = 0;

        virtual QString GetProjectKey(const ProjectInfo& projectInfo) = 0;

        virtual bool GetProjectBuiltSuccessfully(bool& result, const ProjectInfo& projectInfo) = 0;
        virtual bool SetProjectBuiltSuccessfully(const ProjectInfo& projectInfo, bool successfullyBuilt) = 0;
    };

    using SettingsInterface = AZ::Interface<ISettings>;
} // namespace O3DE::ProjectManager
