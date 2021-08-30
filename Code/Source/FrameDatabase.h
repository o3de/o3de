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

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/tuple.h>

#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <Frame.h>

namespace EMotionFX
{
    class Motion;
    class ActorInstance;

    namespace MotionMatching
    {
        class FrameData;
        class BehaviorInstance;
        class MotionMatchEventData;
        class KdTree;

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
                // TODO: Add tags to ignore.
            };

            FrameDatabase();
            ~FrameDatabase();

            // Main functions.
            AZStd::tuple<size_t, size_t> ImportFrames(Motion* motion, const FrameImportSettings& settings, bool mirrored); // Returns the number of imported frames and the number of discarded frames as second element.
            void Clear(); // Clear the data, so you can re-initialize it with new data.
            void DebugDraw(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance);

            // Statistics.
            size_t GetNumFrames() const;
            size_t GetNumFrameDataTypes() const;
            size_t GetNumUsedMotions() const;
            size_t CalcMemoryUsageInBytes() const;

            // Misc.
            const Motion* GetUsedMotion(size_t index) const;
            const Frame& GetFrame(size_t index) const;
            const AZStd::vector<Frame>& GetFrames() const;
            AZStd::vector<Frame>& GetFrames();
            const AZStd::vector<const Motion*>& GetUsedMotions() const;
            FrameData* FindFrameData(const AZ::TypeId& frameDataTypeId) const;
            const FrameData* GetFrameData(size_t index) const;
            const AZStd::vector<FrameData*>& GetFrameData() const;

            //
            void RegisterFrameData(FrameData* frameData);
            bool GenerateFrameDatas(ActorInstance* actorInstance, size_t maxKdTreeDepth=20, size_t minFramesPerKdTreeNode=2000);

            size_t CalcNumDataDimensionsForKdTree() const;
            AZStd::vector<float> CalcMedians() const;

            KdTree& GetKdTree() { return *m_kdTree; }
            const KdTree& GetKdTree() const { return *m_kdTree; }

        private:
            static FrameData* CreateFrameDataByType(const AZ::TypeId& typeId); // create from RTTI type
            void ImportFrame(Motion* motion, float timeValue, bool mirrored);
            bool IsFrameDiscarded(const AZStd::vector<MotionMatchEventData*>& activeEventDatas) const;
            void ExtractActiveMotionMatchEventDatas(const Motion* motion, float time, AZStd::vector<MotionMatchEventData*>& activeEventDatas); // Vector will be cleared internally.

        private:
            AZStd::vector<Frame> m_frames; /**< The collection of frames. Keep in mind these don't hold a pose, but reference to a given frame/time value inside a given motion. */
            AZStd::unordered_map<AZ::TypeId, FrameData*> m_frameDatas; /**< The per frame additional data. */
            AZStd::vector<FrameData*> m_frameDataVector; /**< This is a flat vector of all frame datas. */
            AZStd::vector<const Motion*> m_usedMotions; /**< The list of used motions. */
            KdTree* m_kdTree = nullptr; /**< The acceleration structure to speed up the search for lowest cost frames. */
        };
    } // namespace MotionMatching
} // namespace EMotionFX
