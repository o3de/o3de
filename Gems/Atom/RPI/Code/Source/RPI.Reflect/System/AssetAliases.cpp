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

#include <Atom/RPI.Reflect/System/AssetAliases.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {        
        void AssetAliases::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<AssetAliases>()
                    ->Version(0)
                    ->Field("AssetMapping", &AssetAliases::m_assetMapping)
                    ;
            }
        }

        Data::AssetId AssetAliases::FindAssetId(const Name& alias) const
        {
            auto foundAsset = m_assetMapping.find(alias.GetCStr());
            if (foundAsset != m_assetMapping.end())
            {
                return foundAsset->second;
            }
            else
            {
                return Data::AssetId();
            }
        }

        const AZStd::unordered_map<AZStd::string, Data::AssetId>& AssetAliases::GetAssetMapping() const
        {
            return m_assetMapping;
        }

    } // namespace RPI
} // namespace AZ
