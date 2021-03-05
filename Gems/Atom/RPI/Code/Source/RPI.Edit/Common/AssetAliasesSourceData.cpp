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

#include <Atom/RPI.Edit/Common/AssetAliasesSourceData.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/System/AssetAliases.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AZ
{
    namespace RPI
    {             
        void AssetAliasesSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AssetAliasInfo>()
                    ->Version(1)
                    ->Field("Name", &AssetAliasInfo::m_alias)
                    ->Field("Path", &AssetAliasInfo::m_path)
                    ;

                serializeContext->Class<AssetAliasesSourceData, ConvertibleSource>()
                    ->Version(1)
                    ->Field("AssetPaths", &AssetAliasesSourceData::m_assetPaths)
                    ;
            }
        }

        bool AssetAliasesSourceData::Convert(TypeId& outTypeId, AZStd::shared_ptr<void>& outData) const
        {
            outTypeId = AssetAliases::RTTI_Type();
            AssetAliases* assetAliases = aznew AssetAliases;

            for (auto& assetInfo : m_assetPaths)
            {
                if (assetAliases->m_assetMapping.find(assetInfo.m_alias) != assetAliases->m_assetMapping.end())
                {
                    AZ_Error("Asset Builder", false, "Duplicate asset alias [%s]", assetInfo.m_alias.c_str());
                    delete assetAliases;
                    return false;
                }

                Outcome<Data::AssetId> assetId = AssetUtils::MakeAssetId(assetInfo.m_path, 0);

                if (!assetId.IsSuccess())
                {
                    AZ_Error("Asset Builder", false, "Failed to find asset id with path [%s]", assetInfo.m_path.c_str());
                    delete assetAliases;
                    return false;
                }

                assetAliases->m_assetMapping[assetInfo.m_alias] = assetId.TakeValue();
            }

            outData = AZStd::shared_ptr<void>(assetAliases);
            return true;
        }

    } // namespace RPI
} // namespace AZ
