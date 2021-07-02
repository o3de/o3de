/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <Builders/CopyDependencyBuilder/XmlFormattedAssetBuilderWorker.h>

namespace CopyDependencyBuilder
{
    //! The copy dependency builder is copy job that examines asset files for asset references,
    //! to output product dependencies.
    class FontBuilderWorker
        : public XmlFormattedAssetBuilderWorker
    {
    public:

        AZ_RTTI(FontBuilderWorker, "{399862CD-30BE-4D9A-A0F2-056BAB79E495}");

        FontBuilderWorker();

        void RegisterBuilderWorker() override;
        void UnregisterBuilderWorker() override;

    private:

        AZ::Data::AssetType GetAssetType(const AZStd::string& fileName) const override;
        void AddProductDependencies(
            const AZ::rapidxml::xml_node<char>* node,
            const AZStd::string& fullPath,
            const AZStd::string& sourceFile,
            const AZStd::string& platformIdentifier,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies) override;
    };
}
