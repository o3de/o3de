/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AnimGraphNode.h"
#include <AzCore/Math/Vector2.h>


namespace EMotionFX
{
    class EMFX_API BlendSpaceNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendSpaceNode, "{11EC99C4-6A25-4610-86FD-B01F2E53007E}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum EBlendSpaceEventMode : AZ::u8
        {
            BSEVENTMODE_ALL_ACTIVE_MOTIONS = 0,
            BSEVENTMODE_MOST_ACTIVE_MOTION = 1,
            BSEVENTMODE_NONE               = 2
        };

        enum ECalculationMethod : AZ::u8
        {
            AUTO = 0,
            MANUAL = 1
        };

        class EMFX_API BlendSpaceMotion final
        {
        public:
            AZ_RTTI(BlendSpaceMotion, "{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}")
            AZ_CLASS_ALLOCATOR_DECL

            enum class TypeFlags : AZ::u8
            {
                None = 0,
                UserSetCoordinateX = 1 << 0,   // Flag set if the x coordinate is set by the user instead of being auto computed.
                BlendSpace1D = 1 << 1,
                BlendSpace2D = 1 << 2,
                UserSetCoordinateY = 1 << 3,   // Flag set if the y coordinate is set by the user instead of being auto computed.
                InvalidMotion = 1 << 4    // Flag set when the motion is invalid
            };

            BlendSpaceMotion();
            BlendSpaceMotion(const AZStd::string& motionId);
            BlendSpaceMotion(const AZStd::string& motionId, const AZ::Vector2& coordinates, TypeFlags typeFlags);
            ~BlendSpaceMotion();

            void Set(const AZStd::string& motionId, const AZ::Vector2& coordinates, TypeFlags typeFlags = TypeFlags::None);
            const AZStd::string& GetMotionId() const;

            const AZ::Vector2& GetCoordinates() const;

            float GetXCoordinate() const;
            float GetYCoordinate() const;

            void SetXCoordinate(float x);
            void SetYCoordinate(float y);

            bool IsXCoordinateSetByUser() const;
            bool IsYCoordinateSetByUser() const;

            void MarkXCoordinateSetByUser(bool setByUser);
            void MarkYCoordinateSetByUser(bool setByUser);

            int GetDimension() const;
            void SetDimension(int dimension);

            void SetFlag(TypeFlags flag);
            void UnsetFlag(TypeFlags flag);
            bool TestFlag(TypeFlags flag) const;

            static void Reflect(AZ::ReflectContext* context);

        private:
            AZStd::string   m_motionId;
            AZ::Vector2     m_coordinates; // Coordinates of the motion in blend space
            TypeFlags       m_typeFlags;
        };

    public:
        BlendSpaceNode();
        BlendSpaceNode(AnimGraph* animGraph, const char* name);

        //! Compute the position of the motion in blend space.
        virtual void ComputeMotionCoordinates(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance, AZ::Vector2& position) = 0;

        //! Restore the motion coordinates that are set to automatic mode back to the computed values.
        virtual void RestoreMotionCoordinates(BlendSpaceMotion& motion, AnimGraphInstance* animGraphInstance) = 0;

        //! Common interface to access the blend space motions regardless of the blend space dimension.
        virtual void SetMotions(const AZStd::vector<BlendSpaceMotion>& motions) = 0;
        virtual const AZStd::vector<BlendSpaceMotion>& GetMotions() const = 0;

        //! The node is in interactive mode when the user is interactively changing the current point.
        void SetInteractiveMode(bool enable) { m_interactiveMode = enable; }
        bool IsInInteractiveMode() const { return m_interactiveMode; }

        static void Reflect(AZ::ReflectContext* context);

    protected:
        struct MotionInfo
        {
            MotionInstance*     m_motionInstance;
            AnimGraphSyncTrack* m_syncTrack;
            size_t              m_syncIndex;
            float               m_playSpeed;
            float               m_currentTime; // current play time (NOT normalized)
            float               m_preSyncTime;

            MotionInfo();
        };
        using MotionInfos = AZStd::vector<MotionInfo>;

        struct BlendInfo
        {
            AZ::u32 m_motionIndex;
            float   m_weight;

            bool operator<(const BlendInfo& rhs) const
            {
                // We want to sort in decreasing order of weight.
                return m_weight > rhs.m_weight;
            }
        };
        using BlendInfos = AZStd::vector<BlendInfo>;

    protected:
        size_t FindMotionIndexByMotionId(const AZStd::vector<BlendSpaceMotion>& motions, const AZStd::string& motionId) const;

        void DoUpdate(float timePassedInSeconds, const BlendInfos& blendInfos, ESyncMode syncMode, AZ::u32 leaderIdx, MotionInfos& motionInfos);
        void DoTopDownUpdate(AnimGraphInstance* animGraphInstance, ESyncMode syncMode,
            AZ::u32 leaderIdx, MotionInfos& motionInfos, bool motionsHaveSyncTracks);
        void DoPostUpdate(AnimGraphInstance* animGraphInstance, AZ::u32 leaderIdx, BlendInfos& blendInfos, MotionInfos& motionInfos,
            EBlendSpaceEventMode eventFilterMode, AnimGraphRefCountedData* data, bool inPlace);

        void RewindMotions(MotionInfos& motionInfos);

        static size_t GetIndexOfMotionInBlendInfos(const BlendInfos& blendInfos, size_t motionIndex);

        static void ClearMotionInfos(MotionInfos& motionInfos);
        static void AddMotionInfo(MotionInfos& motionInfos, MotionInstance* motionInstance);

        static bool DoAllMotionsHaveSyncTracks(const MotionInfos& motionInfos);

        static void DoClipBasedSyncOfMotionsToLeader(AZ::u32 leaderIdx, MotionInfos& motionInfos);
        static void DoEventBasedSyncOfMotionsToLeader(AZ::u32 leaderIdx, MotionInfos& motionInfos);

        static void SyncMotionToNode(AnimGraphInstance* animGraphInstance, ESyncMode syncMode, MotionInfo& motionInfo, AnimGraphNode* srcNode);

    private:
        static const char* s_calculationModeAuto;
        static const char* s_calculationModeManual;

        static const char* s_eventModeAllActiveMotions;
        static const char* s_eventModeMostActiveMotion;
        static const char* s_eventModeNone;

    protected:
        bool m_interactiveMode = false;// true when the user is changing the current point by dragging in GUI
        bool m_retarget = true;
        bool m_inPlace = false;
    };

    //================  Inline implementations ====================================

    MCORE_INLINE size_t BlendSpaceNode::GetIndexOfMotionInBlendInfos(const BlendInfos& blendInfos, size_t motionIndex)
    {
        const size_t numBlendInfos = blendInfos.size();
        for (size_t i = 0; i < numBlendInfos; ++i)
        {
            if (blendInfos[i].m_motionIndex == motionIndex)
            {
                return i;
            }
        }
        return MCORE_INVALIDINDEX32;
    }
} // namespace EMotionFX


namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::BlendSpaceNode::ECalculationMethod, "{A038B95B-6D36-45DD-813A-9A75863DEA7A}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::BlendSpaceNode::EBlendSpaceEventMode, "{F451554D-0CCB-4E22-96DB-213EC69E565F}");
} // namespace AZ
