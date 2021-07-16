/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "CopyDependencyBuilderWorker.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzCore/XML/rapidxml.h>

namespace CopyDependencyBuilder
{
    class XmlFormattedAssetBuilderWorker
        : public CopyDependencyBuilderWorker
    {
    public:

        XmlFormattedAssetBuilderWorker(AZStd::string jobKey, bool critical, bool skipServer);

        bool ParseProductDependencies(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies) override;

    protected:

        // Check each XML node and add product dependencies if exist
        virtual void AddProductDependencies(
            const AZ::rapidxml::xml_node<char>* node,
            const AZStd::string& fullPath,
            const AZStd::string& sourceFile,
            const AZStd::string& platformIdentifier,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) = 0;

        // Parse the XML file
        virtual bool ParseXmlFile(
            const AZ::rapidxml::xml_node<char>* node,
            const AZStd::string& fullPath,
            const AZStd::string& sourceFile,
            const AZStd::string& platformIdentifier,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencies);
    };
}
