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

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include "AssetBuilderSDK/AssetBuilderSDK.h"

namespace AZ
{
    namespace RC
    {
        class AssetParser
        {
        public:
            AssetParser(const AZStd::string& assetName);
            virtual ~AssetParser() {}
            virtual AssetBuilderSDK::ProductPathDependencySet GetProductDependencies(const AZStd::string& fullPath);

        protected:
            AZStd::string m_assetName;
        };
    } // namespace RC
} // namespace AZ