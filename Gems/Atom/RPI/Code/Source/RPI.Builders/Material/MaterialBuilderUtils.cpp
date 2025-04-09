/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialBuilderUtils.h"
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ::RPI::MaterialBuilderUtils
{
    AssetBuilderSDK::JobDependency& AddJobDependency(
        AssetBuilderSDK::JobDescriptor& jobDescriptor,
        const AZStd::string& path,
        const AZStd::string& jobKey,
        const AZStd::string& platformId,
        const AZStd::vector<AZ::u32>& subIds,
        const bool updateFingerprint)
    {
        if (updateFingerprint)
        {
            AddFingerprintForDependency(path, jobDescriptor);
        }

        AssetBuilderSDK::JobDependency jobDependency(
            jobKey,
            platformId,
            AssetBuilderSDK::JobDependencyType::Order,
            AssetBuilderSDK::SourceFileDependency(
                path, AZ::Uuid{}, AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Absolute));
        jobDependency.m_productSubIds = subIds;
        return jobDescriptor.m_jobDependencyList.emplace_back(AZStd::move(jobDependency));
    }

    void AddPossibleImageDependencies(
        const AZStd::string& originatingSourceFilePath,
        const AZStd::string& referencedSourceFilePath,
        AssetBuilderSDK::JobDescriptor& jobDescriptor)
    {
        if (!referencedSourceFilePath.empty())
        {
            AZStd::string ext;
            AzFramework::StringFunc::Path::GetExtension(referencedSourceFilePath.c_str(), ext, false);
            AZStd::to_upper(ext.begin(), ext.end());

            if (!ext.empty())
            {
                auto& jobDependency = MaterialBuilderUtils::AddJobDependency(
                    jobDescriptor,
                    AssetUtils::ResolvePathReference(originatingSourceFilePath, referencedSourceFilePath),
                    "Image Compile: " + ext,
                    {},
                    { 0 });
                jobDependency.m_type = AssetBuilderSDK::JobDependencyType::OrderOnce;
            }
        }
    }

    void AddFingerprintForDependency(const AZStd::string& path, AssetBuilderSDK::JobDescriptor& jobDescriptor)
    {
        jobDescriptor.m_additionalFingerprintInfo +=
            AZStd::string::format("|%u:%llu", (AZ::u32)AZ::Crc32(path), AZ::IO::SystemFile::ModificationTime(path.c_str()));
    }
} // namespace AZ::RPI::MaterialBuilderUtils
