/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TranslationHelper.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/Gem/GemInfo.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace ScriptCanvasEditor::TranslationHelper
{
    AZStd::string SanitizeText(const AZStd::string& text)
    {
        AZStd::string result = text;
        AZ::StringFunc::Replace(result, "+", "");
        AZ::StringFunc::Replace(result, "-", "");
        AZ::StringFunc::Replace(result, "*", "");
        AZ::StringFunc::Replace(result, "/", "");
        AZ::StringFunc::Replace(result, "(", "");
        AZ::StringFunc::Replace(result, ")", "");
        AZ::StringFunc::Replace(result, "{", "");
        AZ::StringFunc::Replace(result, "}", "");
        AZ::StringFunc::Replace(result, ":", "");
        AZ::StringFunc::Replace(result, "<", "");
        AZ::StringFunc::Replace(result, ">", "");
        AZ::StringFunc::Replace(result, ",", "");
        AZ::StringFunc::Replace(result, ".", "");
        AZ::StringFunc::Replace(result, "=", "");
        AZ::StringFunc::Replace(result, "!", "");
        AZ::StringFunc::Strip(result, " ");
        return result;
    }

    AZStd::string SanitizeCustomNodeFileName(const AZStd::string& nodeName, const AZ::Uuid& nodeUuid)
    {
        AZStd::string sanitizedNodeName = SanitizeText(nodeName);
        AZ::Uuid::FixedString nodeUuidName = nodeUuid.ToFixedString(false);

        AZStd::string result = AZStd::string::format("%s_%s", sanitizedNodeName.c_str(), nodeUuidName.c_str());
        AZ::StringFunc::Path::Normalize(result);
        return result;
    }

    AZStd::string GetSafeTypeName(ScriptCanvas::Data::Type dataType)
    {
        if (!dataType.IsValid())
        {
            return "";
        }

        AZStd::string azType = dataType.GetAZType().ToString<AZStd::string>();

        GraphCanvas::TranslationKey key;
        key << "BehaviorType" << azType << "details";

        GraphCanvas::TranslationRequests::Details details;

        details.m_name = ScriptCanvas::Data::GetName(dataType);

        GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

        return details.m_name;
    }

    AZ::IO::Path GetGemAssetPath(const AZStd::string& gemName)
    {
        if (auto settingsRegistry = AZ::Interface<AZ::SettingsRegistryInterface>::Get(); settingsRegistry != nullptr)
        {
            AZ::IO::Path gemSourceAssetDirectories;
            AZStd::vector<AzFramework::GemInfo> gemInfos;
            if (AzFramework::GetGemsInfo(gemInfos, *settingsRegistry))
            {
                auto FindGemByName = [gemName](const AzFramework::GemInfo& gemInfo)
                {
                    return gemInfo.m_gemName == gemName;
                };

                // Gather unique list of Gem Paths from the Settings Registry
                auto foundIt = AZStd::find_if(gemInfos.begin(), gemInfos.end(), FindGemByName);
                if (foundIt != gemInfos.end())
                {
                    const AzFramework::GemInfo& gemInfo = *foundIt;
                    for (const AZ::IO::Path& absoluteSourcePath : gemInfo.m_absoluteSourcePaths)
                    {
                        gemSourceAssetDirectories = (absoluteSourcePath / gemInfo.GetGemAssetFolder());
                    }

                    return AZ::IO::Path(gemSourceAssetDirectories.c_str());
                }
            }
        }
        return AZ::IO::Path();
    }

    AZ::IO::Path GetTranslationDefaultFolderPath()
    {
        AZ::IO::Path folderPath = GetGemAssetPath("ScriptCanvas") / "TranslationAssets";
        return folderPath;
    }

    AZ::IO::Path GetTranslationFilePath(const AZStd::string& fileName)
    {
        // Check asset safe folders where all loaded translation files
        AZStd::vector<AZStd::string> scanFolders;
        bool success = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            success, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetSafeFolders, scanFolders);
        auto fileIO = AZ::IO::FileIOBase::GetInstance();

        if (fileIO && success && !fileName.empty())
        {
            AZStd::string fileNameWithExtension = AZStd::string::format("%s.names", fileName.c_str());
            for (const AZStd::string& assetSafeFolder : scanFolders)
            {
                AZStd::deque<AZStd::string> foldersToSearch;
                foldersToSearch.push_back(assetSafeFolder);
                AZ::IO::Result searchResult = AZ::IO::ResultCode::Success;

                AZStd::vector<AZStd::string> filesFound;
                while (foldersToSearch.size() && searchResult == AZ::IO::ResultCode::Success)
                {
                    AZStd::string folderName = foldersToSearch.front();
                    foldersToSearch.pop_front();

                    searchResult = fileIO->FindFiles(folderName.c_str(), "*",
                        [&](const char* path)
                        {
                            AZ::IO::Path currentPath = path;
                            if (fileIO->IsDirectory(path))
                            {
                                foldersToSearch.push_back(path);
                            }
                            else if (currentPath.HasFilename()
                                && strcmp(currentPath.Filename().LexicallyNormal().c_str(), fileNameWithExtension.c_str()) == 0)
                            {
                                filesFound.push_back(path);
                                foldersToSearch.clear();
                                return false;
                            }
                            return true;
                        });
                }
                if (filesFound.size() == 1)
                {
                    AZ::IO::FixedMaxPath resolvedPath("");
                    fileIO->ResolvePath(resolvedPath, AZ::IO::PathView(filesFound.front()));
                    return AZ::IO::Path(resolvedPath);
                }
            }
        }
        AZ_Warning("ScriptCanvas", false, AZStd::string::format("No matching translation file found. Please generate translation file first.").c_str());
        return AZ::IO::Path();
    }
} // namespace ScriptCanvasEditor::TranslationHelper
