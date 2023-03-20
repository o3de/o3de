/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(DebugDraw, Integration::EMotionFXAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(DebugDraw::ActorInstanceData, Integration::EMotionFXAllocator)

    void DebugDraw::ActorInstanceData::Clear()
    {
        Lock();
        ClearLines();
        Unlock();
    }

    void DebugDraw::ActorInstanceData::Lock()
    {
        m_mutex.lock();
    }

    void DebugDraw::ActorInstanceData::Unlock()
    {
        m_mutex.unlock();
    }

    void DebugDraw::ActorInstanceData::ClearLines()
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        m_lines.clear();
    }

    //-------------------------------------------------------------------------

    DebugDraw::~DebugDraw()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        for (auto& data : m_actorInstanceData)
        {
            delete data.second;
        }
    }

    void DebugDraw::Lock()
    {
        m_mutex.lock();
    }

    void DebugDraw::Unlock()
    {
        m_mutex.unlock();
    }

    void DebugDraw::Clear()
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        for (auto& data : m_actorInstanceData)
        {
            data.second->Clear();
        }
    }

    DebugDraw::ActorInstanceData* DebugDraw::RegisterActorInstance(ActorInstance* actorInstance)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        AZ_Assert(m_actorInstanceData.find(actorInstance) == m_actorInstanceData.end(), "This actor instance has already been registered.");
        ActorInstanceData* newData = aznew ActorInstanceData(actorInstance);
        m_actorInstanceData.insert(AZStd::make_pair(actorInstance, newData));
        return newData;
    }

    void DebugDraw::UnregisterActorInstance(ActorInstance* actorInstance)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        const auto it = m_actorInstanceData.find(actorInstance);
        if (it != m_actorInstanceData.end())
        {
            delete it->second;
            m_actorInstanceData.erase(it);
        }
    }

    DebugDraw::ActorInstanceData* DebugDraw::GetActorInstanceData(ActorInstance* actorInstance)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        const auto it = m_actorInstanceData.find(actorInstance);
        if (it != m_actorInstanceData.end())
        {
            return it->second;
        }
        return RegisterActorInstance(actorInstance);
    }

    const DebugDraw::ActorInstanceDataSet& DebugDraw::GetActorInstanceData() const
    {
        return m_actorInstanceData;
    }

    void DebugDraw::ActorInstanceData::DrawPose(const Pose& pose, const AZ::Color& color, const AZ::Vector3& offset)
    {
        const Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();

        const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const size_t nodeIndex = m_actorInstance->GetEnabledNode(i);
            const size_t parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();
            if (parentIndex != InvalidIndex)
            {
                const AZ::Vector3& startPos = pose.GetWorldSpaceTransform(nodeIndex).m_position;
                const AZ::Vector3& endPos = pose.GetWorldSpaceTransform(parentIndex).m_position;
                DrawLine(offset + startPos, offset + endPos, color);
            }
        }
    }

    void DebugDraw::ActorInstanceData::DrawWireframeSphere(const AZ::Vector3& center, float radius, const AZ::Color& color, const AZ::Quaternion& jointRotation, AZ::u32 numSegments, AZ::u32 numRings)
    {
        numRings = AZ::GetClamp<AZ::u32>(numRings, 4, 128);
        numSegments = AZ::GetClamp<AZ::u32>(numSegments, 4, 128);

        m_tempPositions.clear();
        m_tempPositions.reserve(numSegments * numRings);

        const float ringIncr = 1.0f / static_cast<float>(numRings - 1);
        const float segIncr = 1.0f / static_cast<float>(numSegments - 1);
        for (AZ::u32 r = 0; r < numRings; r++)
        {
            const float v = r * ringIncr;
            for (AZ::u32 s = 0; s < numSegments; s++)
            {
                const float u = 1.0f - s * segIncr;
                const float x = sinf(AZ::Constants::TwoPi * u) * sinf(AZ::Constants::Pi * v);
                const float y = sinf(AZ::Constants::Pi * (v - 0.5f));
                const float z = cosf(AZ::Constants::TwoPi * u) * sinf(AZ::Constants::Pi * v);
                const AZ::Vector3 pos(x * radius, y * radius, z * radius);
                m_tempPositions.emplace_back(center + jointRotation.TransformVector(pos));
            }
        }

        for (AZ::u32 r = 0; r < numRings - 1; r++)
        {
            for (AZ::u32 s = 0; s < numSegments - 1; s++)
            {
                const size_t indexA = r * numSegments + s;
                const size_t indexB = (r + 1) * numSegments + s;
                const size_t indexC = (r + 1) * numSegments + (s + 1);
                const size_t indexD = r * numSegments + (s + 1);

                DrawLine(m_tempPositions[indexA], m_tempPositions[indexB], color);
                DrawLine(m_tempPositions[indexB], m_tempPositions[indexC], color);
                DrawLine(m_tempPositions[indexC], m_tempPositions[indexD], color);
                DrawLine(m_tempPositions[indexD], m_tempPositions[indexA], color);
            }
        }
    }

    void DebugDraw::ActorInstanceData::DrawWireframeCapsule(const AZ::Vector3& start, const AZ::Vector3& end, float radius, const AZ::Color& color, AZ::u32 numBodySubDivs, AZ::u32 numSideSubDivs)
    {
        m_tempPositions.clear();

        const float length = (start - end).GetLength();
        const AZ::Vector3 center = (start + end) * 0.5f;
        const AZ::Vector3 direction = (end - start).GetNormalizedSafe();
        const AZ::Quaternion finalRotation = AZ::Quaternion::CreateShortestArc(AZ::Vector3(0.0f, 1.0f, 0.0f), direction);

        numSideSubDivs *= 2;
        const AZ::u32 numSegments = 16;
        const AZ::u32 ringsBody = numBodySubDivs + 1;
        const AZ::u32 ringsTotal = numSideSubDivs + ringsBody;
        m_tempPositions.reserve(numSegments * ringsTotal);

        const float ringIncr = 1.0f / static_cast<float>(numSideSubDivs - 1);
        const AZ::u32 halfSideSubDivs = numSideSubDivs / 2;
        for (AZ::u32 r = 0; r < halfSideSubDivs; r++)
        {
            const float internalRadius = sinf(AZ::Constants::Pi * static_cast<float>(r) * ringIncr);
            const float y = sinf(AZ::Constants::Pi * (r * ringIncr - 0.5f));
            AddCapsuleRing(center, length, radius, internalRadius, y, -0.5f, finalRotation, numSegments);
        }

        const float bodyIncr = 1.0f / static_cast<float>(ringsBody - 1);
        for (AZ::u32 r = 0; r < ringsBody; r++)
        {
            AddCapsuleRing(center, length, radius, 1.0f, 0.0f, static_cast<float>(r) * bodyIncr - 0.5f, finalRotation, numSegments);
        }

        for (AZ::u32 r = halfSideSubDivs; r < numSideSubDivs; r++)
        {
            const float internalRadius = sinf(AZ::Constants::Pi * r * ringIncr);
            const float y = sinf(AZ::Constants::Pi * (static_cast<float>(r) * ringIncr - 0.5f));
            AddCapsuleRing(center, length, radius, internalRadius, y, 0.5f, finalRotation, numSegments);
        }

        for (AZ::u32 r = 0; r < ringsTotal - 1; r++)
        {
            for (AZ::u32 s = 0; s < numSegments - 1; s++)
            {
                const size_t indexA = r * numSegments + s;
                const size_t indexB = (r + 1) * numSegments + s;
                const size_t indexC = (r + 1) * numSegments + (s + 1);
                const size_t indexD = r * numSegments + (s + 1);

                DrawLine(m_tempPositions[indexA], m_tempPositions[indexB], color);
                DrawLine(m_tempPositions[indexB], m_tempPositions[indexC], color);
                DrawLine(m_tempPositions[indexC], m_tempPositions[indexD], color);
                DrawLine(m_tempPositions[indexD], m_tempPositions[indexA], color);
            }
        }
    }

    void DebugDraw::ActorInstanceData::AddCapsuleRing(const AZ::Vector3& center, float length, float capsuleRadius, float internalRadius, float y, float dy, const AZ::Quaternion& rotation, AZ::u32 numSegments)
    {
        const float segIncr = 1.0f / static_cast<float>(numSegments - 1);
        for (AZ::u32 s = 0; s < numSegments; s++)
        {
            const float t = AZ::Constants::TwoPi * static_cast<float>(s) * segIncr;
            const float x = cosf(t) * internalRadius;
            const float z = sinf(t) * internalRadius;
            const AZ::Vector3 pos(x * capsuleRadius, y * capsuleRadius + length * dy, z * capsuleRadius);
            m_tempPositions.emplace_back(center + rotation.TransformVector(pos));
        }
    }

    void DebugDraw::ActorInstanceData::DrawMarker(const AZ::Vector3& position, const AZ::Color& color, float scale)
    {
        DrawLine(position + AZ::Vector3(0.0f, 0.0f, -scale), position + AZ::Vector3(0.0f, 0.0f, scale), color);
        DrawLine(position + AZ::Vector3(0.0f, -scale, 0.0f), position + AZ::Vector3(0.0f, scale, 0.0f), color);
        DrawLine(position + AZ::Vector3(-scale, 0.0f, 0.0f), position + AZ::Vector3(scale, 0.0f, 0.0f), color);
    }

    void DebugDraw::ActorInstanceData::DrawWireframeJointLimitCone(const AZ::Vector3& positionOffset, const AZ::Vector3& direction, float scale, float swingLimitDegreesX, float swingLimitDegreesY, const AZ::Color& color, AZ::u32 numAngularSubDivs, AZ::u32 numRadialSubDivs)
    {
        const AZ::Quaternion rotation = AZ::Quaternion::CreateShortestArc(AZ::Vector3(1.0f, 0.0f, 0.0f), direction.GetNormalizedSafe());
        GenerateJointLimitVisualizationData(positionOffset, rotation, scale, swingLimitDegreesX, swingLimitDegreesY, numAngularSubDivs, numRadialSubDivs);

        const size_t numLineBufferEntries = m_tempPositions.size();
        for (size_t i = 0; i < numLineBufferEntries; i += 2)
        {
            DrawLine(m_tempPositions[i], m_tempPositions[i + 1], color);
        }
    }

    void DebugDraw::ActorInstanceData::GenerateJointLimitVisualizationData(const AZ::Vector3 positionOffset, const AZ::Quaternion& localRotation, float scale, float swingLimitDegreesX, float swingLimitDegreesY, AZ::u32 numAngularSubDivs, AZ::u32 numRadialSubDivs)
    {
        const AZ::u32 angularSubdivisionsClamped = AZ::GetClamp(numAngularSubDivs, 4u, 32u);
        const AZ::u32 radialSubdivisionsClamped = AZ::GetClamp(numRadialSubDivs, 1u, 4u);

        const float swingLimitY = AZ::DegToRad(swingLimitDegreesX);
        const float swingLimitZ = AZ::DegToRad(swingLimitDegreesY);

        const AZ::u32 numLinesSwingCone = angularSubdivisionsClamped * (1 + radialSubdivisionsClamped);
        m_tempPositions.clear();
        m_tempPositions.reserve(2 * numLinesSwingCone);

        // the orientation quat for a radial line in the cone can be represented in terms of sin and cos half angles
        // these expressions can be efficiently calculated using tan quarter angles as follows:
        // writing t = tan(x / 4)
        // sin(x / 2) = 2 * t / (1 + t * t)
        // cos(x / 2) = (1 - t * t) / (1 + t * t)
        const float tanQuarterSwingZ = tanf(0.25f * swingLimitZ);
        const float tanQuarterSwingY = tanf(0.25f * swingLimitY);

        AZ::Vector3 previousRadialVector = AZ::Vector3::CreateZero();
        for (AZ::u32 angularIndex = 0; angularIndex <= angularSubdivisionsClamped; angularIndex++)
        {
            const float angle = AZ::Constants::TwoPi / angularSubdivisionsClamped * angularIndex;

            // the axis about which to rotate the x-axis to get the radial vector for this segment of the cone
            const AZ::Vector3 rotationAxis(0, -tanQuarterSwingY * sinf(angle), tanQuarterSwingZ * cosf(angle));
            const float normalizationFactor = rotationAxis.GetLengthSq();
            const AZ::Quaternion radialVectorRotation = 1.0f / (1.0f + normalizationFactor) * AZ::Quaternion::CreateFromVector3AndValue(2.0f * rotationAxis, 1.0f - normalizationFactor);
            const AZ::Vector3 radialVector = (localRotation * radialVectorRotation).TransformVector(AZ::Vector3::CreateAxisX(scale));

            if (angularIndex > 0)
            {
                for (AZ::u32 radialIndex = 1; radialIndex <= radialSubdivisionsClamped; radialIndex++)
                {
                    const float radiusFraction = 1.0f / radialSubdivisionsClamped * radialIndex;
                    m_tempPositions.emplace_back(radiusFraction * radialVector + positionOffset);
                    m_tempPositions.emplace_back(radiusFraction * previousRadialVector + positionOffset);
                }
            }

            if (angularIndex < angularSubdivisionsClamped)
            {
                m_tempPositions.emplace_back(positionOffset);
                m_tempPositions.emplace_back(radialVector + positionOffset);
            }

            previousRadialVector = radialVector;
        }
    }

} // namespace EMotionFX
