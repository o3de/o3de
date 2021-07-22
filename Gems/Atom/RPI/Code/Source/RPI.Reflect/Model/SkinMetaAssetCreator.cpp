/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
