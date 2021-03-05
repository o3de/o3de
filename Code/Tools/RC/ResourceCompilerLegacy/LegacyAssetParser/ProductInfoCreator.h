/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "AssetParser.h"

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AZ
{
    namespace RC
    {
        static AZ::Data::AssetType fontAssetType("{57767D37-0EBE-43BE-8F60-AB36D2056EF8}"); // form UiAssetTypes.h

        class ProductInfoCreator
        {
        public:

            ProductInfoCreator();

            AssetBuilderSDK::JobProduct GenerateProductInfo(const AZStd::string& sourceFile, const AZStd::string& fullPath);

        protected:

            virtual AZStd::unique_ptr<AssetParser> CreateLegacyAssetParser(const AZStd::string& sourceFile);

            AZ::Data::AssetType m_productAssetType;
        };
    } // namespace RC
} // namespace AZ
