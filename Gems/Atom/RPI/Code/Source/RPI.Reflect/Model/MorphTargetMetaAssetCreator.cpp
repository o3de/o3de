/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <Atom/RPI.Reflect/Model/MorphTargetMetaAssetCreator.h>

namespace AZ::RPI
{
    void MorphTargetMetaAssetCreator::Begin(const Data::AssetId& assetId)
    {
        BeginCommon(assetId);
    }

    void MorphTargetMetaAssetCreator::AddMorphTarget(const MorphTargetMetaAsset::MorphTarget& morphTarget)
    {
        if (ValidateIsReady())
        {
            m_asset->AddMorphTarget(morphTarget);
        }
    }

    bool MorphTargetMetaAssetCreator::End(Data::Asset<MorphTargetMetaAsset>& result)
    {
        if (!ValidateIsReady())
        {
            return false;
        }

        m_asset->SetReady();
        return EndCommon(result);
    }

    bool MorphTargetMetaAssetCreator::IsEmpty()
    {
        if (ValidateIsReady())
        {
            return m_asset->GetMorphTargets().empty();
        }

        return true;
    }

} // namespace AZ::RPI
