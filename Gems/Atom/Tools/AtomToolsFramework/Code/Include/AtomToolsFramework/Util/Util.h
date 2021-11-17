/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/vector.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFileInfo>
#include <QString>
#include <QStringList>
AZ_POP_DISABLE_WARNING

class QImage;

namespace AtomToolsFramework
{
    template<typename T>
    T GetSettingOrDefault(AZStd::string_view path, const T& defaultValue)
    {
        T result;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return (settingsRegistry && settingsRegistry->Get(result, path)) ? result : defaultValue;
    }

    using LoadImageAsyncCallback = AZStd::function<void(const QImage&)>;
    void LoadImageAsync(const AZStd::string& path, LoadImageAsyncCallback callback);

    QFileInfo GetSaveFileInfo(const QString& initialPath);
    QFileInfo GetOpenFileInfo(const AZStd::vector<AZ::Data::AssetType>& assetTypes);
    QFileInfo GetUniqueFileInfo(const QString& initialPath);
    QFileInfo GetDuplicationFileInfo(const QString& initialPath);
    bool LaunchTool(const QString& baseName, const QString& extension, const QStringList& arguments);
} // namespace AtomToolsFramework
