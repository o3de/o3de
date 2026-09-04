/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AnimKey.h>

namespace AZ
{

/** IAssetBlendKey used in AssetBlend animation track.
*/
struct IAssetBlendKey
    : public ITimeRangeKey
{
    AZ::Data::AssetId m_assetId;    //!< Motion Asset Id (virtual property "Motion" of SimpleMotionComponent).
    AZStd::string m_description;    //!< Description (filename part of path to motion asset).
    float m_blendInTime;            //!< Blend in time in seconds (hidden virtual property "BlendInTime" of SimpleMotionComponent).
    float m_blendOutTime;           //!< Blend out time in seconds (hidden virtual property "BlendOutTime" of SimpleMotionComponent).;

    //!< ITimeRangeKey members:
    //!< m_duration  : Duration in seconds of this animation.
    //  There is SimpleMotionComponentRequestBus::Events::GetDuration, but TrackView uses EditorSimpleMotionComponentRequestBus::Events::GetAssetDuration.
    //!< m_startTime : Start time of this animation, equal to key time.
    //!< m_endTime   : End time of this animation, set to key time plus actual duration (m_duration / m_speed).
    //!< m_bLoop     : Whether to loop motion (hidden virtual property "LoopMotion" of SimpleMotionComponent).
    //!< m_speed     : Determines the rate at which the motion is played (hidden virtual property "PlaySpeed" of SimpleMotionComponent).

    IAssetBlendKey()
        : ITimeRangeKey()
        , m_blendInTime(0.0f)
        , m_blendOutTime(0.0f)
    {
    }

    static constexpr const float s_minSpeed = 0.1f;
    static constexpr const float s_maxSpeed = 10.f;
};

    AZ_TYPE_INFO_SPECIALIZE(IAssetBlendKey, "{15B82C3A-6DB8-466F-AF7F-18298FCD25FD}");
}
