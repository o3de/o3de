/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Reflect/Model/MorphTargetMetaAsset.h>

namespace AZ::RPI
{
    void MorphTargetMetaAsset::MorphTarget::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<MorphTargetMetaAsset::MorphTarget>()
                ->Version(1)
                ->Field("meshNodeName", &MorphTargetMetaAsset::MorphTarget::m_meshNodeName)
                ->Field("morphTargetName", &MorphTargetMetaAsset::MorphTarget::m_morphTargetName)
                ->Field("startIndex", &MorphTargetMetaAsset::MorphTarget::m_startIndex)
                ->Field("numVertices", &MorphTargetMetaAsset::MorphTarget::m_numVertices)
                ->Field("minPositionDelta", &MorphTargetMetaAsset::MorphTarget::m_minPositionDelta)
                ->Field("maxPositionDelta", &MorphTargetMetaAsset::MorphTarget::m_maxPositionDelta)
                ->Field("wrinkleMask", &MorphTargetMetaAsset::MorphTarget::m_wrinkleMask)
                ->Field("hasColorDeltas", &MorphTargetMetaAsset::MorphTarget::m_hasColorDeltas)
                ;
        }
    }

    void MorphTargetMetaAsset::Reflect(ReflectContext* context)
    {
        MorphTarget::Reflect(context);

        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<MorphTargetMetaAsset>()
                ->Version(1)
                ->Field("morphTargets", &MorphTargetMetaAsset::m_morphTargets)
                ;
        }
    }

    AZ::Data::AssetId MorphTargetMetaAsset::ConstructAssetId(const AZ::Data::AssetId& modelAssetId, const AZStd::string& modelAssetName)
    {
        if (modelAssetName.empty())
        {
            AZ_Error("SkinMetaAsset", false, "Cannot construct asset id for morph target meta asset. Model asset name is empty.");
            return {};
        }

        // The sub id of any model related assets starts with the same prefix for first 8 bits and uses the name hash for the last 24 bits.
        uint32_t productSubId = MorphTargetMetaAsset::s_assetIdPrefix | AZ::Crc32(modelAssetName) & 0xffffff;
        return {modelAssetId.m_guid, productSubId};
    }

    void MorphTargetMetaAsset::SetReady()
    {
        m_status = Data::AssetData::AssetStatus::Ready;
    }

    void MorphTargetMetaAsset::AddMorphTarget(const MorphTarget& morphTarget)
    {
        m_morphTargets.emplace_back(morphTarget);
    }

    const AZStd::vector<MorphTargetMetaAsset::MorphTarget>& MorphTargetMetaAsset::GetMorphTargets() const
    {
        return m_morphTargets;
    }
} // namespace AZ::RPI
