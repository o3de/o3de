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
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
AZ_PUSH_DISABLE_WARNING(4127 4251 4800, "-Wunknown-warning-option")
#include <QStringList>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace AssetUtils
    {
        extern const char* AssetProcessorPlatformConfigFileName;
        extern const char* AssetProcessorGamePlatformConfigFileName;

        //! This struct stores gem related information
        struct GemInfo
        {
            AZ_CLASS_ALLOCATOR(GemInfo, AZ::SystemAllocator, 0);
            GemInfo(AZStd::string name, AZStd::string relativeFilePath, AZStd::string absoluteFilePath, AZStd::string identifier, bool isGameGem, bool assetOnlyGem);
            GemInfo() = default;
            AZStd::string m_gemName; ///< A friendly display name, not to be used for any pathing stuff.
            AZStd::string m_relativeFilePath; ///< Where the gem's folder is (relative to the gems search path(s))
            AZStd::string m_absoluteFilePath; ///< Where the gem's folder is (as an absolute path)
            AZStd::string m_identifier; ///< The UUID of the gem.

            bool m_isGameGem = false; //< True if its a 'game project' gem. Only one such gem can exist for any game project.
            bool m_assetOnly = false; ///< True if it is an asset only gems.

            static AZStd::string GetGemAssetFolder() { return AZStd::string("Assets"); }

        };

        //! Returns all the enabledPlatforms by reading the specified config files in order.  
        QStringList GetEnabledPlatforms(QStringList configFiles);

        //! Returns all the config files including the the platform and gems ones.
        //! Please note that config files are order dependent
        //! the root config file has the lowest priority.
        //! the project configuration file has the absolutely highest priority
        //! Also note that if the project has any "game project gems", then those will also be inserted last, 
        //! and thus have a higher priority than the root or non - project gems.
        //! Also note that the game project could be in a different location to the engine therefore we need the assetRoot param.
        QStringList GetConfigFiles(const char* root, const char* assetRoot, const char* gameName, bool addPlatformConfigs = true, bool addGemsConfigs = true);

        //! Returns a list of GemInfo of all the gems that are active for the for the specified game project. 
        //! Please note that the game project could be in a different location to the engine therefore we need the assetRoot param.
        bool GetGemsInfo(const char* root, const char* assetRoot, const char* gameName, AZStd::vector<GemInfo>& gemInfoList);

        //! A utility function which checks the given path starting at the root and updates the relative path to be the actual case correct path.
        //! For example, if you pass it "c:\lumberyard\dev" as the root and "editor\icons\whatever.ico" as the relative path.
        //! It may update relativePathFromRoot to be "Editor\Icons\Whatever.ico" if such a casing is the actual physical case on disk already.
        //! @param root a trusted already-case-correct path (will not be case corrected). If empty it will be set to appRoot.
        //! @param relativePathFromRoot a non-trusted (may be incorrect case) path relative to rootPath,
        //!        which will be normalized and updated to be correct casing.
        //! @return if such a file does NOT exist, it returns FALSE, else returns TRUE.
        //! @note A very expensive function!  Call sparingly.
        bool UpdateFilePathToCorrectCase(const QString& root, QString& relativePathFromRoot);
    } //namespace AssetUtils
} //namespace AzToolsFramework
