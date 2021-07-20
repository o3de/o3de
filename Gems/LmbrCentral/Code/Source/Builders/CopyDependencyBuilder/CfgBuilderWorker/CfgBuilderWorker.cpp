/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LmbrCentral_precompiled.h>
#include "CfgBuilderWorker.h"

#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace CopyDependencyBuilder
{
    // m_skipServer (3rd Param) should be false - we want to process xml files on the server as it's a generic data format which could
    // have meaningful data for a server
    CfgBuilderWorker::CfgBuilderWorker()
        : CopyDependencyBuilderWorker("CFG", true, false)
    {
    }

    void CfgBuilderWorker::RegisterBuilderWorker()
    {
        AssetBuilderSDK::AssetBuilderDesc cfgBuilderDescriptor;
        cfgBuilderDescriptor.m_name = "CfgBuilderWorker";
        cfgBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.cfg", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        cfgBuilderDescriptor.m_busId = azrtti_typeid<CfgBuilderWorker>();
        cfgBuilderDescriptor.m_version = 3;
        cfgBuilderDescriptor.m_createJobFunction =
            AZStd::bind(&CfgBuilderWorker::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        cfgBuilderDescriptor.m_processJobFunction =
            AZStd::bind(&CfgBuilderWorker::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(cfgBuilderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, cfgBuilderDescriptor);
    }

    void CfgBuilderWorker::UnregisterBuilderWorker()
    {
        BusDisconnect();
    }

    struct CfgValueExtensionAndDependencyType
    {
        CfgValueExtensionAndDependencyType(const AZStd::string& extension, const AssetBuilderSDK::ProductPathDependencyType dependencyType) :
            m_extension(extension),
            m_dependencyType(dependencyType)
        {

        }

        AZStd::string m_extension;
        AssetBuilderSDK::ProductPathDependencyType m_dependencyType;
    };

    struct CfgKeysAndValuesWithDependencies
    {
        CfgKeysAndValuesWithDependencies(const AZStd::string& key, const AZStd::vector<CfgValueExtensionAndDependencyType>& values) :
            m_key(key),
            m_values(values)
        {

        }

        AZStd::string ExtensionsToString() const
        {
            AZStd::string extensionList;

            for (const CfgValueExtensionAndDependencyType& cfgValue : m_values)
            {
                extensionList = AZStd::string::format("%s%s%s", extensionList.c_str(), extensionList.empty() ? "" : ", ", cfgValue.m_extension.c_str());
            }
            return extensionList;
        }

        AZStd::string m_key;
        AZStd::vector<CfgValueExtensionAndDependencyType> m_values;
    };

    bool CfgBuilderWorker::ParseProductDependencies(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& /*productDependencies*/,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {
        AZ::Outcome<AZStd::string, AZStd::string> configFileContents(AzFramework::FileFunc::GetCfgFileContents(request.m_fullPath));

        if (!configFileContents.IsSuccess())
        {
            AZ_Error("CfgBuilderWorker", false, configFileContents.GetError().c_str());
            return false;
        }
        return ParseProductDependenciesFromCfgContents(request.m_fullPath, configFileContents.GetValue(), pathDependencies);
    }

    bool CfgBuilderWorker::ParseProductDependenciesFromCfgContents([[maybe_unused]] const AZStd::string& fullPath, const AZStd::string& contents, AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {
        const AZStd::vector<CfgValueExtensionAndDependencyType> uiCanvas =
        {
            {".uicanvas", AssetBuilderSDK::ProductPathDependencyType::ProductFile }
        };
        const AZStd::vector<CfgValueExtensionAndDependencyType> imageFiles =
        {
            // There is no common place this extension list is defined, it is copy & pasted in many places.
            {".bmp", AssetBuilderSDK::ProductPathDependencyType::SourceFile },
            {".gif", AssetBuilderSDK::ProductPathDependencyType::SourceFile },
            {".jpeg", AssetBuilderSDK::ProductPathDependencyType::SourceFile },
            {".jpg", AssetBuilderSDK::ProductPathDependencyType::SourceFile },
            {".png", AssetBuilderSDK::ProductPathDependencyType::SourceFile },
            {".tif", AssetBuilderSDK::ProductPathDependencyType::SourceFile },
            {".tiff", AssetBuilderSDK::ProductPathDependencyType::SourceFile },
            {".tga", AssetBuilderSDK::ProductPathDependencyType::SourceFile },
            // Only need to look for DDS as a product, if it's in the source it gets copied to the cache.
            {".dds", AssetBuilderSDK::ProductPathDependencyType::ProductFile }
        };

        CfgKeysAndValuesWithDependencies supportedConfigFileDependencies[] = {
            // These commands are defined in CrySystem\SystemInit.cpp
            {"game_load_screen_uicanvas_path", uiCanvas },
            {"level_load_screen_uicanvas_path", uiCanvas },
            {"sys_splashscreen", imageFiles },
        };

        bool allConfigDependenciesValid = true;

        for (const CfgKeysAndValuesWithDependencies& config : supportedConfigFileDependencies)
        {
            AZ::Outcome<AZStd::string, AZStd::string> valueForKey(
                AzFramework::FileFunc::GetValueForKeyInCfgFileContents(contents, config.m_key.c_str()));

            if (!valueForKey.IsSuccess())
            {
                // No error here, the key was either not in the file or not set to something valid.
                continue;
            }

            AZStd::string cleanedUpKey(valueForKey.GetValue());            
            AzFramework::StringFunc::AssetDatabasePath::Normalize(cleanedUpKey);
            AZStd::to_lower(cleanedUpKey.begin(), cleanedUpKey.end());

            CfgValueExtensionAndDependencyType const* validExtension(nullptr);
            for (const CfgValueExtensionAndDependencyType& cfgValue : config.m_values)
            {
                if (AzFramework::StringFunc::Path::IsExtension(cleanedUpKey.c_str(), cfgValue.m_extension.c_str()))
                {
                    validExtension = &cfgValue;
                    break;
                }
            }

            if (!validExtension)
            {
                AZ_Error("CfgBuilderWorker", false, "Unsupported extension in config file '%s' for key '%s'. Expected '%s', found value '%s'",
                    fullPath.c_str(),
                    config.m_key.c_str(),
                    config.ExtensionsToString().c_str(),
                    valueForKey.GetValue().c_str());
                allConfigDependenciesValid = false;
            }
            else
            {
                pathDependencies.emplace(cleanedUpKey.c_str(), validExtension->m_dependencyType);
            }
        }
        return allConfigDependenciesValid;
    }
}
