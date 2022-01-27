/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Outcome/Outcome.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Algorithms.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphSetupInstance.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/TransformData.h>

#include <EMotionFX/Source/Importer/SharedFileFormatStructs.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <MCore/Source/LogManager.h>

namespace EMotionFX
{
    NonUniformMotionData::~NonUniformMotionData()
    {
        ClearAllData();
    }

    MotionData* NonUniformMotionData::CreateNew() const
    {
        return aznew NonUniformMotionData();
    }

    const char* NonUniformMotionData::GetSceneSettingsName() const
    {
        return "Reduced Keyframes (slower, mostly smaller)";
    }

    template <class RT, class T>
    RT CalculateInterpolatedValue(const NonUniformMotionData::KeyTrack<T>& track, float sampleTime)
    {
        float t;
        size_t indexA;
        size_t indexB;
        MotionData::CalculateInterpolationIndicesNonUniform(track.m_times, sampleTime, indexA, indexB, t);
        const AZStd::vector<T>& values = track.m_values;
        return AZ::Lerp(values[indexA], values[indexB], t);
    }

    template <>
    AZ::Vector3 CalculateInterpolatedValue(const NonUniformMotionData::KeyTrack<AZ::Vector3>& track, float sampleTime)
    {
        float t;
        size_t indexA;
        size_t indexB;
        MotionData::CalculateInterpolationIndicesNonUniform(track.m_times, sampleTime, indexA, indexB, t);
        const AZStd::vector<AZ::Vector3>& values = track.m_values;
        return values[indexA].Lerp(values[indexB], t);
    }

    template <>
    AZ::Quaternion CalculateInterpolatedValue(const NonUniformMotionData::KeyTrack<MCore::Compressed16BitQuaternion>& track, float sampleTime)
    {
        float t;
        size_t indexA;
        size_t indexB;
        MotionData::CalculateInterpolationIndicesNonUniform(track.m_times, sampleTime, indexA, indexB, t);
        const AZStd::vector<MCore::Compressed16BitQuaternion>& values = track.m_values;
        return values[indexA].ToQuaternion().NLerp(values[indexB].ToQuaternion(), t);
    }

