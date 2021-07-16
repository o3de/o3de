/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "IMovieSystem.h"
#include "AnimTrack.h"
#include <Maestro/Types/AssetBlends.h>

/** CAssetBlendTrack contains entity keys, when time reach event key, it fires script event or start animation etc...
*/
class CAssetBlendTrack
    : public TAnimTrack<AZ::IAssetBlendKey>
{
public:
    AZ_CLASS_ALLOCATOR(CAssetBlendTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CAssetBlendTrack, "{8F606315-A8D9-4267-A1DA-8E84097F40CD}", IAnimTrack);

    CAssetBlendTrack()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // Overrides of IAnimTrack.
    //////////////////////////////////////////////////////////////////////////
    bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;

    void GetKeyInfo(int key, const char*& description, float& duration) override;
    void SerializeKey(AZ::IAssetBlendKey& key, XmlNodeRef& keyNode, bool bLoading) override;

    //! Gets the duration of an animation key. If it's a looped animation,
    //! a special consideration is required to compute the actual duration.
    float GetKeyDuration(int key) const;

    AnimValueType GetValueType() override;
    void GetValue(float time, Maestro::AssetBlends<AZ::Data::AssetData>& value) override;

    void SetDefaultValue(const Maestro::AssetBlends<AZ::Data::AssetData>& defaultValue);

    float GetEndTime() const;

    static void Reflect(AZ::ReflectContext* context);

private:

    // Internal transient state, not serialized.
    Maestro::AssetBlends<AZ::Data::AssetData> m_assetBlend;
    Maestro::AssetBlends<AZ::Data::AssetData> m_defaultValue;
};
