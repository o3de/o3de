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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Reflect/Model/SkinMetaAsset.h>

namespace AZ
{
    namespace RPI
    {
        void SkinMetaAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SkinMetaAsset>()
                    ->Version(1)
                    ->Field("jointIndexToNameMap", &SkinMetaAsset::m_jointNameToIndexMap)
                    ;
            }
        }

        AZ::Data::AssetId SkinMetaAsset::ConstructAssetId(const AZ::Data::AssetId& modelAssetId, const AZStd::string& modelAssetName)
        {
            if (modelAssetName.empty())
            {
                AZ_Error("SkinMetaAsset", false, "Cannot construct asset id for skin meta asset. Model asset name is empty.");
                return {};
            }

            // The sub id of any model related assets starts with the same prefix for first 8 bits and uses the name hash for the last 24 bits.
            uint32_t productSubId = SkinMetaAsset::s_assetIdPrefix | AZ::Crc32(modelAssetName) & 0xffffff;
            return {modelAssetId.m_guid, productSubId};
        }

        void SkinMetaAsset::SetReady()
        {
            m_status = Data::AssetData::AssetStatus::Ready;
        }

        void SkinMetaAsset::SetJointNameToIndexMap(const AZStd::unordered_map<AZStd::string, uint16_t>& jointNameToIndexMap)
        {
            m_jointNameToIndexMap = jointNameToIndexMap;
        }

        const AZStd::unordered_map<AZStd::string, uint16_t>& SkinMetaAsset::GetJointNameToIndexMap() const
        {
            return m_jointNameToIndexMap;
        }

        uint16_t SkinMetaAsset::GetJointIndexByName(const AZStd::string& jointName) const
        {
            const auto findIter = m_jointNameToIndexMap.find(jointName);
            if (findIter != m_jointNameToIndexMap.end())
            {
                return findIter->second;
            }

            return InvalidJointIndex;
        }
    } //namespace RPI
} // namespace AZ