    Transform NonUniformMotionData::SampleJointTransform(const MotionDataSampleSettings& settings, size_t jointSkeletonIndex) const
    {
        const Actor* actor = settings.m_actorInstance->GetActor();
        const MotionLinkData* motionLinkData = FindMotionLinkData(actor);

        const size_t jointDataIndex = motionLinkData->GetJointDataLinks()[jointSkeletonIndex];
        if (m_additive && jointDataIndex == InvalidIndex)
        {
            return Transform::CreateIdentity();
        }

        // Sample the interpolated data.
        Transform result;
        const bool inPlace = (settings.m_inPlace && jointSkeletonIndex == actor->GetMotionExtractionNodeIndex());
        if (jointDataIndex != InvalidIndex && !inPlace)
        {
            const JointData& jointData = m_jointData[jointDataIndex];
            result.m_position = (!jointData.m_positionTrack.m_times.empty()) ? CalculateInterpolatedValue<AZ::Vector3, AZ::Vector3>(jointData.m_positionTrack, settings.m_sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_position;
            result.m_rotation = (!jointData.m_rotationTrack.m_times.empty()) ? CalculateInterpolatedValue<AZ::Quaternion, MCore::Compressed16BitQuaternion>(jointData.m_rotationTrack, settings.m_sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_rotation;
#ifndef EMFX_SCALE_DISABLED
            result.m_scale = (!jointData.m_scaleTrack.m_times.empty()) ? CalculateInterpolatedValue<AZ::Vector3, AZ::Vector3>(jointData.m_scaleTrack, settings.m_sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_scale;
#endif
        }
        else
        {
            if (settings.m_inputPose && !inPlace)
            {
                result = settings.m_inputPose->GetLocalSpaceTransform(jointSkeletonIndex);
            }
            else
            {
                result = settings.m_actorInstance->GetTransformData()->GetBindPose()->GetLocalSpaceTransform(jointSkeletonIndex);
            }
        }

        // Apply retargeting.
        if (settings.m_retarget)
        {
            BasicRetarget(settings.m_actorInstance, motionLinkData, jointSkeletonIndex, result);
        }

        // Apply runtime motion mirroring.
        if (settings.m_mirror && actor->GetHasMirrorInfo())
        {
            const Pose* bindPose = settings.m_actorInstance->GetTransformData()->GetBindPose();
            const Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(jointSkeletonIndex);
            Transform mirrored = bindPose->GetLocalSpaceTransform(jointSkeletonIndex);
            AZ::Vector3 mirrorAxis = AZ::Vector3::CreateZero();
            mirrorAxis.SetElement(mirrorInfo.m_axis, 1.0f);
            const AZ::u16 motionSource = actor->GetNodeMirrorInfo(jointSkeletonIndex).m_sourceNode;
            mirrored.ApplyDeltaMirrored(bindPose->GetLocalSpaceTransform(motionSource), result, mirrorAxis, mirrorInfo.m_flags);
            result = mirrored;
        }

        return result;
    }

    void NonUniformMotionData::SamplePose(const MotionDataSampleSettings& settings, Pose* outputPose) const
    {
        AZ_Assert(settings.m_actorInstance, "Expecting a valid actor instance.");
        const Actor* actor = settings.m_actorInstance->GetActor();
        const MotionLinkData* motionLinkData = FindMotionLinkData(actor);
       
        const ActorInstance* actorInstance = settings.m_actorInstance;
        const Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();
        const size_t numNodes = actorInstance->GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const uint16 jointIndex = actorInstance->GetEnabledNode(i);
            const size_t jointDataIndex = motionLinkData->GetJointDataLinks()[jointIndex];
            const bool inPlace = (settings.m_inPlace && jointIndex == actor->GetMotionExtractionNodeIndex());

            // Sample the interpolated data.
            Transform result;
            if (jointDataIndex != InvalidIndex && !inPlace)
            {
                const JointData& jointData = m_jointData[jointDataIndex];
                result.m_position = (!jointData.m_positionTrack.m_times.empty()) ? CalculateInterpolatedValue<AZ::Vector3>(jointData.m_positionTrack, settings.m_sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_position;
                result.m_rotation = (!jointData.m_rotationTrack.m_times.empty()) ? CalculateInterpolatedValue<AZ::Quaternion>(jointData.m_rotationTrack, settings.m_sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_rotation;
#ifndef EMFX_SCALE_DISABLED
                result.m_scale = (!jointData.m_scaleTrack.m_times.empty()) ? CalculateInterpolatedValue<AZ::Vector3>(jointData.m_scaleTrack, settings.m_sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_scale;
#endif
            }
            else
            {
                if (m_additive && jointDataIndex == InvalidIndex)
                {
                    result = Transform::CreateIdentity();
                }
                else
                {
                    if (settings.m_inputPose && !inPlace)
                    {
                        result = settings.m_inputPose->GetLocalSpaceTransform(jointIndex);
                    }
                    else
                    {
                        result = bindPose->GetLocalSpaceTransform(jointIndex);
                    }
                }
            }

            // Apply retargeting.
            if (settings.m_retarget)
            {
                BasicRetarget(settings.m_actorInstance, motionLinkData, jointIndex, result);
            }

            outputPose->SetLocalSpaceTransformDirect(jointIndex, result);
        }

        // Apply runtime motion mirroring.
        if (settings.m_mirror && actor->GetHasMirrorInfo())
        {
            outputPose->Mirror(motionLinkData);
        }

        // Output morph target weights.
        const MorphSetupInstance* morphSetup = actorInstance->GetMorphSetupInstance();
        const size_t numMorphTargets = morphSetup->GetNumMorphTargets();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            const AZ::u32 morphTargetId = morphSetup->GetMorphTarget(i)->GetID();
            const AZ::Outcome<size_t> morphIndex = FindMorphIndexByNameId(morphTargetId);
            if (morphIndex.IsSuccess())
            {
                const size_t realIndex = morphIndex.GetValue();
                const FloatData& data = m_morphData[realIndex];
                const auto& track = data.m_track;
                if (!track.m_times.empty())
                {
                    const float interpolated = CalculateInterpolatedValue<float, float>(track, settings.m_sampleTime);
                    outputPose->SetMorphWeight(i, interpolated);
                }
                else
                {
                    outputPose->SetMorphWeight(i, m_staticMorphData[realIndex].m_staticValue);
                }
            }
            else
            {
                if (settings.m_inputPose)
                {
                    outputPose->SetMorphWeight(i, settings.m_inputPose->GetMorphWeight(i));
                }
                else
                {
                    outputPose->SetMorphWeight(i, bindPose->GetMorphWeight(i));
                }
            }
        }

        // TODO: output float curves once we have that system in place inside the poses etc

        // Since we used the SetLocalTransformDirect, make sure we manually invalidate all model space transforms.
        outputPose->InvalidateAllModelSpaceTransforms();
    }

    float NonUniformMotionData::SampleMorph(float sampleTime, size_t morphDataIndex) const
    {
        return (!m_morphData[morphDataIndex].m_track.m_times.empty()) ? CalculateInterpolatedValue<float, float>(m_morphData[morphDataIndex].m_track, sampleTime) : m_staticMorphData[morphDataIndex].m_staticValue;
    }

    float NonUniformMotionData::SampleFloat(float sampleTime, size_t floatDataIndex) const
    {
        return (!m_floatData[floatDataIndex].m_track.m_times.empty()) ? CalculateInterpolatedValue<float, float>(m_floatData[floatDataIndex].m_track, sampleTime) : m_staticFloatData[floatDataIndex].m_staticValue;
    }

    bool NonUniformMotionData::VerifyStartEndTimeIntegrity(const AZStd::vector<float>& timeValues, bool& firstCheck, float& startTime, float& endTime) const
    {
        if (firstCheck && !timeValues.empty())
        {
            startTime = timeValues.front();
            endTime = timeValues.back();
            firstCheck = false;
        }
        else if (!timeValues.empty())
        {
            if (!AZ::IsClose(timeValues.front(), startTime, AZ::Constants::FloatEpsilon))
            {
                AZ_Error("EMotionFX", false, "No keyframe present at the start of the animation (%f). The first keyframe is at %f.",
                    startTime, timeValues.front());
                return false;
            }
            if (!AZ::IsClose(timeValues.back(), endTime, AZ::Constants::FloatEpsilon))
            {
                AZ_Error("EMotionFX", false, "No keyframe present at the end of the animation (%f). The last keyframe is at %f.",
                    endTime, timeValues.back());
                return false;
            }
        }

        return true;
    }

    bool NonUniformMotionData::VerifyIntegrity() const
    {
        for (const JointData& jointData : m_jointData)
        {
            if (jointData.m_positionTrack.m_times.size() != jointData.m_positionTrack.m_values.size())
            {
                AZ_Error("EMotionFX", false, "Number of position keyframe times (%d) does not match the number of keyframe values (%d).",
                    jointData.m_positionTrack.m_times.size(), jointData.m_positionTrack.m_values.size());
                return false;
            }

            if (jointData.m_rotationTrack.m_times.size() != jointData.m_rotationTrack.m_values.size())
            {
                AZ_Error("EMotionFX", false, "Number of rotation keyframe times (%d) does not match the number of keyframe values (%d).",
                    jointData.m_rotationTrack.m_times.size(), jointData.m_rotationTrack.m_values.size());
                return false;
            }

#ifndef EMFX_SCALE_DISABLED
            if (jointData.m_scaleTrack.m_times.size() != jointData.m_scaleTrack.m_values.size())
            {
                AZ_Error("EMotionFX", false, "Number of scale keyframe times (%d) does not match the number of keyframe values (%d).",
                    jointData.m_scaleTrack.m_times.size(), jointData.m_scaleTrack.m_values.size());
                return false;
            }

            if (!VerifyKeyTrackTimeIntegrity(jointData.m_scaleTrack.m_times))
            {
                return false;
            }
#endif

            if (!VerifyKeyTrackTimeIntegrity(jointData.m_positionTrack.m_times) ||
                !VerifyKeyTrackTimeIntegrity(jointData.m_rotationTrack.m_times))
            {
                return false;
            }
        }

        // Verify that the start and end times are all matching up.
        float startTime = -1.0f;
        float endTime = -1.0f;
        bool firstCheck = true;
        for (const JointData& jointData : m_jointData)
        {
            if (!VerifyStartEndTimeIntegrity(jointData.m_positionTrack.m_times, firstCheck, startTime, endTime) ||
                !VerifyStartEndTimeIntegrity(jointData.m_rotationTrack.m_times, firstCheck, startTime, endTime))
            {
                return false;
            }

#ifndef EMFX_SCALE_DISABLED
            if (!VerifyStartEndTimeIntegrity(jointData.m_scaleTrack.m_times, firstCheck, startTime, endTime))
            {
                return false;
            }
#endif
        }

        for (const FloatData& morphData : m_morphData)
        {
            if (morphData.m_track.m_times.size() != morphData.m_track.m_values.size())
            {
                AZ_Error("EMotionFX", false, "Number of morph keyframe times (%d) does not match the number of keyframe values (%d).",
                    morphData.m_track.m_times.size(), morphData.m_track.m_values.size());
                return false;
            }

            if (!VerifyStartEndTimeIntegrity(morphData.m_track.m_times, firstCheck, startTime, endTime))
            {
                return false;
            }

            if (!VerifyKeyTrackTimeIntegrity(morphData.m_track.m_times))
            {
                return false;
            }
        }

        for (const FloatData& floatData : m_floatData)
        {
            if (floatData.m_track.m_times.size() != floatData.m_track.m_values.size())
            {
                AZ_Error("EMotionFX", false, "Number of float keyframe times (%d) does not match the number of keyframe values (%d).",
                    floatData.m_track.m_times.size(), floatData.m_track.m_values.size());
                return false;
            }
            if (!VerifyStartEndTimeIntegrity(floatData.m_track.m_times, firstCheck, startTime, endTime))
            {
                return false;
            }
            if (!VerifyKeyTrackTimeIntegrity(floatData.m_track.m_times))
            {
                return false;
            }
        }

        return true;
    }

    template<class KeyTrackType>
    void NonUniformMotionData::FixMissingEndKeyframes(KeyTrackType& keytrack, float endTimeToMatch)
    {
        if (keytrack.m_times.empty() || keytrack.m_values.empty())
        {
            return;
        }

        if (!AZ::IsClose(keytrack.m_times.back(), endTimeToMatch, AZ::Constants::FloatEpsilon))
        {
            keytrack.m_times.emplace_back(endTimeToMatch);
            keytrack.m_values.emplace_back(keytrack.m_values.back());
        }
    }

    void NonUniformMotionData::FixMissingEndKeyframes()
    {
        UpdateDuration();

        for (JointData& jointData : m_jointData)
        {
            FixMissingEndKeyframes(jointData.m_positionTrack, m_duration);
            FixMissingEndKeyframes(jointData.m_rotationTrack, m_duration);

#ifndef EMFX_SCALE_DISABLED
            FixMissingEndKeyframes(jointData.m_scaleTrack, m_duration);
#endif
        }

        for (FloatData& morphData : m_morphData)
        {
            FixMissingEndKeyframes(morphData.m_track, m_duration);
        }

        for (FloatData& floatData : m_floatData)
        {
            FixMissingEndKeyframes(floatData.m_track, m_duration);
        }
    }

    void NonUniformMotionData::UpdateDuration()
    {
        m_duration = 0.0f;

        for (const JointData& jointData : m_jointData)
        {
            if (!jointData.m_positionTrack.m_times.empty())
            {
                m_duration = AZ::GetMax(m_duration, jointData.m_positionTrack.m_times.back());
            }

            if (!jointData.m_rotationTrack.m_times.empty())
            {
                m_duration = AZ::GetMax(m_duration, jointData.m_rotationTrack.m_times.back());
            }

#ifndef EMFX_SCALE_DISABLED
            if (!jointData.m_scaleTrack.m_times.empty())
            {
                m_duration = AZ::GetMax(m_duration, jointData.m_scaleTrack.m_times.back());
            }
#endif
        }

        for (const FloatData& morphData : m_morphData)
        {
            if (!morphData.m_track.m_times.empty())
            {
                m_duration = AZ::GetMax(m_duration, morphData.m_track.m_times.back());
            }
        }

        for (const FloatData& floatData : m_floatData)
        {
            if (!floatData.m_track.m_times.empty())
            {
                m_duration = AZ::GetMax(m_duration, floatData.m_track.m_times.back());
            }
        }
    }

    void NonUniformMotionData::AllocateJointPositionSamples(size_t jointDataIndex, size_t numSamples)
    {
        m_jointData[jointDataIndex].m_positionTrack.m_times.resize(numSamples);
        m_jointData[jointDataIndex].m_positionTrack.m_values.resize(numSamples);
    }

    void NonUniformMotionData::AllocateJointRotationSamples(size_t jointDataIndex, size_t numSamples)
    {
        m_jointData[jointDataIndex].m_rotationTrack.m_times.resize(numSamples);
        m_jointData[jointDataIndex].m_rotationTrack.m_values.resize(numSamples);
    }

#ifndef EMFX_SCALE_DISABLED
    void NonUniformMotionData::AllocateJointScaleSamples(size_t jointDataIndex, size_t numSamples)
    {
        m_jointData[jointDataIndex].m_scaleTrack.m_times.resize(numSamples);
        m_jointData[jointDataIndex].m_scaleTrack.m_values.resize(numSamples);
    }
#endif

    void NonUniformMotionData::AllocateMorphSamples(size_t morphDataIndex, size_t numSamples)
    {
        m_morphData[morphDataIndex].m_track.m_times.resize(numSamples);
        m_morphData[morphDataIndex].m_track.m_values.resize(numSamples);
    }

    void NonUniformMotionData::AllocateFloatSamples(size_t floatDataIndex, size_t numSamples)
    {
        m_floatData[floatDataIndex].m_track.m_times.resize(numSamples);
        m_floatData[floatDataIndex].m_track.m_values.resize(numSamples);
    }

    bool NonUniformMotionData::IsJointPositionAnimated(size_t jointDataIndex) const
    {
        return !m_jointData[jointDataIndex].m_positionTrack.m_times.empty();
    }

    bool NonUniformMotionData::IsJointRotationAnimated(size_t jointDataIndex) const
    {
        return !m_jointData[jointDataIndex].m_rotationTrack.m_times.empty();
    }

#ifndef EMFX_SCALE_DISABLED
    bool NonUniformMotionData::IsJointScaleAnimated(size_t jointDataIndex) const
    {
        return !m_jointData[jointDataIndex].m_scaleTrack.m_times.empty();
    }
#endif

    bool NonUniformMotionData::IsJointAnimated(size_t jointDataIndex) const
    {
        const JointData& jointData = m_jointData[jointDataIndex];

#ifndef EMFX_SCALE_DISABLED
        return (!jointData.m_positionTrack.m_times.empty() || !jointData.m_rotationTrack.m_times.empty() || !jointData.m_scaleTrack.m_times.empty());
#else
        return (!jointData.m_positionTrack.m_times.empty() || !jointData.m_rotationTrack.m_times.empty());
#endif
    }

    MotionData::Vector3Key NonUniformMotionData::GetJointPositionSample(size_t jointDataIndex, size_t sampleIndex) const
    {
        const KeyTrack<AZ::Vector3>& track = m_jointData[jointDataIndex].m_positionTrack;
        return { track.m_times[sampleIndex], track.m_values[sampleIndex] };
    }

    MotionData::QuaternionKey NonUniformMotionData::GetJointRotationSample(size_t jointDataIndex, size_t sampleIndex) const
    {
        const KeyTrack<MCore::Compressed16BitQuaternion>& track = m_jointData[jointDataIndex].m_rotationTrack;
        return { track.m_times[sampleIndex], track.m_values[sampleIndex].ToQuaternion() };
    }

#ifndef EMFX_SCALE_DISABLED
    MotionData::Vector3Key NonUniformMotionData::GetJointScaleSample(size_t jointDataIndex, size_t sampleIndex) const
    {
        const KeyTrack<AZ::Vector3>& track = m_jointData[jointDataIndex].m_scaleTrack;
        return { track.m_times[sampleIndex], track.m_values[sampleIndex] };
    }
#endif

    MotionData::FloatKey NonUniformMotionData::GetMorphSample(size_t morphDataIndex, size_t sampleIndex) const
    {
        const KeyTrack<float>& track = m_morphData[morphDataIndex].m_track;
        return { track.m_times[sampleIndex], track.m_values[sampleIndex] };
    }

    MotionData::FloatKey NonUniformMotionData::GetFloatSample(size_t floatDataIndex, size_t sampleIndex) const
    {
        const KeyTrack<float>& track = m_floatData[floatDataIndex].m_track;
        return { track.m_times[sampleIndex], track.m_values[sampleIndex] };
    }

    bool NonUniformMotionData::IsMorphAnimated(size_t morphDataIndex) const
    {
        return !m_morphData[morphDataIndex].m_track.m_times.empty();
    }

    bool NonUniformMotionData::IsFloatAnimated(size_t floatDataIndex) const
    {
        return !m_floatData[floatDataIndex].m_track.m_times.empty();
    }

    void NonUniformMotionData::ClearAllJointTransformSamples()
    {
        const size_t numJoints = m_jointData.size();
        for (size_t i = 0; i < numJoints; ++i)
        {
            ClearJointTransformSamples(i);
        }
    }

    void NonUniformMotionData::ClearAllMorphSamples()
    {
        for (FloatData& data : m_morphData)
        {
            data.m_track.m_times.clear();
            data.m_track.m_values.clear();
        }
    }

    void NonUniformMotionData::ClearAllFloatSamples()
    {
        for (FloatData& data : m_floatData)
        {
            data.m_track.m_times.clear();
            data.m_track.m_values.clear();
        }
    }

    void NonUniformMotionData::ClearJointPositionSamples(size_t jointDataIndex)
    {
        auto& track = m_jointData[jointDataIndex].m_positionTrack;
        track.m_times.clear();
        track.m_values.clear();
    }

    void NonUniformMotionData::ClearJointRotationSamples(size_t jointDataIndex)
    {
        auto& track = m_jointData[jointDataIndex].m_rotationTrack;
        track.m_times.clear();
        track.m_values.clear();
    }

#ifndef EMFX_SCALE_DISABLED
    void NonUniformMotionData::ClearJointScaleSamples(size_t jointDataIndex)
    {
        auto& track = m_jointData[jointDataIndex].m_scaleTrack;
        track.m_times.clear();
        track.m_values.clear();
    }
#endif

    void NonUniformMotionData::ClearJointTransformSamples(size_t jointDataIndex)
    {
        ClearJointPositionSamples(jointDataIndex);
        ClearJointRotationSamples(jointDataIndex);
#ifndef EMFX_SCALE_DISABLED
        ClearJointScaleSamples(jointDataIndex);
#endif
    }

    void NonUniformMotionData::ClearMorphSamples(size_t morphDataIndex)
    {
        auto& track = m_morphData[morphDataIndex].m_track;
        track.m_times.clear();
        track.m_values.clear();
    }

    void NonUniformMotionData::ClearFloatSamples(size_t floatDataIndex)
    {
        auto& track = m_floatData[floatDataIndex].m_track;
        track.m_times.clear();
        track.m_values.clear();
    }

    void NonUniformMotionData::SetJointPositionSample(size_t jointDataIndex, size_t sampleIndex, const Vector3Key& key)
    {
        auto& track = m_jointData[jointDataIndex].m_positionTrack;
        track.m_times[sampleIndex] = key.m_time;
        track.m_values[sampleIndex] = key.m_value;
    }

    void NonUniformMotionData::SetJointRotationSample(size_t jointDataIndex, size_t sampleIndex, const QuaternionKey& key)
    {
        auto& track = m_jointData[jointDataIndex].m_rotationTrack;
        track.m_times[sampleIndex] = key.m_time;
        track.m_values[sampleIndex] = key.m_value;
    }

#ifndef EMFX_SCALE_DISABLED
    void NonUniformMotionData::SetJointScaleSample(size_t jointDataIndex, size_t sampleIndex, const Vector3Key& key)
    {
        auto& track = m_jointData[jointDataIndex].m_scaleTrack;
        track.m_times[sampleIndex] = key.m_time;
        track.m_values[sampleIndex] = key.m_value;
    }
#endif

    void NonUniformMotionData::SetMorphSample(size_t morphDataIndex, size_t sampleIndex, const FloatKey& key)
    {
        auto& track = m_morphData[morphDataIndex].m_track;
        track.m_times[sampleIndex] = key.m_time;
        track.m_values[sampleIndex] = key.m_value;
    }

    void NonUniformMotionData::SetFloatSample(size_t floatDataIndex, size_t sampleIndex, const FloatKey& key)
    {
        auto& track = m_floatData[floatDataIndex].m_track;
        track.m_times[sampleIndex] = key.m_time;
        track.m_values[sampleIndex] = key.m_value;
    }

    void NonUniformMotionData::SetJointPositionSamples(size_t jointDataIndex, const Vector3Track& track)
    {
        m_jointData[jointDataIndex].m_positionTrack = track;
    }

    void NonUniformMotionData::SetJointRotationSamples(size_t jointDataIndex, const QuaternionTrack& track)
    {
        m_jointData[jointDataIndex].m_rotationTrack = track;
    }

#ifndef EMFX_SCALE_DISABLED
    void NonUniformMotionData::SetJointScaleSamples(size_t jointDataIndex, const Vector3Track& track)
    {
        m_jointData[jointDataIndex].m_scaleTrack = track;
    }
#endif

    size_t NonUniformMotionData::GetNumJointPositionSamples(size_t jointDataIndex) const
    {
        return m_jointData[jointDataIndex].m_positionTrack.m_times.size();
    }

    size_t NonUniformMotionData::GetNumJointRotationSamples(size_t jointDataIndex) const
    {
        return m_jointData[jointDataIndex].m_rotationTrack.m_times.size();
    }

#ifndef EMFX_SCALE_DISABLED
    size_t NonUniformMotionData::GetNumJointScaleSamples(size_t jointDataIndex) const
    {
        return m_jointData[jointDataIndex].m_scaleTrack.m_times.size();
    }
#endif
    size_t NonUniformMotionData::GetNumMorphSamples(size_t morphDataIndex) const
    {
        return m_morphData[morphDataIndex].m_track.m_times.size();
    }

    size_t NonUniformMotionData::GetNumFloatSamples(size_t floatDataIndex) const
    {
        return m_floatData[floatDataIndex].m_track.m_times.size();
    }

    bool NonUniformMotionData::VerifyKeyTrackTimeIntegrity(const AZStd::vector<float>& timeValues)
    {
        if (timeValues.empty())
        {
            return true;
        }

        float prevKeyTime = std::numeric_limits<float>::lowest();
        for (float curTime : timeValues)
        {
            if (curTime < prevKeyTime)
            {
                AZ_Error("EMotionFX", false, "Keyframe times need to be ascending. Current keyframe time (%f) is smaller than the previous (%f).",
                    curTime, prevKeyTime);
                return false;
            }
            prevKeyTime = curTime;
        }

        return true;
    }

    void NonUniformMotionData::ClearAllData()
    {
        m_jointData.clear();
        m_jointData.shrink_to_fit();
        m_morphData.clear();
        m_morphData.shrink_to_fit();
        m_floatData.clear();
        m_floatData.shrink_to_fit();
    }

    void NonUniformMotionData::RemoveJointSampleData(size_t jointDataIndex)
    {
        m_jointData.erase(m_jointData.begin() + jointDataIndex);
    }

    void NonUniformMotionData::RemoveMorphSampleData(size_t morphDataIndex)
    {
        m_morphData.erase(m_morphData.begin() + morphDataIndex);
    }

    void NonUniformMotionData::RemoveFloatSampleData(size_t floatDataIndex)
    {
        m_floatData.erase(m_floatData.begin() + floatDataIndex);
    }

    void NonUniformMotionData::ResizeSampleData(size_t numJoints, size_t numMorphs, size_t numFloats)
    {
        m_jointData.resize(numJoints);
        m_morphData.resize(numMorphs);
        m_floatData.resize(numFloats);
    }

    void NonUniformMotionData::AddJointSampleData([[maybe_unused]] size_t jointDataIndex)
    {
        AZ_Assert(jointDataIndex == m_jointData.size(), "Expected the size of the jointData vector to be a different size. Is it in sync with the m_staticJointData vector?");
        m_jointData.emplace_back();
    }

    void NonUniformMotionData::AddMorphSampleData([[maybe_unused]] size_t morphDataIndex)
    {
        AZ_Assert(morphDataIndex == m_morphData.size(), "Expected the size of the morphData vector to be a different size. Is it in sync with the m_staticMorphData vector?");
        m_morphData.emplace_back();
    }

    void NonUniformMotionData::AddFloatSampleData([[maybe_unused]] size_t floatDataIndex)
    {
        AZ_Assert(floatDataIndex == m_floatData.size(), "Expected the size of the floatData vector to be a different size. Is it in sync with the m_staticFloatData vector?");
        m_floatData.emplace_back();
    }

    template <class T>
    void ClearTrack(NonUniformMotionData::KeyTrack<T>& track)
    {
        track.m_times.clear();
        track.m_times.shrink_to_fit();
        track.m_values.clear();
        track.m_values.shrink_to_fit();
    }

    template <class T>
    size_t ReduceTrackSamples(NonUniformMotionData::KeyTrack<T>& track, const T& poseValue, float maxError)
    {
        AZ_Assert(track.m_times.size() == track.m_values.size(), "Time and value vectors have to be the same size!");

        if (track.m_times.empty())
        {
            return 0;
        }
        else
        if (track.m_times.size() == 1)
        {
           if (IsClose<T>(track.m_values[0], poseValue, 0.001f))
           {
               ClearTrack(track);
               return 1;
           }
        }
        else
        if (track.m_times.size() == 2)
        {
            if (IsClose<T>(track.m_values[0], track.m_values[1], 0.001f) && 
                IsClose<T>(track.m_values[0], poseValue, 0.001f))
            {
                ClearTrack(track);
                return 2;
            }
        }

        // Create a temparory copy of the track data we're going to optimize.
        NonUniformMotionData::KeyTrack<T> trackCopy;
        trackCopy = track;

        // Try removing every keyframe and see the impact of that.
        size_t i = 1;
        size_t numRemoved = 0;
        do
        {
            const float timeValue = track.m_times[i];

            // Remove the sample from the track.
            trackCopy.m_times.erase(trackCopy.m_times.begin() + i);
            trackCopy.m_values.erase(trackCopy.m_values.begin() + i);

            // Get the values at the given time stamp before and after keyframe removal.
            const T& v1 = track.m_values[i];
            const T v2 = CalculateInterpolatedValue<T>(trackCopy, timeValue);

            // if the value error introduced between by removing the keyframe is within a given threshold.
            if (IsClose<T>(v1, v2, maxError))
            {
                track.m_times.erase(track.m_times.begin() + i);
                track.m_values.erase(track.m_values.begin() + i);
                numRemoved++;
            }
            else // If the "visual" difference is too high and we do not want to remove the key, copy over the original keys again to restore it.
            {
                trackCopy = track;
                i++;
            }
        } while (i < track.m_times.size() - 1);

        // Remove the entire key track contents if it is just the same as the pose value.
        if (track.m_times.size() == 2)
        {
            if (IsClose<T>(track.m_values[0], track.m_values[1], 0.001f) && 
                IsClose<T>(track.m_values[0], poseValue, 0.001f))
            {
                ClearTrack(track);
                numRemoved += 2;
            }
        }

        return numRemoved;
    }

    template <>
    size_t ReduceTrackSamples(NonUniformMotionData::KeyTrack<MCore::Compressed16BitQuaternion>& track, const MCore::Compressed16BitQuaternion& poseValue, float maxError)
    {
        AZ_Assert(track.m_times.size() == track.m_values.size(), "Time and value vectors have to be the same size!");

        if (track.m_times.empty())
        {
            return 0;
        }
        else
        if (track.m_times.size() == 1)
        {
           if (IsClose<AZ::Quaternion>(track.m_values[0].ToQuaternion(), poseValue.ToQuaternion(), 0.001f))
           {
               ClearTrack(track);
               return 1;
           }
        }
        else
        if (track.m_times.size() == 2)
        {
            if (IsClose<AZ::Quaternion>(track.m_values[0].ToQuaternion(), track.m_values[1].ToQuaternion(), 0.001f) && 
                IsClose<AZ::Quaternion>(track.m_values[0].ToQuaternion(), poseValue.ToQuaternion(), 0.001f))
            {
                ClearTrack(track);
                return 2;
            }
        }

        // Create a temparory copy of the track data we're going to optimize.
        NonUniformMotionData::KeyTrack<MCore::Compressed16BitQuaternion> trackCopy;
        trackCopy = track;

        // Try removing every keyframe and see the impact of that.
        size_t i = 1;
        size_t numRemoved = 0;
        do
        {
            const float timeValue = track.m_times[i];

            // Remove the sample from the track.
            trackCopy.m_times.erase(trackCopy.m_times.begin() + i);
            trackCopy.m_values.erase(trackCopy.m_values.begin() + i);

            // Get the values at the given time stamp before and after keyframe removal.
            const AZ::Quaternion v1 = track.m_values[i].ToQuaternion();
            const AZ::Quaternion v2 = CalculateInterpolatedValue<AZ::Quaternion, MCore::Compressed16BitQuaternion>(trackCopy, timeValue);

            // if the value error introduced between by removing the keyframe is within a given threshold.
            if (IsClose<AZ::Quaternion>(v1, v2, maxError))
            {
                track.m_times.erase(track.m_times.begin() + i);
                track.m_values.erase(track.m_values.begin() + i);
                numRemoved++;
            }
            else // If the "visual" difference is too high and we do not want to remove the key, copy over the original keys again to restore it.
            {
                trackCopy = track;
                i++;
            }
        } while (i < track.m_times.size() - 1);

        // Remove the entire key track contents if it is just the same as the pose value.
        if (track.m_times.size() == 2)
        {
            if (IsClose<AZ::Quaternion>(track.m_values[0].ToQuaternion(), track.m_values[1].ToQuaternion(), 0.001f) && 
                IsClose<AZ::Quaternion>(track.m_values[0].ToQuaternion(), poseValue.ToQuaternion(), 0.001f))
            {
                ClearTrack(track);
                numRemoved += 2;
            }
        }

        return numRemoved;
    }

    template <class T>
    void LogTrack(const AZStd::string& name, const AZStd::string& channel, NonUniformMotionData::KeyTrack<T>& track)
    {
        AZ_Printf("EMotionFX", "Track (Name='%s', Channel='%s', NumKeys=%d):", name.c_str(), channel.c_str(), track.m_times.size());
        const size_t numKeys = track.m_times.size();
        for (size_t i = 0; i < numKeys; ++i)
        {
            AZ_TracePrintf("EMotionFX", "\t%d --> %.4f", i, track.m_times[i]);
        }
    }

    template <>
    void LogTrack(const AZStd::string& name, const AZStd::string& channel, NonUniformMotionData::KeyTrack<float>& track)
    {
        AZ_Printf("EMotionFX", "Float Track (Name='%s', Channel='%s', NumKeys=%d):", name.c_str(), channel.c_str(), track.m_times.size());
        const size_t numKeys = track.m_times.size();
        for (size_t i = 0; i < numKeys; ++i)
        {
            AZ_TracePrintf("EMotionFX", "\t%d --> %.4f = %.6f", i, track.m_times[i], track.m_values[i]);
        }
    }

    template <>
    void LogTrack<AZ::Vector3>([[maybe_unused]] const AZStd::string& name, [[maybe_unused]] const AZStd::string& channel, [[maybe_unused]] NonUniformMotionData::KeyTrack<AZ::Vector3>& track)
    {
#if defined(AZ_ENABLE_TRACING)
        AZ_Printf("EMotionFX", "Vector3 Track (Name='%s', Channel='%s', NumKeys=%d):", name.c_str(), channel.c_str(), track.m_times.size());
        const size_t numKeys = track.m_times.size();
        for (size_t i = 0; i < numKeys; ++i)
        {
            const AZ::Vector3& value = track.m_values[i];
            AZ_TracePrintf("EMotionFX", "\t%d --> %.4f = (%.6f, %.6f, %.6f)",
                i,
                track.m_times[i],
                static_cast<float>(value.GetX()),
                static_cast<float>(value.GetY()),
                static_cast<float>(value.GetZ()));
        }
#endif
    }

    template <>
    void LogTrack<AZ::Quaternion>([[maybe_unused]] const AZStd::string& name, [[maybe_unused]] const AZStd::string& channel, [[maybe_unused]] NonUniformMotionData::KeyTrack<AZ::Quaternion>& track)
    {
#if defined(AZ_ENABLE_TRACING)
        AZ_Printf("EMotionFX", "Quaternion Track (Name='%s', Channel='%s', NumKeys=%d):", name.c_str(), channel.c_str(), track.m_times.size());
        const size_t numKeys = track.m_times.size();
        for (size_t i = 0; i < numKeys; ++i)
        {
            const AZ::Quaternion& value = track.m_values[i];
            AZ_TracePrintf("EMotionFX", "\t%d --> %.4f = (%.6f, %.6f, %.6f, %.6f)",
                i,
                track.m_times[i],
                static_cast<float>(value.GetX()),
                static_cast<float>(value.GetY()),
                static_cast<float>(value.GetZ()),
                static_cast<float>(value.GetW()));
        }
#endif
    }

    void NonUniformMotionData::Optimize(const OptimizeSettings& settings)
    {
        // Joints.
        for (size_t i = 0; i < m_jointData.size(); ++i)
        {
            float maxPosError = settings.m_maxPosError;
            float maxRotError = settings.m_maxRotError;
            float maxScaleError = settings.m_maxScaleError;

            JointData& jointData = m_jointData[i];
            if (AZStd::find(settings.m_jointIgnoreList.begin(), settings.m_jointIgnoreList.end(), i) != settings.m_jointIgnoreList.end())
            {
                maxPosError = 0.00001f;
                maxRotError = 0.00001f;
                maxScaleError = 0.00001f;
            }

            ReduceTrackSamples<AZ::Vector3>(jointData.m_positionTrack, m_staticJointData[i].m_staticTransform.m_position, maxPosError);
            ReduceTrackSamples<MCore::Compressed16BitQuaternion>(jointData.m_rotationTrack, m_staticJointData[i].m_staticTransform.m_rotation, maxRotError);
            EMFX_SCALECODE
            (
                ReduceTrackSamples<AZ::Vector3>(jointData.m_scaleTrack, m_staticJointData[i].m_staticTransform.m_scale, maxScaleError);
            )
        }

        // Morphs.
        for (size_t i = 0; i < m_morphData.size(); ++i)
        {
            FloatData& morphData = m_morphData[i];
            if (AZStd::find(settings.m_morphIgnoreList.begin(), settings.m_morphIgnoreList.end(), i) != settings.m_morphIgnoreList.end())
            {
                continue;
            }
            ReduceTrackSamples<float>(morphData.m_track, m_staticMorphData[i].m_staticValue, settings.m_maxMorphError);
        }

        // Floats.
        for (size_t i = 0; i < m_floatData.size(); ++i)
        {
            FloatData& floatData = m_floatData[i];
            if (AZStd::find(settings.m_floatIgnoreList.begin(), settings.m_floatIgnoreList.end(), i) != settings.m_floatIgnoreList.end())
            {
                continue;
            }
            ReduceTrackSamples<float>(floatData.m_track, m_staticFloatData[i].m_staticValue, settings.m_maxFloatError);
        }

        if (settings.m_updateDuration)
        {
            UpdateDuration();
        }
    }

    void NonUniformMotionData::ScaleData(float scaleFactor)
    {
        for (JointData& jointData : m_jointData)
        {
            for (AZ::Vector3& pos : jointData.m_positionTrack.m_values)
            {
                pos *= scaleFactor;
            }
        }
    }

    const NonUniformMotionData::Vector3Track& NonUniformMotionData::GetJointPositionTrack(size_t jointDataIndex) const
    {
        return m_jointData[jointDataIndex].m_positionTrack;
    }

    const NonUniformMotionData::QuaternionTrack& NonUniformMotionData::GetJointRotationTrack(size_t jointDataIndex) const
    {
        return m_jointData[jointDataIndex].m_rotationTrack;
    }

#ifndef EMFX_SCALE_DISABLED
    const NonUniformMotionData::Vector3Track& NonUniformMotionData::GetJointScaleTrack(size_t jointDataIndex) const
    {
        return m_jointData[jointDataIndex].m_scaleTrack;
    }
#endif

    const NonUniformMotionData::FloatTrack& NonUniformMotionData::GetMorphTrack(size_t morphDataIndex) const
    {
        return m_morphData[morphDataIndex].m_track;
    }

    const NonUniformMotionData::FloatTrack& NonUniformMotionData::GetFloatTrack(size_t floatDataIndex) const
    {
        return m_floatData[floatDataIndex].m_track;
    }

    NonUniformMotionData& NonUniformMotionData::operator=(const NonUniformMotionData& sourceMotionData)
    {
        CopyBaseMotionData(&sourceMotionData);

        m_jointData = sourceMotionData.m_jointData;
        m_morphData = sourceMotionData.m_morphData;
        m_floatData = sourceMotionData.m_floatData;

        return *this;
    }

    void NonUniformMotionData::InitFromNonUniformData(const NonUniformMotionData* motionData, bool keepSameSampleRate, float newSampleRate, bool updateDuration)
    {
        AZ_Assert(newSampleRate > 0.0f, "Expected the sample rate to be larger than zero.");

        // Copy over the motion data directly in case the sample rate doesn't change.
        if (keepSameSampleRate || AZ::IsClose(newSampleRate, motionData->GetSampleRate(), AZ::Constants::FloatEpsilon))
        {
            *this = *motionData;
            return;
        }

        CopyBaseMotionData(motionData);
        SetSampleRate(newSampleRate);

        // Resample the motion data at our newly desired rate.
        // Calculate the sample spacing and number of samples required.
        float sampleSpacing = 0.0f;
        size_t numSamples = 0;
        MotionData::CalculateSampleInformation(motionData->GetDuration(), m_sampleRate, numSamples, sampleSpacing);

        // Joints.
        for (size_t i = 0; i < GetNumJoints(); ++i)
        {
            if (!motionData->IsJointAnimated(i))
            {
                continue;
            }

            // Allocate samples where needed.
            AllocateJointPositionSamples(i, numSamples);
            AllocateJointRotationSamples(i, numSamples);
            EMFX_SCALECODE
            (
                AllocateJointScaleSamples(i, numSamples);
            )

            for (size_t s = 0; s < numSamples; ++s)
            {
                const float keyTime = s * sampleSpacing;
                const Transform transform = motionData->SampleJointTransform(keyTime, i);
                SetJointPositionSample(i, s, {keyTime, transform.m_position});
                SetJointRotationSample(i, s, {keyTime, transform.m_rotation});
                EMFX_SCALECODE
                (
                    SetJointScaleSample(i, s, {keyTime, transform.m_scale});
                )
            }
        }

        // Morphs.
        for (size_t i = 0; i < GetNumMorphs(); ++i)
        {
            if (!motionData->IsMorphAnimated(i))
            {
                continue;
            }

            AllocateMorphSamples(i, numSamples);
            for (size_t s = 0; s < numSamples; ++s)
            {
                const float keyTime = s * sampleSpacing;
                const float value = motionData->SampleMorph(keyTime, i);
                SetMorphSample(i, s, {keyTime, value});
            }
        }

        // Floats.
        for (size_t i = 0; i < GetNumFloats(); ++i)
        {
            if (!motionData->IsFloatAnimated(i))
            {
                continue;
            }

            AllocateFloatSamples(i, numSamples);
            for (size_t s = 0; s < numSamples; ++s)
            {
                const float keyTime = s * sampleSpacing;
                const float value = motionData->SampleFloat(keyTime, i);
                SetFloatSample(i, s, {keyTime, value});
            }
        }

        RemoveRedundantKeyframes(updateDuration);
    }

    AZ::Vector3 NonUniformMotionData::SampleJointPosition(float sampleTime, size_t jointDataIndex) const
    {
        return !m_jointData[jointDataIndex].m_positionTrack.m_times.empty() ? CalculateInterpolatedValue<AZ::Vector3, AZ::Vector3>(m_jointData[jointDataIndex].m_positionTrack, sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_position;
    }

    AZ::Quaternion NonUniformMotionData::SampleJointRotation(float sampleTime, size_t jointDataIndex) const
    {
        return !m_jointData[jointDataIndex].m_rotationTrack.m_times.empty() ? CalculateInterpolatedValue<AZ::Quaternion, MCore::Compressed16BitQuaternion>(m_jointData[jointDataIndex].m_rotationTrack, sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_rotation;
    }

#ifndef EMFX_SCALE_DISABLED
    AZ::Vector3 NonUniformMotionData::SampleJointScale(float sampleTime, size_t jointDataIndex) const
    {
        return !m_jointData[jointDataIndex].m_scaleTrack.m_times.empty() ? CalculateInterpolatedValue<AZ::Vector3, AZ::Vector3>(m_jointData[jointDataIndex].m_scaleTrack, sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_scale;
    }
#endif

    Transform NonUniformMotionData::SampleJointTransform(float sampleTime, size_t jointDataIndex) const
    {
        return Transform
        (
            !m_jointData[jointDataIndex].m_positionTrack.m_times.empty() ? CalculateInterpolatedValue<AZ::Vector3, AZ::Vector3>(m_jointData[jointDataIndex].m_positionTrack, sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_position,
            !m_jointData[jointDataIndex].m_rotationTrack.m_times.empty() ? CalculateInterpolatedValue<AZ::Quaternion, MCore::Compressed16BitQuaternion>(m_jointData[jointDataIndex].m_rotationTrack, sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_rotation
#ifndef EMFX_SCALE_DISABLED
            ,!m_jointData[jointDataIndex].m_scaleTrack.m_times.empty() ? CalculateInterpolatedValue<AZ::Vector3, AZ::Vector3>(m_jointData[jointDataIndex].m_scaleTrack, sampleTime) : m_staticJointData[jointDataIndex].m_staticTransform.m_scale
#endif
        );
    }

    void NonUniformMotionData::RemoveRedundantKeyframes(bool updateDurationAfterwards)
    {
        // Joints.
        AZStd::vector<JointData> tempJointData = m_jointData;
        for (size_t i = 0; i < m_jointData.size(); ++i)
        {
            JointData& jointData = tempJointData[i];
            ReduceTrackSamples<AZ::Vector3>(jointData.m_positionTrack, m_staticJointData[i].m_staticTransform.m_position, 0.0001f);
            ReduceTrackSamples<MCore::Compressed16BitQuaternion>(jointData.m_rotationTrack, m_staticJointData[i].m_staticTransform.m_rotation, 0.0001f);
            EMFX_SCALECODE
            (
                ReduceTrackSamples<AZ::Vector3>(jointData.m_scaleTrack, m_staticJointData[i].m_staticTransform.m_scale, 0.0001f);
                if (jointData.m_scaleTrack.m_times.empty())
                {
                    ClearJointScaleSamples(i);
                }
            )

            if (jointData.m_positionTrack.m_times.empty())
            {
                ClearJointPositionSamples(i);
            }

            if (jointData.m_rotationTrack.m_times.empty())
            {
                ClearJointRotationSamples(i);
            }
        }

        // Morphs.
        AZStd::vector<FloatData> tempMorphData = m_morphData;
        for (size_t i = 0; i < m_morphData.size(); ++i)
        {
            FloatData& morphData = tempMorphData[i];
            ReduceTrackSamples<float>(morphData.m_track, m_staticMorphData[i].m_staticValue, 0.0001f);
            if (morphData.m_track.m_times.empty())
            {
                ClearMorphSamples(i);
            }
        }

        // Floats.
        AZStd::vector<FloatData> tempFloatData = m_floatData;
        for (size_t i = 0; i < m_floatData.size(); ++i)
        {
            FloatData& floatData = tempFloatData[i];
            ReduceTrackSamples<float>(floatData.m_track, m_staticFloatData[i].m_staticValue, 0.0001f);
            if (floatData.m_track.m_times.empty())
            {
                ClearFloatSamples(i);
            }
        }

        if (updateDurationAfterwards)
        {
            UpdateDuration();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct File_NonUniformMotionData_Info
    {
        AZ::u32 m_numJoints = 0;
        AZ::u32 m_numMorphs = 0;
        AZ::u32 m_numFloats = 0;
        float m_sampleRate = 30.0f;
        float m_duration = 0.0f;

        // Followed by:
        // File_NonUniformMotionData_Joint[m_numJoints]
        // File_NonUniformMotionData_Float[m_numMorphs]
        // File_NonUniformMotionData_Float[m_numFloats]
    };

    struct File_NonUniformMotionData_Joint
    {
        FileFormat::File16BitQuaternion m_staticRot { 0, 0, 0, (1 << 15) - 1 };  // First frames rotation.
        FileFormat::File16BitQuaternion m_bindPoseRot { 0, 0, 0, (1 << 15) - 1 };// Bind pose rotation.
        FileFormat::FileVector3         m_staticPos { 0.0f, 0.0f, 0.0f };        // First frame position.
        FileFormat::FileVector3         m_staticScale { 1.0f, 1.0f, 1.0f };      // First frame scale.
        FileFormat::FileVector3         m_bindPosePos { 0.0f, 0.0f, 0.0f };      // Bind pose position.
        FileFormat::FileVector3         m_bindPoseScale { 1.0f, 1.0f, 1.0f };    // Bind pose scale.
        AZ::u32                         m_numPosKeys = 0;
        AZ::u32                         m_numRotKeys = 0;
        AZ::u32                         m_numScaleKeys = 0;

        // Followed by:
        // string : The name of the joint.
        // File_NonUniformMotionData_Vector3Key[ m_numPosKeys ]
        // File_NonUniformMotionData_16BitQuaternionKey[ m_numRotKeys ]
        // File_NonUniformMotionData_Vector3Key[ m_numScaleKeys ]
    };

    struct File_NonUniformMotionData_Float
    {
        float m_staticValue = 0.0f; // The static (first frame) value.
        AZ::u32 m_numKeys = 0;

        // Followed by:
        // String: The name of the channel.
        // File_NonUniformMotionData_FloatKey[ m_numKeys ]
    };

    struct File_NonUniformMotionData_FloatKey
    {
        float m_value = 0.0f;
        float m_time = 0.0f;
    };

    struct File_NonUniformMotionData_Vector3Key
    {
        FileFormat::FileVector3 m_value { 0.0f, 0.0f, 0.0f };
        float m_time = 0.0f;
    };

    struct File_NonUniformMotionData_16BitQuaternionKey
    {
        FileFormat::File16BitQuaternion m_value { 0, 0, 0, (1 << 15) - 1};
        float m_time = 0.0f;
    };

    bool SaveJoint(MCore::Stream* stream, const NonUniformMotionData* motionData, size_t jointDataIndex, const MotionData::SaveSettings& saveSettings)
    {
        const MCore::Endian::EEndianType targetEndianType = saveSettings.m_targetEndianType;

        // get the animation start pose and the bind pose transformation information
        AZ::PackedVector3f posePosition = AZ::PackedVector3f(motionData->GetJointStaticPosition(jointDataIndex));
        AZ::PackedVector3f bindPosePosition = AZ::PackedVector3f(motionData->GetJointBindPosePosition(jointDataIndex));
        MCore::Compressed16BitQuaternion poseRotation(motionData->GetJointStaticRotation(jointDataIndex));
        MCore::Compressed16BitQuaternion bindPoseRotation(motionData->GetJointBindPoseRotation(jointDataIndex));
        #ifndef EMFX_SCALE_DISABLED
            AZ::PackedVector3f poseScale = AZ::PackedVector3f(motionData->GetJointStaticScale(jointDataIndex));
            AZ::PackedVector3f bindPoseScale = AZ::PackedVector3f(motionData->GetJointBindPoseScale(jointDataIndex));
        #else
            AZ::PackedVector3f bindPoseScale(1.0f, 1.0f, 1.0f);
            AZ::PackedVector3f poseScale(1.0f, 1.0f, 1.0f);
        #endif

        File_NonUniformMotionData_Joint jointInfo;
        ExporterLib::CopyVector(jointInfo.m_staticPos, posePosition);
        ExporterLib::Copy16BitQuaternion(jointInfo.m_staticRot, poseRotation);
        ExporterLib::CopyVector(jointInfo.m_staticScale, poseScale);
        ExporterLib::CopyVector(jointInfo.m_bindPosePos, bindPosePosition);
        ExporterLib::Copy16BitQuaternion(jointInfo.m_bindPoseRot, bindPoseRotation);
        ExporterLib::CopyVector(jointInfo.m_bindPoseScale, bindPoseScale);

        jointInfo.m_numPosKeys = static_cast<AZ::u32>(motionData->GetNumJointPositionSamples(jointDataIndex));
        jointInfo.m_numRotKeys = static_cast<AZ::u32>(motionData->GetNumJointRotationSamples(jointDataIndex));
        EMFX_SCALECODE
        (
            jointInfo.m_numScaleKeys = static_cast<AZ::u32>(motionData->GetNumJointScaleSamples(jointDataIndex));
        )

        if (saveSettings.m_logDetails)
        {
            const AZ::Quaternion uncompressedPoseRot = MCore::Compressed16BitQuaternion(jointInfo.m_staticRot.m_x, jointInfo.m_staticRot.m_y, jointInfo.m_staticRot.m_z, jointInfo.m_staticRot.m_w).ToQuaternion().GetNormalized();
            const AZ::Quaternion uncompressedBindPoseRot = MCore::Compressed16BitQuaternion(jointInfo.m_bindPoseRot.m_x, jointInfo.m_bindPoseRot.m_y, jointInfo.m_bindPoseRot.m_z, jointInfo.m_bindPoseRot.m_w).ToQuaternion().GetNormalized();
            MCore::LogDetailedInfo("- Motion Joint: %s", motionData->GetJointName(jointDataIndex).c_str());
            MCore::LogDetailedInfo("   + Pose Translation: x=%f y=%f z=%f", jointInfo.m_staticPos.m_x, jointInfo.m_staticPos.m_y, jointInfo.m_staticPos.m_z);
            MCore::LogDetailedInfo("   + Pose Rotation:    x=%f y=%f z=%f w=%f", static_cast<float>(uncompressedPoseRot.GetX()), static_cast<float>(uncompressedPoseRot.GetY()), static_cast<float>(uncompressedPoseRot.GetZ()), static_cast<float>(uncompressedPoseRot.GetW()));
            MCore::LogDetailedInfo("   + Pose Scale:       x=%f y=%f z=%f", jointInfo.m_staticScale.m_x, jointInfo.m_staticScale.m_y, jointInfo.m_staticScale.m_z);
            MCore::LogDetailedInfo("   + Bind Pose Translation: x=%f y=%f z=%f", jointInfo.m_bindPosePos.m_x, jointInfo.m_bindPosePos.m_y, jointInfo.m_bindPosePos.m_z);
            MCore::LogDetailedInfo("   + Bind Pose Rotation:    x=%f y=%f z=%f w=%f", static_cast<float>(uncompressedBindPoseRot.GetX()), static_cast<float>(uncompressedBindPoseRot.GetY()), static_cast<float>(uncompressedBindPoseRot.GetZ()), static_cast<float>(uncompressedBindPoseRot.GetW()));
            MCore::LogDetailedInfo("   + Bind Pose Scale:       x=%f y=%f z=%f", jointInfo.m_bindPoseScale.m_x, jointInfo.m_bindPoseScale.m_y, jointInfo.m_bindPoseScale.m_z);
            MCore::LogDetailedInfo("   + Num Position Keys:     %d", jointInfo.m_numPosKeys);
            MCore::LogDetailedInfo("   + Num Rotation Keys:     %d", jointInfo.m_numRotKeys);
            MCore::LogDetailedInfo("   + Num Scale Keys:        %d", jointInfo.m_numScaleKeys);
        }

        // Convert endian.
        ExporterLib::ConvertFileVector3(&jointInfo.m_staticPos, targetEndianType);
        ExporterLib::ConvertFile16BitQuaternion(&jointInfo.m_staticRot, targetEndianType);
        ExporterLib::ConvertFileVector3(&jointInfo.m_staticScale, targetEndianType);
        ExporterLib::ConvertFileVector3(&jointInfo.m_bindPosePos, targetEndianType);
        ExporterLib::ConvertFile16BitQuaternion(&jointInfo.m_bindPoseRot, targetEndianType);
        ExporterLib::ConvertFileVector3(&jointInfo.m_bindPoseScale, targetEndianType);
        ExporterLib::ConvertUnsignedInt(&jointInfo.m_numPosKeys, targetEndianType);
        ExporterLib::ConvertUnsignedInt(&jointInfo.m_numRotKeys, targetEndianType);
        ExporterLib::ConvertUnsignedInt(&jointInfo.m_numScaleKeys, targetEndianType);

        stream->Write(&jointInfo, sizeof(File_NonUniformMotionData_Joint));

        ExporterLib::SaveString(motionData->GetJointName(jointDataIndex), stream, targetEndianType);

        // Position keys.
        const size_t numPosKeys = motionData->GetNumJointPositionSamples(jointDataIndex);
        for (size_t k = 0; k < numPosKeys; ++k)
        {
            const NonUniformMotionData::Vector3Key key = motionData->GetJointPositionSample(jointDataIndex, k);
            File_NonUniformMotionData_Vector3Key keyframe;
            keyframe.m_time = key.m_time;
            ExporterLib::CopyVector(keyframe.m_value, AZ::PackedVector3f(key.m_value));          
            ExporterLib::ConvertFloat(&keyframe.m_time, targetEndianType);
            ExporterLib::ConvertFileVector3(&keyframe.m_value, targetEndianType);
            if (stream->Write(&keyframe, sizeof(File_NonUniformMotionData_Vector3Key)) == 0)
            {
                return false;
            }
        }

        // Rotation keys.
        const size_t numRotKeys = motionData->GetNumJointRotationSamples(jointDataIndex);
        for (size_t k = 0; k < numRotKeys; ++k)
        {
            const NonUniformMotionData::QuaternionKey key = motionData->GetJointRotationSample(jointDataIndex, k);
            File_NonUniformMotionData_16BitQuaternionKey keyframe;
            keyframe.m_time = key.m_time;
            ExporterLib::Copy16BitQuaternion(keyframe.m_value, key.m_value);
            ExporterLib::ConvertFloat(&keyframe.m_time, targetEndianType);
            ExporterLib::ConvertFile16BitQuaternion(&keyframe.m_value, targetEndianType);
            if (stream->Write(&keyframe, sizeof(File_NonUniformMotionData_16BitQuaternionKey)) == 0)
            {
                return false;
            }
        }

        // Scale keys.
        EMFX_SCALECODE
        (
            const size_t numScaleKeys = motionData->GetNumJointScaleSamples(jointDataIndex);
            for (size_t k = 0; k < numScaleKeys; ++k)
            {
                const NonUniformMotionData::Vector3Key key = motionData->GetJointScaleSample(jointDataIndex, k);
                File_NonUniformMotionData_Vector3Key keyframe;
                keyframe.m_time = key.m_time;
                ExporterLib::CopyVector(keyframe.m_value, AZ::PackedVector3f(key.m_value));          
                ExporterLib::ConvertFloat(&keyframe.m_time, targetEndianType);
                ExporterLib::ConvertFileVector3(&keyframe.m_value, targetEndianType);
                if (stream->Write(&keyframe, sizeof(File_NonUniformMotionData_Vector3Key)) == 0)
                {
                    return false;
                }
            }
        )

        return true;
    }

    bool SaveMorph(MCore::Stream* stream, const NonUniformMotionData* motionData, size_t morphDataIndex, const MotionData::SaveSettings& saveSettings)
    {
        if (saveSettings.m_logDetails)
        {
            MCore::LogInfo("Saving motion morph with name '%s'", motionData->GetMorphName(morphDataIndex).c_str());
        }

        const MCore::Endian::EEndianType targetEndianType = saveSettings.m_targetEndianType;

        // Verify the morph target name.
        const AZStd::string& morphName = motionData->GetMorphName(morphDataIndex);
        if (morphName.empty())
        {
            AZ_Error("EMotionFX", false, "Cannot save morph target with empty name.");
            return false;
        }

        // Save the morph target header.
        File_NonUniformMotionData_Float morphInfo;
        morphInfo.m_staticValue = motionData->GetMorphStaticValue(morphDataIndex);
        morphInfo.m_numKeys     = static_cast<AZ::u32>(motionData->GetNumMorphSamples(morphDataIndex));
        if (saveSettings.m_logDetails)
        {
            MCore::LogDetailedInfo("    - Motion Morph: '%s'", morphName.c_str());
            MCore::LogDetailedInfo("       + NumKeys      = %d", morphInfo.m_numKeys);
            MCore::LogDetailedInfo("       + Static value = %f", morphInfo.m_staticValue);
        }
        ExporterLib::ConvertUnsignedInt(&morphInfo.m_numKeys, targetEndianType);
        ExporterLib::ConvertFloat(&morphInfo.m_staticValue, targetEndianType);
        if (stream->Write(&morphInfo, sizeof(File_NonUniformMotionData_Float)) == 0)
        {
            return false;
        }
        ExporterLib::SaveString(morphName, stream, targetEndianType);

        // Save the keyframes.
        const size_t numKeys = motionData->GetNumMorphSamples(morphDataIndex);
        for (size_t i = 0; i < numKeys; ++i)
        {
            NonUniformMotionData::FloatKey key = motionData->GetMorphSample(morphDataIndex, i);
            File_NonUniformMotionData_FloatKey keyChunk;
            keyChunk.m_time  = key.m_time;
            keyChunk.m_value = key.m_value;
            ExporterLib::ConvertFloat(&keyChunk.m_time, targetEndianType);
            ExporterLib::ConvertFloat(&keyChunk.m_value, targetEndianType);
            if (stream->Write(&keyChunk, sizeof(File_NonUniformMotionData_FloatKey)) == 0)
            {
                return false;
            }
        }

        return true;
    }

    bool SaveFloat(MCore::Stream* stream, const NonUniformMotionData* motionData, size_t floatDataIndex, const MotionData::SaveSettings& saveSettings)
    {
        if (saveSettings.m_logDetails)
        {
            MCore::LogInfo("Saving motion float with name '%s'", motionData->GetFloatName(floatDataIndex).c_str());
        }

        const MCore::Endian::EEndianType targetEndianType = saveSettings.m_targetEndianType;

        // Verify the name.
        const AZStd::string& floatName = motionData->GetMorphName(floatDataIndex);
        if (floatName.empty())
        {
            AZ_Error("EMotionFX", false, "Cannot save motion float channel with empty name.");
            return false;
        }

        // Save the float header.
        File_NonUniformMotionData_Float floatInfo;
        floatInfo.m_staticValue = motionData->GetFloatStaticValue(floatDataIndex);
        floatInfo.m_numKeys     = static_cast<AZ::u32>(motionData->GetNumFloatSamples(floatDataIndex));
        if (saveSettings.m_logDetails)
        {
            MCore::LogDetailedInfo("    - Motion Float: '%s'", floatName.c_str());
            MCore::LogDetailedInfo("       + NumKeys      = %d", floatInfo.m_numKeys);
            MCore::LogDetailedInfo("       + Static value = %f", floatInfo.m_staticValue);
        }
        ExporterLib::ConvertUnsignedInt(&floatInfo.m_numKeys, targetEndianType);
        ExporterLib::ConvertFloat(&floatInfo.m_staticValue, targetEndianType);
        if (stream->Write(&floatInfo, sizeof(File_NonUniformMotionData_Float)) == 0)
        {
            return false;
        }
        ExporterLib::SaveString(floatName, stream, targetEndianType);

        // Save the keyframes.
        const size_t numKeys = motionData->GetNumFloatSamples(floatDataIndex);
        for (size_t i = 0; i < numKeys; ++i)
        {
            NonUniformMotionData::FloatKey key = motionData->GetFloatSample(floatDataIndex, i);
            File_NonUniformMotionData_FloatKey keyChunk;
            keyChunk.m_time  = key.m_time;
            keyChunk.m_value = key.m_value;
            ExporterLib::ConvertFloat(&keyChunk.m_time, targetEndianType);
            ExporterLib::ConvertFloat(&keyChunk.m_value, targetEndianType);
            if (stream->Write(&keyChunk, sizeof(File_NonUniformMotionData_FloatKey)) == 0)
            {
                return false;
            }
        }

        return true;
    }

    bool NonUniformMotionData::Save(MCore::Stream* stream, const SaveSettings& saveSettings) const
    {
        const MCore::Endian::EEndianType targetEndianType = saveSettings.m_targetEndianType;

        // Write the info chunk.
        File_NonUniformMotionData_Info info;
        info.m_numJoints = static_cast<AZ::u32>(GetNumJoints());
        info.m_numMorphs = static_cast<AZ::u32>(GetNumMorphs());
        info.m_numFloats = static_cast<AZ::u32>(GetNumFloats());
        info.m_sampleRate = GetSampleRate();
        info.m_duration = GetDuration();
        ExporterLib::ConvertUnsignedInt(&info.m_numJoints, targetEndianType);
        ExporterLib::ConvertUnsignedInt(&info.m_numMorphs, targetEndianType);
        ExporterLib::ConvertUnsignedInt(&info.m_numFloats, targetEndianType);
        ExporterLib::ConvertFloat(&info.m_sampleRate, targetEndianType);
        ExporterLib::ConvertFloat(&info.m_duration, targetEndianType);
        if (stream->Write(&info, sizeof(File_NonUniformMotionData_Info)) == 0)
        {
            return false;
        }

        // Save joints.
        for (size_t i = 0; i < GetNumJoints(); i++)
        {
            if (!SaveJoint(stream, this, i, saveSettings))
            {
                return false;
            }
        }

        // Save morphs.
        for (size_t i = 0; i < GetNumMorphs(); i++)
        {
            if (!SaveMorph(stream, this, i, saveSettings))
            {
                return false;
            }
        }

        // Save floats.
        for (size_t i = 0; i < GetNumFloats(); i++)
        {
            if (!SaveFloat(stream, this, i, saveSettings))
            {
                return false;
            }
        }

        return true;
    }

    AZ::u32 NonUniformMotionData::GetStreamSaveVersion() const
    {
        return 1;
    }

    size_t NonUniformMotionData::CalcStreamSaveSizeInBytes([[maybe_unused]] const SaveSettings& saveSettings) const
    {
        size_t numBytes = sizeof(File_NonUniformMotionData_Info);

        // Add the joints.
        for (size_t i = 0; i < GetNumJoints(); ++i)
        {
            numBytes += sizeof(File_NonUniformMotionData_Joint);
            numBytes += ExporterLib::GetStringChunkSize(GetJointName(i));
            numBytes += GetNumJointPositionSamples(i) * sizeof(File_NonUniformMotionData_Vector3Key);
            numBytes += GetNumJointRotationSamples(i) * sizeof(File_NonUniformMotionData_16BitQuaternionKey);
            EMFX_SCALECODE
            (
                numBytes += GetNumJointScaleSamples(i) * sizeof(File_NonUniformMotionData_Vector3Key);
            )
        }

        // Add the morphs.
        for (size_t i = 0; i < GetNumMorphs(); ++i)
        {
            numBytes += sizeof(File_NonUniformMotionData_Float);
            numBytes += GetNumMorphSamples(i) * sizeof(File_NonUniformMotionData_FloatKey);
            numBytes += ExporterLib::GetStringChunkSize(GetMorphName(i));
        }

        // Add the floats.
        for (size_t i = 0; i < GetNumFloats(); ++i)
        {
            numBytes += sizeof(File_NonUniformMotionData_Float);
            numBytes += GetNumFloatSamples(i) * sizeof(File_NonUniformMotionData_FloatKey);
            numBytes += ExporterLib::GetStringChunkSize(GetFloatName(i));
        }

        return numBytes;
    }

    bool ReadVersion1(MCore::Stream* stream, NonUniformMotionData* motionData, const MotionData::ReadSettings& readSettings)
    {
        const MCore::Endian::EEndianType sourceEndianType = readSettings.m_sourceEndianType;

        // Read the info chunk.
        File_NonUniformMotionData_Info info;
        if (stream->Read(&info, sizeof(File_NonUniformMotionData_Info)) == 0)
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&info.m_numJoints, sourceEndianType);
        MCore::Endian::ConvertUnsignedInt32(&info.m_numMorphs, sourceEndianType);
        MCore::Endian::ConvertUnsignedInt32(&info.m_numFloats, sourceEndianType);
        MCore::Endian::ConvertFloat(&info.m_sampleRate, sourceEndianType);
        MCore::Endian::ConvertFloat(&info.m_duration, sourceEndianType);
        motionData->Resize(info.m_numJoints, info.m_numMorphs, info.m_numFloats);
        motionData->SetSampleRate(info.m_sampleRate);
        motionData->SetDuration(info.m_duration);
        if (readSettings.m_logDetails)
        {
            MCore::LogDetailedInfo("- NonUniformMotionData:");
            MCore::LogDetailedInfo("  + NumJoints  = %zu", info.m_numJoints);
            MCore::LogDetailedInfo("  + NumMorphs  = %zu", info.m_numMorphs);
            MCore::LogDetailedInfo("  + NumFloats  = %zu", info.m_numFloats);
            MCore::LogDetailedInfo("  + SampleRate = %f", info.m_sampleRate);
            MCore::LogDetailedInfo("  + Duration = %f", info.m_duration);
        }

        // Read the joints.
        AZStd::string name;
        for (size_t i = 0; i < motionData->GetNumJoints(); ++i)
        {
            File_NonUniformMotionData_Joint jointInfo;
            if (stream->Read(&jointInfo, sizeof(File_NonUniformMotionData_Joint)) == 0)
            {
                return false;
            }

            AZ::Vector3 staticPos(jointInfo.m_staticPos.m_x, jointInfo.m_staticPos.m_y, jointInfo.m_staticPos.m_z);
            AZ::Vector3 staticScale(jointInfo.m_staticScale.m_x, jointInfo.m_staticScale.m_y, jointInfo.m_staticScale.m_z);
            MCore::Compressed16BitQuaternion staticRot(jointInfo.m_staticRot.m_x, jointInfo.m_staticRot.m_y, jointInfo.m_staticRot.m_z, jointInfo.m_staticRot.m_w);
            AZ::Vector3 bindPosePos(jointInfo.m_bindPosePos.m_x, jointInfo.m_bindPosePos.m_y, jointInfo.m_bindPosePos.m_z);
            AZ::Vector3 bindPoseScale(jointInfo.m_bindPoseScale.m_x, jointInfo.m_bindPoseScale.m_y, jointInfo.m_bindPoseScale.m_z);
            MCore::Compressed16BitQuaternion bindPoseRot(jointInfo.m_bindPoseRot.m_x, jointInfo.m_bindPoseRot.m_y, jointInfo.m_bindPoseRot.m_z, jointInfo.m_bindPoseRot.m_w);
            MCore::Endian::ConvertVector3(&staticPos, sourceEndianType);
            MCore::Endian::Convert16BitQuaternion(&staticRot, sourceEndianType);
            MCore::Endian::ConvertVector3(&staticScale, sourceEndianType);
            MCore::Endian::ConvertVector3(&bindPosePos, sourceEndianType);
            MCore::Endian::Convert16BitQuaternion(&bindPoseRot, sourceEndianType);
            MCore::Endian::ConvertVector3(&bindPoseScale, sourceEndianType);
            MCore::Endian::ConvertUnsignedInt32(&jointInfo.m_numPosKeys, sourceEndianType);
            MCore::Endian::ConvertUnsignedInt32(&jointInfo.m_numRotKeys, sourceEndianType);
            MCore::Endian::ConvertUnsignedInt32(&jointInfo.m_numScaleKeys, sourceEndianType);

            motionData->SetJointStaticPosition(i, staticPos);
            motionData->SetJointStaticRotation(i, staticRot.ToQuaternion().GetNormalized());
            motionData->SetJointBindPosePosition(i, bindPosePos);
            motionData->SetJointBindPoseRotation(i, bindPoseRot.ToQuaternion().GetNormalized());
            EMFX_SCALECODE
            (
                motionData->SetJointStaticScale(i, staticScale);
                motionData->SetJointBindPoseScale(i, bindPoseScale);
            )

            // Read the name.
            name = MotionData::ReadStringFromStream(stream, sourceEndianType);
            motionData->SetJointName(i, name);

            if (readSettings.m_logDetails)
            {
                MCore::LogDetailedInfo("  + [%zu] Joint = '%s'", i, name.c_str());
                MCore::LogDetailedInfo("    - Num Pos Keys   = %d", jointInfo.m_numPosKeys);
                MCore::LogDetailedInfo("    - Num Rot Keys   = %d", jointInfo.m_numRotKeys);
                MCore::LogDetailedInfo("    - Num Scale Keys = %d", jointInfo.m_numScaleKeys);
            }

            // Read the position keys.
            if (jointInfo.m_numPosKeys > 0)
            {
                motionData->AllocateJointPositionSamples(i, jointInfo.m_numPosKeys);
                for (size_t s = 0; s < jointInfo.m_numPosKeys; ++s)
                {
                    File_NonUniformMotionData_Vector3Key keyInfo;
                    if (stream->Read(&keyInfo, sizeof(File_NonUniformMotionData_Vector3Key)) == 0)
                    {
                        return false;
                    }
                    MCore::Endian::ConvertFloat(&keyInfo.m_time, sourceEndianType);
                    MCore::Endian::ConvertFloat(&keyInfo.m_value.m_x, sourceEndianType, /*numFloats=*/3);
                    motionData->SetJointPositionSample(i, s, {keyInfo.m_time, AZ::Vector3(keyInfo.m_value.m_x, keyInfo.m_value.m_y, keyInfo.m_value.m_z)});
                }
            }

            // Read the rotation keys.
            if (jointInfo.m_numRotKeys > 0)
            {
                motionData->AllocateJointRotationSamples(i, jointInfo.m_numRotKeys);
                for (size_t s = 0; s < jointInfo.m_numRotKeys; ++s)
                {
                    File_NonUniformMotionData_16BitQuaternionKey keyInfo;
                    if (stream->Read(&keyInfo, sizeof(File_NonUniformMotionData_16BitQuaternionKey))== 0)
                    {
                        return false;
                    }
                    MCore::Endian::ConvertFloat(&keyInfo.m_time, sourceEndianType);
                    MCore::Compressed16BitQuaternion compressedQuat(keyInfo.m_value.m_x, keyInfo.m_value.m_y, keyInfo.m_value.m_z, keyInfo.m_value.m_w);
                    MCore::Endian::Convert16BitQuaternion(&compressedQuat, sourceEndianType);
                    motionData->SetJointRotationSample(i, s, {keyInfo.m_time, compressedQuat.ToQuaternion().GetNormalized()});
                }
            }

            // Read the scale keys.
            EMFX_SCALECODE
            (
                if (jointInfo.m_numScaleKeys > 0)
                {
                    motionData->AllocateJointScaleSamples(i, jointInfo.m_numScaleKeys);
                    for (size_t s = 0; s < jointInfo.m_numScaleKeys; ++s)
                    {
                        File_NonUniformMotionData_Vector3Key keyInfo;
                        if (stream->Read(&keyInfo, sizeof(File_NonUniformMotionData_Vector3Key)) == 0)
                        {
                            return false;
                        }
                        MCore::Endian::ConvertFloat(&keyInfo.m_time, sourceEndianType);
                        MCore::Endian::ConvertFloat(&keyInfo.m_value.m_x, sourceEndianType, /*numFloats=*/3);
                        motionData->SetJointScaleSample(i, s, {keyInfo.m_time, AZ::Vector3(keyInfo.m_value.m_x, keyInfo.m_value.m_y, keyInfo.m_value.m_z)});
                    }
                }
            )
        }

        // Read the morphs.
        for (size_t i = 0; i < motionData->GetNumMorphs(); ++i)
        {
            File_NonUniformMotionData_Float floatInfo;
            if (stream->Read(&floatInfo, sizeof(File_NonUniformMotionData_Float)) == 0)
            {
                return false;
            }

            MCore::Endian::ConvertUnsignedInt32(&floatInfo.m_numKeys, sourceEndianType);
            MCore::Endian::ConvertFloat(&floatInfo.m_staticValue, sourceEndianType);
            name = MotionData::ReadStringFromStream(stream, sourceEndianType);

            if (readSettings.m_logDetails)
            {
                MCore::LogDetailedInfo("  + Morph: '%s'", name.c_str());
                MCore::LogDetailedInfo("       + NumKeys      = %d", floatInfo.m_numKeys);
                MCore::LogDetailedInfo("       + Static value = %f", floatInfo.m_staticValue);
            }

            motionData->SetMorphName(i, name);
            motionData->SetMorphStaticValue(i, floatInfo.m_staticValue);

            // Read the keys.
            if (floatInfo.m_numKeys > 0)
            {
                motionData->AllocateMorphSamples(i, floatInfo.m_numKeys);
                for (size_t s = 0; s < floatInfo.m_numKeys; ++s)
                {
                    File_NonUniformMotionData_FloatKey keyChunk;
                    if (stream->Read(&keyChunk, sizeof(File_NonUniformMotionData_FloatKey)) == 0)
                    {
                        return false;
                    }
                    MCore::Endian::ConvertFloat(&keyChunk.m_time, sourceEndianType);
                    MCore::Endian::ConvertFloat(&keyChunk.m_value, sourceEndianType);
                    motionData->SetMorphSample(i, s, {keyChunk.m_time, keyChunk.m_value});
                }
            }
        }

        // Read the floats.
        for (size_t i = 0; i < motionData->GetNumFloats(); ++i)
        {
            File_NonUniformMotionData_Float floatInfo;
            if (stream->Read(&floatInfo, sizeof(File_NonUniformMotionData_Float)) == 0)
            {
                return false;
            }

            MCore::Endian::ConvertUnsignedInt32(&floatInfo.m_numKeys, sourceEndianType);
            MCore::Endian::ConvertFloat(&floatInfo.m_staticValue, sourceEndianType);
            name = MotionData::ReadStringFromStream(stream, sourceEndianType);

            if (readSettings.m_logDetails)
            {
                MCore::LogDetailedInfo("  + Float: '%s'", name.c_str());
                MCore::LogDetailedInfo("       + NumKeys      = %d", floatInfo.m_numKeys);
                MCore::LogDetailedInfo("       + Static value = %f", floatInfo.m_staticValue);
            }

            motionData->SetFloatName(i, name);
            motionData->SetFloatStaticValue(i, floatInfo.m_staticValue);

            // Read the keys.
            if (floatInfo.m_numKeys > 0)
            {
                motionData->AllocateFloatSamples(i, floatInfo.m_numKeys);
                for (size_t s = 0; s < floatInfo.m_numKeys; ++s)
                {
                    File_NonUniformMotionData_FloatKey keyChunk;
                    if (stream->Read(&keyChunk, sizeof(File_NonUniformMotionData_FloatKey)) == 0)
                    {
                        return false;
                    }
                    MCore::Endian::ConvertFloat(&keyChunk.m_time, sourceEndianType);
                    MCore::Endian::ConvertFloat(&keyChunk.m_value, sourceEndianType);
                    motionData->SetFloatSample(i, s, {keyChunk.m_time, keyChunk.m_value});
                }
            }
        }

        return true;
    }

    bool NonUniformMotionData::Read(MCore::Stream* stream, const ReadSettings& readSettings)
    {
        switch (readSettings.m_version)
        {
            case 1:
            {
                return ReadVersion1(stream, this, readSettings);
            }
            break;

            default:
            {
                AZ_Error("EMotionFX", false, "Unsupported NonUniformMotionData version (version=%d), cannot load motion data.", readSettings.m_version);
            }
        }

        return false;
    }
    
} // namespace EMotionFX
