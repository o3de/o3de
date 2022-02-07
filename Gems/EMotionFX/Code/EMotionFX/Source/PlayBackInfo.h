/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/Vector.h>

namespace EMotionFX
{
    /**
     * A value you can pass to as the number of loops for a MotionInstance or PlayBackInfo. When using this value
     * the motion will loop forever, and never stop repeating. Unless it gets stopped automatically or manually of course.
     */
    #define EMFX_LOOPFOREVER 0xFFFFFFFF


    /**
     * The motion blend modes.
     * These blendmodes describe how the motion gets blend together with other motions currently being played.
     */
    enum EMotionBlendMode : AZ::u8
    {
        BLENDMODE_OVERWRITE = 0, /**< Overwrite mode. This can be used to switch from for example walk into run. */
        BLENDMODE_ADDITIVE  = 1 /**< Additive mode. This can be used to add the given motion relatively to the current result. */
    };


    /**
     * The playback mode, which defines the playback direction of the motion.
     */
    enum EPlayMode : AZ::u8
    {
        PLAYMODE_FORWARD    = 0,/**< Forward playback mode, this is the regular playback what you expect when playing a motion. */
        PLAYMODE_BACKWARD   = 1 /**< Backward playback mode, which makes the motion play from the last frame towards the first frame. */
    };


    /**
     * Motion extraction settings unique to each motion.
     */
    enum EMotionExtractionFlags : uint8
    {
        MOTIONEXTRACT_CAPTURE_Z     = 1 << 0    /**< Capture the movement over the z axis, so the height, for thing such as climbing motions. */
    };


    /**
     * The playback information, which are used when playing a motion on an actor.
     * You have to pass an object of this class to the Actor::PlayMotion(...) method.
     */
    class EMFX_API PlayBackInfo
    {
    public:
        /**
         * The constructor.
         * Automatically initializes all members on the following default values.
         *
         * <pre>
         * Member Name           - Default Value
         * ---------------------------------
         * m_blendInTime          - 0.3 (seconds)
         * m_blendOutTime         - 0.3 (seconds)
         * m_playSpeed            - 1.0 (original speed)
         * m_targetWeight         - 1.0 (fully blend in)
         * m_eventWeightThreshold - 0.0 (allow all events even with low motion instance weight values)
         * m_maxPlayTime          - 0.0 (disabled when zero or negative)
         * m_clipStartTime        - 0.0 (start and loop from the beginning of the motion)
         * m_clipEndTime          - 0.0 (set to negative or zero to play the full range of the motion)
         * m_numLoops             - EMFX_LOOPFOREVER
         * m_blendMode            - BLENDMODE_OVERWRITE (overwrites motions)
         * m_playMode             - PLAYMODE_FORWARD (regular forward playing motion)
         * m_mirrorMotion         - false (disable motion mirroring)
         * m_playNow              - true (start playing immediately)
         * m_mix                  - false (non mixing motion)
         * m_priorityLevel        - 0 (no priority)
         * m_motionExtractionEnabled - true
         * m_retarget             - false (no motion retargeting allowed)
         * m_freezeAtLastFrame    - true (motion freezes in last frame when not looping forever)
         * m_enableMotionEvents   - true (all motion events will be processed for this motion instance
         * m_blendOutBeforeEnded  - true (blend out so that it faded out at the end of the motion).
         * m_canOverwrite         - true (can overwrite other motion instances when reaching a weight of 1.0)
         * m_deleteOnZeroWeight   - true (delete this motion instance when it reaches a weight of 0.0)
         * m_freezeAtTime;        - -1.0 (Freeze at a given time offset in seconds. The current play time would continue running though, and a blend out would be triggered, unlike the m_freezeAtLastFrame. Set to negative value to disable.
         *
         * </pre>
         *
         * You can adjust these settings.
         */
        PlayBackInfo()
        {
            m_blendInTime         = 0.3f;
            m_blendOutTime        = 0.3f;
            m_playSpeed           = 1.0f;
            m_targetWeight        = 1.0f;
            m_eventWeightThreshold = 0.0f;
            m_maxPlayTime         = 0.0f;
            m_clipStartTime       = 0.0f;
            m_clipEndTime         = 0.0f;
            m_freezeAtTime        = -1.0f;
            m_numLoops            = EMFX_LOOPFOREVER;
            m_blendMode           = BLENDMODE_OVERWRITE;
            m_playMode            = PLAYMODE_FORWARD;
            m_mirrorMotion        = false;
            m_playNow             = true;
            m_mix                 = false;
            m_motionExtractionEnabled = true;
            m_retarget            = false;
            m_freezeAtLastFrame   = true;
            m_enableMotionEvents  = true;
            m_blendOutBeforeEnded = true;
            m_canOverwrite        = true;
            m_deleteOnZeroWeight  = true;
            m_inPlace             = false;
            m_priorityLevel       = 0;
        }

