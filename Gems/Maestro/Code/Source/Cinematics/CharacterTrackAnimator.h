/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

/*!
        Utility class to handle Animation of Character Tracks (aka 'Animation' Tracks in the TrackView UI)
    */

#include <Cry_Math.h>

struct SAnimContext;
struct ICharacterKey;
struct IAnimNode;

#define MAX_CHARACTER_TRACKS 3
#define ADDITIVE_LAYERS_OFFSET 6

class CCharacterTrackAnimator
{
public:
    CCharacterTrackAnimator();
    ~CCharacterTrackAnimator() = default;
 
    struct SAnimState
    {
        int32 m_lastAnimationKeys[3][2];
        bool m_layerPlaysAnimation[3];

        //! This is used to indicate that a time-jumped blending is currently happening in the animation track.
        bool m_bTimeJumped[3];
        float m_jumpTime[3];
    };

    void OnReset(IAnimNode* animNode);

    ILINE bool IsAnimationPlaying(const SAnimState& animState) const;

    //! Animate Character Track.
    void AnimateTrack(class CCharacterTrack* track, SAnimContext& ec, int layer, int trackIndex);

    // Forces current playhead anim key state change to reset animation cues
    void ForceAnimKeyChange();
    //////////////////////////////////////////////////////////////////////////////////////////
protected:
    f32 ComputeAnimKeyNormalizedTime(const ICharacterKey& key, float ectime) const;

private:
    void ResetLastAnimKeys();
    void ReleaseAllAnimations(IAnimNode* animNode);

    //////////////////////////////////////////////////////////////////////////////////////////
    static const float s_minClipDuration;

    SAnimState m_baseAnimState;
    bool       m_characterWasTransRot;
    bool       m_forceAnimKeyChange;
};
