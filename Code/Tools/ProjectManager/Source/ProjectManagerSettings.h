/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QStringList>
#include <ProjectInfo.h>
#endif

namespace O3DE::ProjectManager
{
    namespace PMSettings
    {
        static constexpr char ProjectManagerKeyPrefix[] = "/O3DE/ProjectManager";

        bool SaveProjectManagerSettings();
        bool GetProjectManagerKey(QString& result, const QString& settingsKey);
        bool GetProjectManagerKey(bool& result, const QString& settingsKey);
        bool SetProjectManagerKey(const QString& settingsKey, const QString& settingsValue, bool saveToDisk = true);
        bool SetProjectManagerKey(const QString& settingsKey, bool settingsValue, bool saveToDisk = true);
        bool RemoveProjectManagerKey(const QString& settingsKey, bool saveToDisk = true);
        bool CopyProjectManagerKeyString(
            const QString& settingsKeyOrig, const QString& settingsKeyDest, bool removeOrig = false, bool saveToDisk = true);

        QString GetProjectKey(const ProjectInfo& projectInfo);
        QString GetExternalLinkWarningKey();

        bool GetProjectBuiltSuccessfully(bool& result, const ProjectInfo& projectInfo);
        bool SetProjectBuiltSuccessfully(const ProjectInfo& projectInfo, bool successfullyBuilt, bool saveToDisk = true);
    } // namespace PMSettings
} // namespace O3DE::ProjectManager
