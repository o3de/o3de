/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EMotionFXConfig.h"
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>
#include <Integration/System/SystemCommon.h>
#include <EMotionFX/Source/Pose.h>

namespace EMotionFX
{
    class ActorInstance;

    class EMFX_API DebugDraw
    {
    public:
        AZ_RTTI(DebugDraw, "{44B1A0DB-422E-40D2-B0BC-54B9D7536E1A}");
        AZ_CLASS_ALLOCATOR_DECL

        struct EMFX_API Line
        {
            AZ::Vector3 m_start;
            AZ::Vector3 m_end;
            AZ::Color m_startColor;
            AZ::Color m_endColor;

            AZ_INLINE Line(const AZ::Vector3& start, const AZ::Vector3& end, const AZ::Color& startColor, const AZ::Color& endColor)
                : m_start(start)
                , m_end(end)
                , m_startColor(startColor)
                , m_endColor(endColor)
            {
            }

            AZ_INLINE Line(const AZ::Vector3& start, const AZ::Vector3& end, const AZ::Color& color)
                : Line(start, end, color, color)
            {
            }
        };

        class EMFX_API ActorInstanceData
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            ActorInstanceData(ActorInstance* actorInstance)
                : m_actorInstance(actorInstance)
            {
            }

            AZ_INLINE size_t GetNumLines() const { return m_lines.size(); }
            AZ_INLINE const AZStd::vector<Line>& GetLines() const { return m_lines; }

            void Clear();
            void Lock();
            void Unlock();

            AZ_INLINE void DrawLine(const Line& line) { m_lines.emplace_back(line); }
            AZ_INLINE void DrawLine(const AZ::Vector3& start, const AZ::Vector3& end, const AZ::Color& color) { m_lines.emplace_back(Line(start, end, color, color)); }
            AZ_INLINE void DrawLine(const AZ::Vector3& start, const AZ::Vector3& end, const AZ::Color& startColor, const AZ::Color& endColor) { m_lines.emplace_back(Line(start, end, startColor, endColor)); }
            void DrawWireframeSphere(const AZ::Vector3& center, float radius, const AZ::Color& color, const AZ::Quaternion& rotation, AZ::u32 numSegments = 16, AZ::u32 numRings = 16);
            void DrawWireframeCapsule(const AZ::Vector3& start, const AZ::Vector3& end, float radius, const AZ::Color& color, AZ::u32 numBodySubDivs = 16, AZ::u32 numSideSubDivs = 8);
            void DrawWireframeJointLimitCone(const AZ::Vector3& positionOffset, const AZ::Vector3& direction, float swingLimitDegreesX, float swingLimitDegreesY, float scale, const AZ::Color& color, AZ::u32 numAngularSubDivs = 32, AZ::u32 numRadialSubDivs = 4);
            void DrawMarker(const AZ::Vector3& position, const AZ::Color& color, float scale = 0.015f);
            void DrawPose(const Pose& pose, const AZ::Color& color, const AZ::Vector3& offset = AZ::Vector3::CreateZero());

        private:
            void ClearLines();
            void AddCapsuleRing(const AZ::Vector3& center, float length, float capsuleRadius, float internalRadius, float y, float dy, const AZ::Quaternion& rotation, AZ::u32 numSegments = 16);
            void GenerateJointLimitVisualizationData(const AZ::Vector3 positionOffset, const AZ::Quaternion& localRotation, float scale, float swingLimitDegreesX, float swingLimitDegreesY, AZ::u32 numAngularSubDivs, AZ::u32 numRadialSubDivs);

            ActorInstance* m_actorInstance;
            AZStd::recursive_mutex m_mutex;
            AZStd::vector<Line> m_lines;
            AZStd::vector<AZ::Vector3> m_tempPositions;
        };

        using ActorInstanceDataSet = AZStd::unordered_map<ActorInstance*, ActorInstanceData*>;

        virtual ~DebugDraw();

        void Clear();
        void Lock();
        void Unlock();

        ActorInstanceData* GetActorInstanceData(ActorInstance* actorInstance);
        const ActorInstanceDataSet& GetActorInstanceData() const;

    private:
        friend class ActorInstance;

        ActorInstanceData* RegisterActorInstance(ActorInstance* actorInstance);
        void UnregisterActorInstance(ActorInstance* actorInstance);

        ActorInstanceDataSet m_actorInstanceData;
        AZStd::recursive_mutex m_mutex;
    };
} // namespace EMotionFX
