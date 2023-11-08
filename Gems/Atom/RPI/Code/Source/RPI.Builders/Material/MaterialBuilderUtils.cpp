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
                auto& jobDepedency = jobDescriptor.m_jobDependencyList.emplace_back();
                jobDepedency.m_type = AssetBuilderSDK::JobDependencyType::OrderOnce;
                jobDepedency.m_sourceFile.m_sourceFileDependencyPath =
                    AssetUtils::ResolvePathReference(originatingSourceFilePath, referencedSourceFilePath);
                jobDepedency.m_jobKey = "Image Compile: " + ext;
                jobDepedency.m_productSubIds = { 0 };
                AddFingerprintForDependency(jobDepedency.m_sourceFile.m_sourceFileDependencyPath, jobDescriptor);
            }
        }
    }

    void AddFingerprintForDependency(const AZStd::string& path, AssetBuilderSDK::JobDescriptor& jobDescriptor)
    {
        jobDescriptor.m_additionalFingerprintInfo +=
            AZStd::string::format("|%u:%llu", (AZ::u32)AZ::Crc32(path), AZ::IO::SystemFile::ModificationTime(path.c_str()));
    }
} // namespace AZ::RPI::MaterialBuilderUtils
