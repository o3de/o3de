/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Utils.h"
#include "LastPathBus.h"

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Utils/Utils.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <QFileDialog>


namespace
{
    template<typename StringType>
    void ToUnixPath(StringType& path)
    {
        AZStd::replace(path.begin(), path.end(), '\\', '/');
    }

    template<typename StringType>
    StringType GetAbsoluteEngineRoot()
    {
        AZ::IO::FixedMaxPath engineRoot = AZ::Utils::GetEnginePath();

        if (engineRoot.empty())
        {
            return "";
        }

        StringType engineRootString(engineRoot.c_str());
        ToUnixPath(engineRootString);
        return engineRootString;
    }

    template<typename StringType>
    StringType GetAbsoluteProjectRoot()
    {
        AZ::IO::FixedMaxPath projectRoot = AZ::Utils::GetProjectPath();

        if (projectRoot.empty())
        {
            return "";
        }

        StringType projectRootString(projectRoot.c_str());
        ToUnixPath(projectRootString);
        return projectRootString;
    }

    template<typename StringType>
    StringType GetProjectName();

    template<>
    AZStd::string GetProjectName()
    {
        auto projectName = AZ::Utils::GetProjectName();
        return AZStd::string{projectName.c_str()};
    }

    template<>
    QString GetProjectName()
    {
        auto projectName = AZ::Utils::GetProjectName();
        return QString::fromUtf8(projectName.c_str(), aznumeric_cast<int>(projectName.size()));
    }
}

namespace ProjectSettingsTool
{
    void* ConvertFunctorToVoid(AZStd::pair<QValidator::State, const QString> (*func)(const QString&))
    {
        return reinterpret_cast<void*>(func);
    }

    AZStd::string GetEngineRoot()
    {
        return GetAbsoluteEngineRoot<AZStd::string>();
    }
    AZStd::string GetProjectRoot()
    {
        return GetAbsoluteProjectRoot<AZStd::string>();
    }

    AZStd::string GetProjectName()
    {
        return ::GetProjectName<AZStd::string>();
    }

    QString SelectXmlFromFileDialog(const QString& currentFile)
    {
        // The selected file must be relative to this path
        QString defaultPath = GetAbsoluteEngineRoot<QString>();
        QString startPath;

        // Choose the starting path for file dialog
        if (currentFile != "")
        {
            if (currentFile.contains(defaultPath))
            {
                startPath = currentFile;
            }
            else
            {
                startPath = defaultPath + currentFile;
            }
        }
        else
        {
            startPath = defaultPath;
        }

        QString pickedPath = QFileDialog::getOpenFileName(nullptr, QObject::tr("Select Override"),
            startPath, QObject::tr("Extensible Markup Language file (*.xml)"));
        ToUnixPath(pickedPath);

        // Remove the default relative path
        if (pickedPath.contains(defaultPath))
        {
            pickedPath = pickedPath.mid(defaultPath.length());
        }

        return pickedPath;
    }

    QString SelectImageFromFileDialog(const QString& currentFile)
    {
        QString defaultPath = QStringLiteral("%1Code%2/Resources/").arg(GetAbsoluteEngineRoot<QString>(), ::GetProjectName<QString>());

        QString startPath;

        // Choose the starting path for file dialog
        if (currentFile != "")
        {
            if (QDir::isAbsolutePath(currentFile))
            {
                startPath = currentFile;
            }
            else
            {
                startPath = defaultPath + currentFile;
            }
        }
        else
        {
            LastPathBus::BroadcastResult(
                startPath,
                &LastPathBus::Handler::GetLastImagePath);
        }

        QString pickedPath = QFileDialog::getOpenFileName(nullptr, QObject::tr("Select Image"),
            startPath, QObject::tr("Image file (*.png)"));
        ToUnixPath(pickedPath);

        if (!pickedPath.isEmpty())
        {
            LastPathBus::Broadcast(
                &LastPathBus::Handler::SetLastImagePath,
                pickedPath.left(pickedPath.lastIndexOf('/')));
        }

        // Remove the default relative path if it is used
        if (pickedPath.contains(defaultPath))
        {
            pickedPath = pickedPath.mid(defaultPath.length());
        }

        return pickedPath;
    }

    AZStd::string GenDefaultImagePath(ImageGroup group, AZStd::string size)
    {
        AZStd::string root;
        // Android
        if (group <= ImageGroup::AndroidPortrait)
        {
            root = GetEngineRoot() + "/Code/Tools/Android/ProjectBuilder/app_";
        }
        //Ios
        else
        {
            using AZ::IO::SystemFile;
            root = GetProjectRoot() + "/Gem/Resources/Platform/iOS/Images.xcassets/";
            if (!SystemFile::Exists(root.c_str()))
            {
                root = GetProjectRoot() + "/Gem/Resources/IOSLauncher/Images.xcassets/";
            }
        }

        AZStd::string groupStr;
        switch (group)
        {
        case ImageGroup::AndroidIcons:
            groupStr = "icon-";
            break;
        case ImageGroup::AndroidLandscape:
            groupStr = "splash-land-";
            break;
        case ImageGroup::AndroidPortrait:
            groupStr = "splash-port-";
            break;
        case ImageGroup::IosIcons:
            groupStr = "AppIcon.appiconset/";
            break;
        case ImageGroup::IosLaunchScreens:
            groupStr = "LaunchImage.launchimage/";
            break;
        default:
            AZ_Assert(false, "Unknown ImageGroup.");
            break;
        }

        return root + groupStr + size + ".png";
    }
} // namespace ProjectSettingsTool
