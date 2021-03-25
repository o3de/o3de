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

#include <AzCore/Asset/AssetManager.h>
#include <Atom/RPI.Reflect/Model/SkinMetaAssetCreator.h>

namespace AZ
{
    namespace RPI
    {
        void SkinMetaAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);
        }

        void SkinMetaAssetCreator::SetJointNameToIndexMap(const AZStd::unordered_map<AZStd::string, AZ::u16>& jointNameToIndexMap)
        {
            if (ValidateIsReady())
            {
                m_asset->SetJointNameToIndexMap(jointNameToIndexMap);
            }
        }

        bool SkinMetaAssetCreator::End(Data::Asset<SkinMetaAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            m_asset->SetReady();
            return EndCommon(result);
        }
    } // namespace RPI
} // namespace AZ