        /**
         * The destructor.
         */
        ~PlayBackInfo() {}


    public:
        float               m_blendInTime;               /**< The time, in seconds, which it will take to fully have blended to the target weight. */
        float               m_blendOutTime;              /**< The time, in seconds, which it takes to smoothly fadeout the motion, after it has been stopped playing. */
        float               m_playSpeed;                 /**< The playback speed factor. A value of 1 stands for the original speed, while for example 2 means twice the original speed. */
        float               m_targetWeight;              /**< The target weight, where 1 means fully active, and 0 means not active at all. */
        float               m_eventWeightThreshold;      /**< The motion event weight threshold. If the motion instance weight is lower than this value, no motion events will be executed for this motion instance. */
        float               m_maxPlayTime;               /**< The maximum play time, in seconds. Set to zero or a negative value to disable it. */
        float               m_clipStartTime;             /**< The start playback time in seconds. Also in case of looping it will jump to this position on a loop. */
        float               m_clipEndTime;               /**< The end playback time in seconds. It will jump back to the clip start time after reaching this playback time. */
        float               m_freezeAtTime;              /**< Freeze at a given time offset in seconds. The current play time would continue running though, and a blend out would be triggered, unlike the m_freezeAtLastFrame. Set to negative value to disable. Default=-1.*/
        uint32              m_numLoops;                  /**< The number of times you want to play this motion. A value of EMFX_LOOPFOREVER means it will loop forever. */
        uint32              m_priorityLevel;             /**< The priority level, the higher this value, the higher priority it has on overwriting other motions. */
        EMotionBlendMode    m_blendMode;                 /**< The motion blend mode. Please read the MotionInstance::SetBlendMode(...) method for more information. */
        EPlayMode           m_playMode;                  /**< The motion playback mode. This means forward or backward playback. */
        bool                m_mirrorMotion;              /**< Is motion mirroring enabled or not? When set to true, the m_mirrorPlaneNormal is used as mirroring axis. */
        bool                m_mix;                       /**< Set to true if you want this motion to mix or not. */
        bool                m_playNow;                   /**< Set to true if you want to start playing the motion right away. If set to false it will be scheduled for later by inserting it into the motion queue. */
        bool                m_motionExtractionEnabled;   /**< Set to true if you want this motion to move and rotate the actor instance, otherwise set to false. */
        bool                m_retarget;                  /**< Set to true if you want to enable motion retargeting. Read the manual for more information. */
        bool                m_freezeAtLastFrame;         /**< Set to true if you like the motion to freeze at the last frame, for example in case of a death motion. */
        bool                m_enableMotionEvents;        /**< Set to true to enable motion events, or false to disable processing of motion events for this motion instance. */
        bool                m_blendOutBeforeEnded;       /**< Set to true if you want the motion to be stopped so that it exactly faded out when the motion/loop fully finished. If set to false it will fade out after the loop has completed (and starts repeating). The default is true. */
        bool                m_canOverwrite;              /**< Set to true if you want this motion to be able to delete other underlaying motion instances when this motion instance reaches a weight of 1.0.*/
        bool                m_deleteOnZeroWeight;        /**< Set to true if you wish to delete this motion instance once it reaches a weight of 0.0. */
        bool                m_inPlace;                   /**< Set to true if you want the motion to play in place. This means the root of the motion will not move. */
    };
} // namespace EMotionFX
