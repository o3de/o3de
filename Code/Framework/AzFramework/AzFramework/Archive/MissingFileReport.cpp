/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/regex.h>

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/FileTag/FileTag.h>
#include <AzFramework/FileTag/FileTagBus.h>
#include <AzFramework/Archive/MissingFileReport.h>
#include <AzFramework/Logging/MissingAssetNotificationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ::IO::Internal
{
    // Whether we should ignore this missing file
    static bool IsIgnored(const char* szPath);

    // Do not report missing LOD files if no CGF files depend on them
    // Do not report missing .cgfm files since they're not actually created and used in Open 3D Engine
    // This checking prevents our missing dependency scanner from having a lot of false positives on these files
    static bool IgnoreCGFDependencies(const char* szPath);

    void ReportFileMissingFromArchive(const char* szPath)
    {
        int reportLevel{};
        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console)
        {
            console->GetCvarValue("sys_report_files_not_found_in_paks", reportLevel);
        }
        if (!reportLevel)
        {
            return;
        }

        if (IsIgnored(szPath))
        {
            return;
        }

        AzFramework::MissingAssetNotificationBus::Broadcast(&AzFramework::MissingAssetNotificationBus::Events::FileMissing, szPath);

        const int LogMissingFileAccess = 1;
        const int WarnMissingFileAccess = 2;
        AZStd::string missingMessage(AZStd::string::format("Missing from bundle: %s", szPath));
        switch (reportLevel)
        {
        case LogMissingFileAccess:
            AZ_TracePrintf("Archive", missingMessage.c_str());
            break;
        case WarnMissingFileAccess:
            AZ_Warning("Archive", false, missingMessage.c_str());
            break;
        default:
            AZ_Error("Archive", false, missingMessage.c_str());
            break;
        }
    }

    bool IsIgnored(const char* szPath)
    {
        if (IgnoreCGFDependencies(szPath))
        {
            return true;
        }
        using namespace AzFramework::FileTag;
        AZStd::vector<AZStd::string> tags{
            FileTags[static_cast<unsigned int>(FileTagsIndex::Ignore)],
            FileTags[static_cast<unsigned int>(FileTagsIndex::ProductDependency)] };

        bool shouldIgnore = false;
        QueryFileTagsEventBus::EventResult(shouldIgnore, FileTagType::Exclude, &QueryFileTagsEventBus::Events::Match, szPath, tags);

        return shouldIgnore;
    }

    // This is a quick and simple solution to handle .cgfm and LOD files
    // We only report them as missing files if they are created by
    // Resource Compiler during the process for source CGF assets
    // A better solution could be to load only valid files that are actually needed
    bool IgnoreCGFDependencies(const char* szPath)
    {
        if (AZ::StringFunc::Path::IsExtension(szPath, "cgfm"))
        {
            // Ignore all the .cgfm files
            return true;
        }

        AZStd::smatch matches;
        const AZStd::regex lodRegex("@products@\\\\(.*)_lod[0-9]+(\\.cgfm?)");
        if (!AZStd::regex_match(szPath, matches, lodRegex) || matches.size() != 3)
        {
            // The current file is not a valid LOD file
            return false;
        }

        AZStd::string nonLodFileName = matches.str(1) + matches.str(2);
        AZ::Data::AssetId nonLodFileId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            nonLodFileId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
            nonLodFileName.c_str(), AZ::Data::s_invalidAssetType, false);
        if (!nonLodFileId.IsValid())
        {
            // The current LOD file is not generated from a valid CGF file
            return false;
        }

        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result = AZ::Failure<AZStd::string>("No response");
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequestBus::Events::GetDirectProductDependencies, nonLodFileId);
        if (!result.IsSuccess())
        {
            return false;
        }

        AZStd::vector<AZ::Data::ProductDependency> dependencies = result.TakeValue();
        for (const AZ::Data::ProductDependency& dependency : dependencies)
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, dependency.m_assetId);
            if (assetInfo.m_assetType == AZ::Data::s_invalidAssetType)
            {
                // This dependency has unresolved path. Cannot compare it with the current LOD file.
                continue;
            }
            AZStd::string dependencyRelativePath = assetInfo.m_relativePath;
            AZStd::replace(dependencyRelativePath.begin(), dependencyRelativePath.end(), AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);
            if (strstr(szPath, dependencyRelativePath.c_str()))
            {
                // The current LOD file is a product dependency of the source CGF file
                return false;
            }
        }

        return true;
    }
}
