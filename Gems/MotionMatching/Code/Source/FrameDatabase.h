/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/tuple.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/TransformSpace.h>
#include <EventData.h>
#include <Frame.h>

namespace EMotionFX
{
    class Motion;
    class ActorInstance;
}

namespace EMotionFX::MotionMatching
{
    class MotionMatchingInstance;
    class MotionMatchEventData;

    //! A set of frames from your animations sampled at a given sample rate is stored in the frame database. A frame object knows about its index in the frame database,
    //! the animation it belongs to and the sample time in seconds. It does not hold the actual sampled pose for memory reasons as the `EMotionFX::Motion` already store the
    //! transform keyframes.
    //! The sample rate of the animation might differ from the sample rate used for the frame database. For example, your animations might be recorded with 60 Hz while we only want
    //! to extract the features with a sample rate of 30 Hz. As the motion matching algorithm is blending between the frames in the motion database while playing the animation window
    //! between the jumps/blends, it can make sense to have animations with a higher sample rate than we use to extract the features.
    //! A frame of the motion database can be used to sample a pose from which we can extract the features. It also provides functionality to sample a pose with a time offset to that frame.
    //! This can be handy in order to calculate joint velocities or trajectory samples.
    //! When importing animations, frames that are within the range of a discard frame motion event are ignored and won't be added to the motion database. Discard motion events can be
    //! used to cut out sections of the imported animations that are unwanted like a stretching part between two dance cards. 
    class EMFX_API FrameDatabase
    {
    public:
        AZ_RTTI(FrameDatabase, "{3E5ED4F9-8975-41F2-B665-0086368F0DDA}")
        AZ_CLASS_ALLOCATOR_DECL

        //! The settings used when importing motions into the frame database.
        //! Used in combination with ImportFrames().
        struct EMFX_API FrameImportSettings
        {
            size_t m_sampleRate = 30; //< Sample at 30 frames per second on default.
            bool m_autoShrink = true; //< Automatically shrink the internal frame arrays to their minimum size afterwards.
        };

        FrameDatabase();
        virtual ~FrameDatabase();

        // Main functions.
        AZStd::tuple<size_t, size_t> ImportFrames(Motion* motion, const FrameImportSettings& settings, bool mirrored); //< Returns the number of imported frames and the number of discarded frames as second element.
        void Clear(); //< Clear the data, so you can re-initialize it with new data.

        // Statistics.
        size_t GetNumFrames() const;
        size_t GetNumUsedMotions() const;
        size_t CalcMemoryUsageInBytes() const;

        // Misc.
        const Motion* GetUsedMotion(size_t index) const;
        const Frame& GetFrame(size_t index) const;
        const AZStd::vector<Frame>& GetFrames() const;
        AZStd::vector<Frame>& GetFrames();
        const AZStd::vector<const Motion*>& GetUsedMotions() const;
        size_t GetSampleRate() const { return m_sampleRate; }

        //! Find the frame index for the given playtime and motion.
        //! NOTE: This is a slow operation and should not be used by the runtime without visual debugging.
        size_t FindFrameIndex(Motion* motion, float playtime) const;

        //! Save every frame as a row to a .csv file.
        void SaveAsCsv(const char* filename, ActorInstance* actorInstance, const ETransformSpace transformSpace, bool writePositions, bool writeRotations) const;

    private:
        void ImportFrame(Motion* motion, float timeValue, bool mirrored);
        bool IsFrameDiscarded(const Motion* motion, float frameTime, AZStd::vector<EventData*>& activeEvents) const;
        void ExtractActiveMotionEventDatas(const Motion* motion, float time, AZStd::vector<EventData*>& activeEventDatas) const; // Vector will be cleared internally.

    private:
        AZStd::vector<Frame> m_frames; //< The collection of frames. Keep in mind these don't hold a pose, but reference to a given frame/time value inside a given motion. 
        AZStd::unordered_map<Motion*, AZStd::vector<size_t>> m_frameIndexByMotion;
        AZStd::vector<const Motion*> m_usedMotions; //< The list of used motions.
        size_t m_sampleRate = 0;
    };
} // namespace EMotionFX::MotionMatching
