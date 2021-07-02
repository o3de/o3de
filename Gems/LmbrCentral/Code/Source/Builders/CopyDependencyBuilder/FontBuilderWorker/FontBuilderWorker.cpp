/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LmbrCentral_precompiled.h>
#include "FontBuilderWorker.h"

#include <AzFramework/StringFunc/StringFunc.h>
#include <LyShine/UiAssetTypes.h>

#include <QString>
#include <QFileInfo>
#include <QDir>

namespace CopyDependencyBuilder
{
    FontBuilderWorker::FontBuilderWorker()
        : XmlFormattedAssetBuilderWorker("Font", true, true)
    {
    }

    void FontBuilderWorker::RegisterBuilderWorker()
    {
        AssetBuilderSDK::AssetBuilderDesc fontBuilderDescriptor;
        fontBuilderDescriptor.m_name = "FontBuilderWorker";
        fontBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.font", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        fontBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.fontfamily", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        fontBuilderDescriptor.m_busId = azrtti_typeid<FontBuilderWorker>();
        fontBuilderDescriptor.m_version = 2;
        fontBuilderDescriptor.m_createJobFunction =
            AZStd::bind(&FontBuilderWorker::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        fontBuilderDescriptor.m_processJobFunction =
            AZStd::bind(&FontBuilderWorker::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(fontBuilderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, fontBuilderDescriptor);
    }

    void FontBuilderWorker::UnregisterBuilderWorker()
    {
        BusDisconnect();
    }

    AZ::Data::AssetType FontBuilderWorker::GetAssetType(const AZStd::string& fileName) const
    {
        AZStd::string fileExtension;
        AzFramework::StringFunc::Path::GetExtension(fileName.c_str(), fileExtension, false);
        
        if (fileExtension == "font" || fileExtension == "fontfamily")
        {
            return azrtti_typeid<LyShine::FontAsset>();
        }

        return AZ::Data::AssetType::CreateNull();
    }

    void FontBuilderWorker::AddProductDependencies(
        const AZ::rapidxml::xml_node<char>* node,
        const AZStd::string& fullPath,
        const AZStd::string& sourceFile,
        const AZStd::string& platformIdentifier,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& /*productDependencies*/,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {
        AZ_UNUSED(platformIdentifier);
        AZ_UNUSED(fullPath);

        AZ::rapidxml::xml_attribute<char>* attr = node->first_attribute("path", 0, false);
        if (attr)
        {
            // Add relative path to the product dependencies list when a dependency is detected
            QString productRelativeDir = QFileInfo(sourceFile.c_str()).dir().path();

            AZStd::string relativePath;
            AzFramework::StringFunc::Path::Join(productRelativeDir.toStdString().c_str(), attr->value(), relativePath);
            pathDependencies.emplace(QDir::cleanPath(relativePath.c_str()).toStdString().c_str(), AssetBuilderSDK::ProductPathDependencyType::ProductFile);
        }
    }
}
