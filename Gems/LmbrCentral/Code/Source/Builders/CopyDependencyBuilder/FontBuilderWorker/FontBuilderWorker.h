/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
