/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SettingsInterface.h>

#include <AzCore/std/containers/set.h>

namespace AZ
{
    class SettingsRegistryInterface;
}

namespace O3DE::ProjectManager
{
    class Settings
        : public SettingsInterface::Registrar
    {
    public:
        Settings(bool saveToDisk = true);

        bool Get(QString& result, const QString& settingsKey) override;
        bool Get(bool& result, const QString& settingsKey) override;
        bool Set(const QString& settingsKey, const QString& settingsValue) override;
        bool Set(const QString& settingsKey, bool settingsValue) override;
        bool Remove(const QString& settingsKey) override;
        bool Copy(const QString& settingsKeyOrig, const QString& settingsKeyDest, bool removeOrig = false) override;

        QString GetProjectKey(const ProjectInfo& projectInfo) override;

        bool GetProjectBuiltSuccessfully(bool& result, const ProjectInfo& projectInfo) override;
        bool SetProjectBuiltSuccessfully(const ProjectInfo& projectInfo, bool successfullyBuilt) override;

    private:
        void Save();
        void OnSettingsChanged();

        bool GetBuiltSuccessfullyPaths(AZStd::set<AZStd::string>& result);

        bool m_saveToDisk;
        AZ::SettingsRegistryInterface* m_settingsRegistry = nullptr;
    };
} // namespace O3DE::ProjectManager
