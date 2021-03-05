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

#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RPI.Edit/Common/ConvertibleSource.h>
#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    class ReflectContext;
    namespace RPI
    {          
        //! Source data for AssetAliases. It implements the ConvertibleSource interface so it can be converted to 
        //! AssetAliases data when it's used for AnyAsset and be processed by AnyAssetBuilder.
        class AssetAliasesSourceData final
            : public ConvertibleSource
        {
        public:
            AZ_TYPE_INFO(AssetAliasesSourceData, "{6EEE3144-33CC-4CE9-9C03-E411571D0712}");
            AZ_CLASS_ALLOCATOR(AssetAliasesSourceData, AZ::SystemAllocator, 0);
            
            static void Reflect(ReflectContext* context);

            // ConvertibleSource overrides...
            bool Convert(TypeId& outTypeId, AZStd::shared_ptr<void>& outData) const override;

        private:
            struct AssetAliasInfo
            {
                AZ_TYPE_INFO(AssetAliasInfo, "{192A7D39-BE4D-4C4C-AEC9-D56745EB62D0}");
                AZ_CLASS_ALLOCATOR(AssetAliasInfo, AZ::SystemAllocator, 0);
                AZStd::string m_alias;
                AZStd::string m_path;
            };

            AZStd::vector<AssetAliasInfo> m_assetPaths;
        };

    } // namespace RPI
} // namespace AZ
