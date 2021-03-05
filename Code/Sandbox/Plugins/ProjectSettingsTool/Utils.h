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

#include <AzCore/std/string/string.h>

#include <QString>
#include <QValidator>


namespace ProjectSettingsTool
{
    void* ConvertFunctorToVoid(AZStd::pair<QValidator::State, const QString>(*func)(const QString&));
    AZStd::string GetDevRoot();
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
