/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

#include <QString>
#include <QValidator>


namespace ProjectSettingsTool
{
    void* ConvertFunctorToVoid(AZStd::pair<QValidator::State, const QString>(*func)(const QString&));
    AZStd::string GetEngineRoot();
    AZStd::string GetProjectRoot();
    AZStd::string GetProjectName();

    // Open file dialogs for each file type and return the result
    // CurrentFile is where the dialog opens
    QString SelectXmlFromFileDialog(const QString& currentFile);
    QString SelectImageFromFileDialog(const QString& currentFile);

    enum class ImageGroup
    {
        AndroidIcons,
        AndroidLandscape,
        AndroidPortrait,
        IosIcons,
        IosLaunchScreens
    };

    AZStd::string GenDefaultImagePath(ImageGroup group, AZStd::string size);
} // namespace ProjectSettingsTool
