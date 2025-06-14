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
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

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

    void AddImageAssetDependenciesToProduct(
        const MaterialPropertiesLayout* propertyLayout,
        const AZStd::vector<MaterialPropertyValue>& propertyValues,
        AssetBuilderSDK::JobProduct& product)
    {
        if (!propertyLayout)
        {
            return;
        }

        for (size_t propertyIndex = 0; propertyIndex < propertyLayout->GetPropertyCount(); ++propertyIndex)
        {
            auto descriptor = propertyLayout->GetPropertyDescriptor(AZ::RPI::MaterialPropertyIndex{ propertyIndex });
            if (descriptor->GetDataType() == MaterialPropertyDataType::Image)
            {
                if (propertyIndex >= propertyValues.size())
                {
                    continue; // invalid index, but let's not crash!
                }

                auto propertyValue = propertyValues[propertyIndex];
                if (propertyValue.IsValid())
                {
                    AZ::Data::Asset<ImageAsset> imageAsset = propertyValue.GetValue<AZ::Data::Asset<ImageAsset>>();
                    if (imageAsset.GetId().IsValid())
                    {
                        // preload images (set to NoLoad to avoid this)
                        auto loadFlags = AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::PreLoad);
                        product.m_dependencies.push_back(AssetBuilderSDK::ProductDependency(imageAsset.GetId(), loadFlags));
                    }
                }
            }
        }
    }

    void AddImageAssetDependenciesToProduct(const AZ::RPI::MaterialAsset* materialAsset, AssetBuilderSDK::JobProduct& product)
    {
        if (!materialAsset)
        {
            return;
        }

        AddImageAssetDependenciesToProduct(materialAsset->GetMaterialPropertiesLayout(), materialAsset->GetPropertyValues(), product);
        
    }

    void AddImageAssetDependenciesToProduct(const AZ::RPI::MaterialTypeAsset* materialTypeAsset, AssetBuilderSDK::JobProduct& product)
    {
        if (!materialTypeAsset)
        {
            return;
        }

        AddImageAssetDependenciesToProduct(materialTypeAsset->GetMaterialPropertiesLayout(), materialTypeAsset->GetDefaultPropertyValues(), product);
    }

    void AddFingerprintForDependency(const AZStd::string& path, AssetBuilderSDK::JobDescriptor& jobDescriptor)
    {
        jobDescriptor.m_additionalFingerprintInfo +=
            AZStd::string::format("|%u:%llu", (AZ::u32)AZ::Crc32(path), AZ::IO::SystemFile::ModificationTime(path.c_str()));
    }
} // namespace AZ::RPI::MaterialBuilderUtils
