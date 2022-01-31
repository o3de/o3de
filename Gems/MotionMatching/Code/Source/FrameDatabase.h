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

    // The motion matching data.
    // This is basically a database of frames (which point to motion objects), together with meta data per frame.
    // No actual pose data is stored directly inside this class, just references to the right sample times inside specific motions.
    class EMFX_API FrameDatabase
    {
    public:
        AZ_RTTI(FrameDatabase, "{3E5ED4F9-8975-41F2-B665-0086368F0DDA}")
        AZ_CLASS_ALLOCATOR_DECL

        // The settings used when importing motions into the frame database.
        // Used in combination with ImportFrames().
        struct EMFX_API FrameImportSettings
        {
            size_t m_sampleRate = 30; /**< Sample at 30 frames per second on default. */
            bool m_autoShrink = true; /**< Automatically shrink the internal frame arrays to their minimum size afterwards. */
        };

        FrameDatabase();
        virtual ~FrameDatabase();

        // Main functions.
        AZStd::tuple<size_t, size_t> ImportFrames(Motion* motion, const FrameImportSettings& settings, bool mirrored); // Returns the number of imported frames and the number of discarded frames as second element.
        void Clear(); // Clear the data, so you can re-initialize it with new data.

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

        /**
         * Find the frame index for the given playtime and motion.
         * NOTE: This is a slow operation and should not be used by the runtime without visual debugging.
         */
        size_t FindFrameIndex(Motion* motion, float playtime) const;

    private:
        void ImportFrame(Motion* motion, float timeValue, bool mirrored);
        bool IsFrameDiscarded(const AZStd::vector<EventData*>& activeEventDatas) const;
        void ExtractActiveMotionEventDatas(const Motion* motion, float time, AZStd::vector<EventData*>& activeEventDatas); // Vector will be cleared internally.

    private:
        AZStd::vector<Frame> m_frames; /**< The collection of frames. Keep in mind these don't hold a pose, but reference to a given frame/time value inside a given motion. */
        AZStd::unordered_map<Motion*, AZStd::vector<size_t>> m_frameIndexByMotion;
        AZStd::vector<const Motion*> m_usedMotions; /**< The list of used motions. */
        size_t m_sampleRate = 0;
    };
} // namespace EMotionFX::MotionMatching
