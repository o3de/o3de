/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <native/utilities/BuilderConfigurationBus.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

#include <QVariant>

namespace AssetProcessor
{
    const char BuilderConfigFile[] = "BuilderConfig.ini";

    class BuilderConfigurationManager : 
        public BuilderConfigurationRequestBus::Handler
    {
    public:

        using ParamMap = AZStd::unordered_map<AZStd::string, QVariant>;
        using ConfigMap = AZStd::unordered_map <AZStd::string, ParamMap>;
        BuilderConfigurationManager();
        ~BuilderConfigurationManager();

        //BuilderConfigurationRequestBus
        bool LoadConfiguration(const AZStd::string& configFile) override;
        bool UpdateJobDescriptor(const AZStd::string& jobKey, AssetBuilderSDK::JobDescriptor& jobDesc) override;
        bool UpdateBuilderDescriptor(const AZStd::string& builderName, AssetBuilderSDK::AssetBuilderDesc& jobDesc) override;

        bool IsLoaded() const { return m_loaded; }
    private:
        
        ConfigMap m_builderSettings;
        ConfigMap m_jobSettings;
        bool m_loaded{ false };
    };

} // namespace AssetProcessor


