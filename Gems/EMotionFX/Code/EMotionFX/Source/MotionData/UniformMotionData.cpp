/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Outcome/Outcome.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphSetupInstance.h>
#include <EMotionFX/Source/MotionData/UniformMotionData.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/TransformData.h>

#include <EMotionFX/Source/Importer/SharedFileFormatStructs.h>
#include <EMotionFX/Source/Importer/MotionFileFormat.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <MCore/Source/LogManager.h>

namespace EMotionFX
{
    UniformMotionData::~UniformMotionData()
    {
        ClearAllData();
    }

    MotionData* UniformMotionData::CreateNew() const
    {
        return aznew UniformMotionData();
    }

    const char* UniformMotionData::GetSceneSettingsName() const
    {
        return "Evenly Spaced Keyframes (faster, mostly larger)";
    }

    void UniformMotionData::InitFromNonUniformData(const NonUniformMotionData* motionData, bool keepSameSampleRate, float newSampleRate, [[maybe_unused]] bool updateDuration)
    {
        AZ_Assert(newSampleRate > 0.0f, "Expected the sample rate to be larger than zero.");
        SetSampleRate(keepSameSampleRate ? motionData->GetSampleRate() : newSampleRate);

        // Calculate the sample spacing and number of samples required.
        float sampleSpacing = 0.0f;
        size_t numSamples = 0;
        MotionData::CalculateSampleInformation(motionData->GetDuration(), m_sampleRate, numSamples, sampleSpacing);

        // Init the sample spacing and number of samples.
        UniformMotionData::InitSettings initSettings;
        initSettings.m_numJoints = motionData->GetNumJoints();
        initSettings.m_numMorphs = motionData->GetNumMorphs();
        initSettings.m_numFloats = motionData->GetNumFloats();
        initSettings.m_sampleRate = m_sampleRate;
        initSettings.m_numSamples = numSamples;
        Init(initSettings);
        CopyBaseMotionData(motionData);

        AZ_Warning("EMotionFX", AZ::IsClose(m_sampleSpacing, sampleSpacing, AZ::Constants::FloatEpsilon),
            "Corrected sample spacing should match the set inverse sample rate. Floating point accuracy error.");

        // Joints.
        for (size_t i = 0; i < initSettings.m_numJoints; ++i)
        {
            if (!motionData->IsJointAnimated(i))
            {
                continue;
            }

            // Allocate samples where needed.
            const bool posAnimated = motionData->IsJointPositionAnimated(i);
            const bool rotAnimated = motionData->IsJointRotationAnimated(i);
            if (posAnimated) { AllocateJointPositionSamples(i); }
            if (rotAnimated) { AllocateJointRotationSamples(i); }
            EMFX_SCALECODE
            (
                const bool scaleAnimated = motionData->IsJointScaleAnimated(i);
                if (scaleAnimated) { AllocateJointScaleSamples(i); }
            )

            for (size_t s = 0; s < m_numSamples; ++s)
            {
                const float keyTime = s * sampleSpacing;
                const Transform transform = motionData->SampleJointTransform(keyTime, i);
                if (posAnimated) m_jointData[i].m_positions[s] = transform.m_position;
                if (rotAnimated) m_jointData[i].m_rotations[s] = transform.m_rotation.GetNormalized();
                EMFX_SCALECODE
                (
                    if (scaleAnimated) m_jointData[i].m_scales[s] = transform.m_scale;
                )
            }
        }

        // Morphs.
        for (size_t i = 0; i < initSettings.m_numMorphs; ++i)
        {
            if (!motionData->IsMorphAnimated(i))
            {
                continue;
            }

            AllocateMorphSamples(i);
            for (size_t s = 0; s < m_numSamples; ++s)
            {
                const float keyTime = s * sampleSpacing;
                m_morphData[i].m_values[s] = motionData->SampleMorph(keyTime, i);
            }
        }

        // Floats.
        for (size_t i = 0; i < initSettings.m_numFloats; ++i)
        {
            if (!motionData->IsFloatAnimated(i))
            {
                continue;
            }

            AllocateFloatSamples(i);
            for (size_t s = 0; s < m_numSamples; ++s)
            {
                const float keyTime = s * sampleSpacing;
                m_floatData[i].m_values[s] = motionData->SampleFloat(keyTime, i);
            }
        }
    }

    Transform UniformMotionData::SampleJointTransform(const MotionDataSampleSettings& settings, size_t jointSkeletonIndex) const
    {
        const Actor* actor = settings.m_actorInstance->GetActor();
        const MotionLinkData* motionLinkData = FindMotionLinkData(actor);

        const size_t transformDataIndex = motionLinkData->GetJointDataLinks()[jointSkeletonIndex];
        if (m_additive && transformDataIndex == InvalidIndex)
        {
            return Transform::CreateIdentity();
        }

        // Calculate the sample indices to interpolate between, and the interpolation fraction.
        float t;
        size_t indexA;
        size_t indexB;
        CalculateInterpolationIndicesUniform(settings.m_sampleTime, m_sampleSpacing, m_duration, m_numSamples, indexA, indexB, t);

        const bool inPlace = (settings.m_inPlace && jointSkeletonIndex == actor->GetMotionExtractionNodeIndex());

        // Sample the interpolated data.
        Transform result;
        if (transformDataIndex != InvalidIndex && !inPlace)
        {
            const StaticJointData& staticJointData = m_staticJointData[transformDataIndex];
            const JointData& jointData = m_jointData[transformDataIndex];
            result.m_position = !jointData.m_positions.empty() ? jointData.m_positions[indexA].Lerp(jointData.m_positions[indexB], t) : staticJointData.m_staticTransform.m_position;
            result.m_rotation = !jointData.m_rotations.empty() ? jointData.m_rotations[indexA].ToQuaternion().NLerp(jointData.m_rotations[indexB].ToQuaternion(), t) : staticJointData.m_staticTransform.m_rotation;
#ifndef EMFX_SCALE_DISABLED
            result.m_scale = !jointData.m_scales.empty() ? jointData.m_scales[indexA].Lerp(jointData.m_scales[indexB], t) : staticJointData.m_staticTransform.m_scale;
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

    void UniformMotionData::SamplePose(const MotionDataSampleSettings& settings, Pose* outputPose) const
    {
        AZ_Assert(settings.m_actorInstance, "Expecting a valid actor instance.");
        const Actor* actor = settings.m_actorInstance->GetActor();
        const MotionLinkData* motionLinkData = FindMotionLinkData(actor);

        // Calculate the sample indices to interpolate between, and the interpolation fraction.
        float t;
        size_t indexA;
        size_t indexB;
        CalculateInterpolationIndicesUniform(settings.m_sampleTime, m_sampleSpacing, m_duration, m_numSamples, indexA, indexB, t);

        const AZStd::vector<size_t>& jointLinks = motionLinkData->GetJointDataLinks();
        const ActorInstance* actorInstance = settings.m_actorInstance;
        const Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();
        const size_t numNodes = actorInstance->GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const size_t skeletonJointIndex = actorInstance->GetEnabledNode(i);
            const bool inPlace = (settings.m_inPlace && skeletonJointIndex == actor->GetMotionExtractionNodeIndex());

            // Sample the interpolated data.
            Transform result;
            const size_t jointDataIndex = jointLinks[skeletonJointIndex];
            if (jointDataIndex != InvalidIndex && !inPlace)
            {
                const StaticJointData& staticJointData = m_staticJointData[jointDataIndex];
                const JointData& jointData = m_jointData[jointDataIndex];
                result.m_position = !jointData.m_positions.empty() ? jointData.m_positions[indexA].Lerp(jointData.m_positions[indexB], t) : staticJointData.m_staticTransform.m_position;
                result.m_rotation = !jointData.m_rotations.empty() ? jointData.m_rotations[indexA].ToQuaternion().NLerp(jointData.m_rotations[indexB].ToQuaternion(), t) : staticJointData.m_staticTransform.m_rotation;

#ifndef EMFX_SCALE_DISABLED
                result.m_scale = !jointData.m_scales.empty() ? jointData.m_scales[indexA].Lerp(jointData.m_scales[indexB], t) : staticJointData.m_staticTransform.m_scale;
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
                        result = settings.m_inputPose->GetLocalSpaceTransform(skeletonJointIndex);
                    }
                    else
                    {
                        result = bindPose->GetLocalSpaceTransform(skeletonJointIndex);
                    }
                }
            }

            // Apply retargeting.
            if (settings.m_retarget)
            {
                BasicRetarget(settings.m_actorInstance, motionLinkData, skeletonJointIndex, result);
            }

            outputPose->SetLocalSpaceTransformDirect(skeletonJointIndex, result);
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
                if (!data.m_values.empty())
                {
                    const float interpolated = AZ::Lerp(data.m_values[indexA], data.m_values[indexB], t);
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

    float UniformMotionData::SampleMorph(float sampleTime, size_t morphDataIndex) const
    {
        // Calculate the sample indices to interpolate between, and the interpolation fraction.
        float t;
        size_t indexA;
        size_t indexB;
        CalculateInterpolationIndicesUniform(sampleTime, m_sampleSpacing, m_duration, m_numSamples, indexA, indexB, t);

        const AZStd::vector<float>& values = m_morphData[morphDataIndex].m_values;
        return (!values.empty()) ? AZ::Lerp(values[indexA], values[indexB], t) : m_staticMorphData[morphDataIndex].m_staticValue;
    }

    float UniformMotionData::SampleFloat(float sampleTime, size_t floatDataIndex) const
    {
        // Calculate the sample indices to interpolate between, and the interpolation fraction.
        float t;
        size_t indexA;
        size_t indexB;
        CalculateInterpolationIndicesUniform(sampleTime, m_sampleSpacing, m_duration, m_numSamples, indexA, indexB, t);

        // Interpolate and store the result.
        const AZStd::vector<float>& values = m_floatData[floatDataIndex].m_values;
        return (!values.empty()) ? AZ::Lerp(values[indexA], values[indexB], t) : m_staticMorphData[floatDataIndex].m_staticValue;
    }

    void UniformMotionData::Init(const InitSettings& settings)
    {
        if (settings.m_numSamples > 0)
        {
            AZ_Error("EMotionFX", settings.m_sampleRate > 0.0f, "Sample rate should be larger than zero.");
        }
        Clear();
        Resize(settings.m_numJoints, settings.m_numMorphs, settings.m_numFloats);
        m_numSamples = settings.m_numSamples;
        SetSampleRate(settings.m_sampleRate);
        UpdateDuration();
    }

    void UniformMotionData::ResizeSampleData(size_t numJoints, size_t numMorphs, size_t numFloats)
    {
        m_jointData.resize(numJoints);
        m_morphData.resize(numMorphs);
        m_floatData.resize(numFloats);
    }

    void UniformMotionData::AddJointSampleData([[maybe_unused]] size_t jointDataIndex)
    {
        AZ_Assert(jointDataIndex == m_jointData.size(), "Expected the size of the jointData vector to be a different size. Is it in sync with the m_staticJointData vector?");
        m_jointData.emplace_back();
    }

    void UniformMotionData::AddMorphSampleData([[maybe_unused]] size_t morphDataIndex)
    {
        AZ_Assert(morphDataIndex == m_morphData.size(), "Expected the size of the morphData vector to be a different size. Is it in sync with the m_staticMorphData vector?");
        m_morphData.emplace_back();
    }

    void UniformMotionData::AddFloatSampleData([[maybe_unused]] size_t floatDataIndex)
    {
        AZ_Assert(floatDataIndex == m_floatData.size(), "Expected the size of the floatData vector to be a different size. Is it in sync with the m_staticFloatData vector?");
        m_floatData.emplace_back();
    }

    void UniformMotionData::UpdateDuration()
    {
        m_duration = (m_numSamples > 0) ? (m_numSamples - 1) * m_sampleSpacing : 0.0f;
    }

    void UniformMotionData::AllocateJointPositionSamples(size_t jointDataIndex)
    {
        m_jointData[jointDataIndex].m_positions.resize(GetNumSamples());
    }

    void UniformMotionData::AllocateJointRotationSamples(size_t jointDataIndex)
    {
        m_jointData[jointDataIndex].m_rotations.resize(GetNumSamples());
    }

#ifndef EMFX_SCALE_DISABLED
    void UniformMotionData::AllocateJointScaleSamples(size_t jointDataIndex)
    {
        m_jointData[jointDataIndex].m_scales.resize(GetNumSamples());
    }
#endif

    void UniformMotionData::AllocateMorphSamples(size_t morphDataIndex)
    {
        m_morphData[morphDataIndex].m_values.resize(GetNumSamples());
    }

    void UniformMotionData::AllocateFloatSamples(size_t floatDataIndex)
    {
        m_floatData[floatDataIndex].m_values.resize(GetNumSamples());
    }

    bool UniformMotionData::IsJointPositionAnimated(size_t jointDataIndex) const
    {
        return !m_jointData[jointDataIndex].m_positions.empty();
    }

    bool UniformMotionData::IsJointRotationAnimated(size_t jointDataIndex) const
    {
        return !m_jointData[jointDataIndex].m_rotations.empty();
    }

#ifndef EMFX_SCALE_DISABLED
    bool UniformMotionData::IsJointScaleAnimated(size_t jointDataIndex) const
    {
        return !m_jointData[jointDataIndex].m_scales.empty();
    }
#endif

    bool UniformMotionData::IsJointAnimated(size_t jointDataIndex) const
    {
        const JointData& jointData = m_jointData[jointDataIndex];

#ifndef EMFX_SCALE_DISABLED
        return (!jointData.m_positions.empty() || !jointData.m_rotations.empty() || !jointData.m_scales.empty());
#else
        return (!jointData.m_positions.empty() || !jointData.m_rotations.empty());
#endif
    }

    MotionData::Vector3Key UniformMotionData::GetJointPositionSample(size_t jointDataIndex, size_t sampleIndex) const
    {
        return { static_cast<float>(m_sampleSpacing * sampleIndex), m_jointData[jointDataIndex].m_positions[sampleIndex] };
    }

    MotionData::QuaternionKey UniformMotionData::GetJointRotationSample(size_t jointDataIndex, size_t sampleIndex) const
    {
        return { static_cast<float>(m_sampleSpacing * sampleIndex), m_jointData[jointDataIndex].m_rotations[sampleIndex] };
    }

#ifndef EMFX_SCALE_DISABLED
    MotionData::Vector3Key UniformMotionData::GetJointScaleSample(size_t jointDataIndex, size_t sampleIndex) const
    {
        return { static_cast<float>(m_sampleSpacing * sampleIndex), m_jointData[jointDataIndex].m_scales[sampleIndex] };
    }
#endif

    MotionData::FloatKey UniformMotionData::GetMorphSample(size_t morphDataIndex, size_t sampleIndex) const
    {
        return { static_cast<float>(m_sampleSpacing * sampleIndex), m_morphData[morphDataIndex].m_values[sampleIndex] };
    }

    MotionData::FloatKey UniformMotionData::GetFloatSample(size_t floatDataIndex, size_t sampleIndex) const
    {
        return { static_cast<float>(m_sampleSpacing * sampleIndex), m_floatData[floatDataIndex].m_values[sampleIndex] };
    }

    bool UniformMotionData::IsMorphAnimated(size_t morphDataIndex) const
    {
        return !m_morphData[morphDataIndex].m_values.empty();
    }

    bool UniformMotionData::IsFloatAnimated(size_t floatDataIndex) const
    {
        return !m_floatData[floatDataIndex].m_values.empty();
    }

    size_t UniformMotionData::GetNumSamples() const
    {
        return m_numSamples;
    }

    float UniformMotionData::GetSampleSpacing() const
    {
        return m_sampleSpacing;
    }

    void UniformMotionData::UpdateSampleSpacing()
    {
        if (m_sampleRate > AZ::Constants::FloatEpsilon)
        {
            m_sampleSpacing = 1.0f / m_sampleRate;
        }
        else
        {
            m_sampleSpacing = 0.0f;
        }
    }

    void UniformMotionData::SetSampleRate(float sampleRate)
    {
        MotionData::SetSampleRate(sampleRate);
        UpdateSampleSpacing();
    }

    void UniformMotionData::ClearAllJointTransformSamples()
    {
        for (JointData& data : m_jointData)
        {
            data.m_positions.clear();
            data.m_rotations.clear();
#ifndef EMFX_SCALE_DISABLED
            data.m_scales.clear();
#endif
        }
    }

    void UniformMotionData::ClearAllMorphSamples()
    {
        for (FloatData& data : m_morphData)
        {
            data.m_values.clear();
        }
    }

    void UniformMotionData::ClearAllFloatSamples()
    {
        for (FloatData& data : m_floatData)
        {
            data.m_values.clear();
        }
    }

    void UniformMotionData::ClearJointPositionSamples(size_t jointDataIndex)
    {
        m_jointData[jointDataIndex].m_positions.clear();
    }

    void UniformMotionData::ClearJointRotationSamples(size_t jointDataIndex)
    {
        m_jointData[jointDataIndex].m_rotations.clear();
    }

#ifndef EMFX_SCALE_DISABLED
    void UniformMotionData::ClearJointScaleSamples(size_t jointDataIndex)
    {
        m_jointData[jointDataIndex].m_scales.clear();
    }
#endif

    void UniformMotionData::ClearJointTransformSamples(size_t jointDataIndex)
    {
        ClearJointPositionSamples(jointDataIndex);
        ClearJointRotationSamples(jointDataIndex);
#ifndef EMFX_SCALE_DISABLED
        ClearJointScaleSamples(jointDataIndex);
#endif
    }

    void UniformMotionData::ClearMorphSamples(size_t morphDataIndex)
    {
        m_morphData[morphDataIndex].m_values.clear();
    }

    void UniformMotionData::ClearFloatSamples(size_t floatDataIndex)
    {
        m_floatData[floatDataIndex].m_values.clear();
    }

    void UniformMotionData::SetJointPositionSample(size_t jointDataIndex, size_t sampleIndex, const AZ::Vector3& position)
    {
        m_jointData[jointDataIndex].m_positions[sampleIndex] = position;
    }

    void UniformMotionData::SetJointRotationSample(size_t jointDataIndex, size_t sampleIndex, const AZ::Quaternion& rotation)
    {
        m_jointData[jointDataIndex].m_rotations[sampleIndex] = rotation;
    }

#ifndef EMFX_SCALE_DISABLED
    void UniformMotionData::SetJointScaleSample(size_t jointDataIndex, size_t sampleIndex, const AZ::Vector3& scale)
    {
        m_jointData[jointDataIndex].m_scales[sampleIndex] = scale;
    }
#endif

    void UniformMotionData::SetMorphSample(size_t morphDataIndex, size_t sampleIndex, float value)
    {
        m_morphData[morphDataIndex].m_values[sampleIndex] = value;
    }

    void UniformMotionData::SetFloatSample(size_t floatDataIndex, size_t sampleIndex, float value)
    {
        m_floatData[floatDataIndex].m_values[sampleIndex] = value;
    }

    void UniformMotionData::SetJointPositionSamples(size_t jointDataIndex, const AZStd::vector<AZ::Vector3>& positions)
    {
        AZ_Error("EMotionFX", positions.size() == m_numSamples, "Expecting positions vector to be of size %d instead of %d.", m_numSamples, positions.size());
        if (positions.size() == m_numSamples)
        {
            m_jointData[jointDataIndex].m_positions = positions;
        }
    }

    void UniformMotionData::SetJointRotationSamples(size_t jointDataIndex, const AZStd::vector<AZ::Quaternion>& rotations)
    {
        AZ_Error("EMotionFX", rotations.size() == m_numSamples, "Expecting rotations vector to be of size %d instead of %d.", m_numSamples, rotations.size());
        if (rotations.size() == m_numSamples)
        {
            for (size_t i = 0; i < m_numSamples; ++i)
            {
                m_jointData[jointDataIndex].m_rotations[i] = MCore::Compressed16BitQuaternion(rotations[i]);
            }
        }
    }

#ifndef EMFX_SCALE_DISABLED
    void UniformMotionData::SetJointScaleSamples(size_t jointDataIndex, const AZStd::vector<AZ::Vector3>& scales)
    {
        AZ_Error("EMotionFX", scales.size() == m_numSamples, "Expecting scales vector to be of size %d instead of %d.", m_numSamples, scales.size());
        if (scales.size() == m_numSamples)
        {
            m_jointData[jointDataIndex].m_scales = scales;
        }
    }
#endif

    void UniformMotionData::ClearAllData()
    {
        m_jointData.clear();
        m_jointData.shrink_to_fit();
        m_morphData.clear();
        m_morphData.shrink_to_fit();
        m_floatData.clear();
        m_floatData.shrink_to_fit();

        m_numSamples = 0;
    }

    void UniformMotionData::RemoveJointSampleData(size_t jointDataIndex)
    {
        m_jointData.erase(m_jointData.begin() + jointDataIndex);
    }

    void UniformMotionData::RemoveMorphSampleData(size_t morphDataIndex)
    {
        m_morphData.erase(m_morphData.begin() + morphDataIndex);
    }

    void UniformMotionData::RemoveFloatSampleData(size_t floatDataIndex)
    {
        m_floatData.erase(m_floatData.begin() + floatDataIndex);
    }

    void UniformMotionData::ScaleData(float scaleFactor)
    {
        for (JointData& jointData : m_jointData)
        {
            for (AZ::Vector3& pos : jointData.m_positions)
            {
                pos *= scaleFactor;
            }
        }
    }

    AZ::Vector3 UniformMotionData::SampleJointPosition(float sampleTime, size_t jointDataIndex) const
    {
        float t;
        size_t indexA;
        size_t indexB;
        CalculateInterpolationIndicesUniform(sampleTime, m_sampleSpacing, m_duration, m_numSamples, indexA, indexB, t);

        const AZStd::vector<AZ::Vector3>& values = m_jointData[jointDataIndex].m_positions;
        return !values.empty() ? values[indexA].Lerp(values[indexB], t) : m_staticJointData[jointDataIndex].m_staticTransform.m_position;
    }

    AZ::Quaternion UniformMotionData::SampleJointRotation(float sampleTime, size_t jointDataIndex) const
    {
        float t;
        size_t indexA;
        size_t indexB;
        CalculateInterpolationIndicesUniform(sampleTime, m_sampleSpacing, m_duration, m_numSamples, indexA, indexB, t);

        const AZStd::vector<MCore::Compressed16BitQuaternion>& values = m_jointData[jointDataIndex].m_rotations;
        return !values.empty() ? values[indexA].ToQuaternion().NLerp(values[indexB].ToQuaternion(), t) : m_staticJointData[jointDataIndex].m_staticTransform.m_rotation;
    }

#ifndef EMFX_SCALE_DISABLED
    AZ::Vector3 UniformMotionData::SampleJointScale(float sampleTime, size_t jointDataIndex) const
    {
        float t;
        size_t indexA;
        size_t indexB;
        CalculateInterpolationIndicesUniform(sampleTime, m_sampleSpacing, m_duration, m_numSamples, indexA, indexB, t);

        const AZStd::vector<AZ::Vector3>& values = m_jointData[jointDataIndex].m_scales;
        return !values.empty() ? values[indexA].Lerp(values[indexB], t) : m_staticJointData[jointDataIndex].m_staticTransform.m_scale;
    }
#endif

    Transform UniformMotionData::SampleJointTransform(float sampleTime, size_t jointDataIndex) const
    {
        float t;
        size_t indexA;
        size_t indexB;
        CalculateInterpolationIndicesUniform(sampleTime, m_sampleSpacing, m_duration, m_numSamples, indexA, indexB, t);

        const AZStd::vector<AZ::Vector3>& posValues = m_jointData[jointDataIndex].m_positions;
        const AZStd::vector<MCore::Compressed16BitQuaternion>& rotValues = m_jointData[jointDataIndex].m_rotations;
        const AZStd::vector<AZ::Vector3>& scaleValues = m_jointData[jointDataIndex].m_scales;
#ifndef EMFX_SCALE_DISABLED
        const StaticJointData& staticData = m_staticJointData[jointDataIndex];
#endif

        return Transform
        (
            !posValues.empty() ? posValues[indexA].Lerp(posValues[indexB], t) : staticData.m_staticTransform.m_scale,
            !rotValues.empty() ? rotValues[indexA].ToQuaternion().NLerp(rotValues[indexB].ToQuaternion(), t) : staticData.m_staticTransform.m_rotation

#ifndef EMFX_SCALE_DISABLED
            ,!scaleValues.empty() ? scaleValues[indexA].Lerp(scaleValues[indexB], t) : staticData.m_staticTransform.m_scale
#endif
        );
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct File_UniformMotionData_Info
    {
        AZ::u32 m_numJoints = 0;
        AZ::u32 m_numMorphs = 0;
        AZ::u32 m_numFloats = 0;
        AZ::u32 m_numSamples = 0;
        float m_sampleRate = 30.0f;

        // Followed by:
        // File_UniformMotionData_Joint[m_numJoints]
        // File_UniformMotionData_Float[m_numMorphs]
        // File_UniformMotionData_Float[m_numFloats]
    };

    enum File_UniformMotionData_Flags : AZ::u8
    {
        IsAnimated = 1 << 0,
        IsPositionAnimated = 1 << 1,
        IsRotationAnimated = 1 << 2,
        IsScaleAnimated = 1 << 3
    };

    struct File_UniformMotionData_Joint
    {
        FileFormat::File16BitQuaternion m_staticRot { 0, 0, 0, (1 << 15) - 1 };  // First frames rotation.
        FileFormat::File16BitQuaternion m_bindPoseRot { 0, 0, 0, (1 << 15) - 1 };// Bind pose rotation.
        FileFormat::FileVector3         m_staticPos { 0.0f, 0.0f, 0.0f };        // First frame position.
        FileFormat::FileVector3         m_staticScale { 1.0f, 1.0f, 1.0f };      // First frame scale.
        FileFormat::FileVector3         m_bindPosePos { 0.0f, 0.0f, 0.0f };      // Bind pose position.
        FileFormat::FileVector3         m_bindPoseScale { 1.0f, 1.0f, 1.0f };    // Bind pose scale.
        AZ::u8                          m_flags = 0; // The flags (see File_UniformMotionData_Flags).

        // Followed by:
        // string : The name of the joint.
        // FileVector3[ File_UniformMotionData_Info.m_numSamples ]         (only when (m_flags & File_UniformMotionData_Flags::IsPositionAnimated) is true).
        // File16BitQuaternion[ File_UniformMotionData_Info.m_numSamples ] (only when (m_flags & File_UniformMotionData_Flags::IsRotationAnimated) is true).
        // FileVector3[ File_UniformMotionData_Info.m_numSamples ]         (only when (m_flags & File_UniformMotionData_Flags::IsScaleAnimated) is true).
    };

    struct File_UniformMotionData_Float
    {
        float m_staticValue = 0.0f; // The static (first frame) value.
        AZ::u8 m_flags = 0;         // The flags (see File_UniformMotionData_Flags).

        // Followed by:
        // String: The name of the channel.
        // float[ File_UniformMotionData_Info.m_numSamples ] (only when (m_flags & File_UniformMotionData_Flags::IsAnimated) is true).
    };
    //---------------------------------------------------------------------------------------

    bool SaveJoint(MCore::Stream* stream, const UniformMotionData* motionData, size_t jointDataIndex, const MotionData::SaveSettings& saveSettings)
    {
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

        File_UniformMotionData_Joint jointChunk;

        ExporterLib::CopyVector(jointChunk.m_staticPos, posePosition);
        ExporterLib::Copy16BitQuaternion(jointChunk.m_staticRot, poseRotation);
        ExporterLib::CopyVector(jointChunk.m_staticScale, poseScale);

        ExporterLib::CopyVector(jointChunk.m_bindPosePos, bindPosePosition);
        ExporterLib::Copy16BitQuaternion(jointChunk.m_bindPoseRot, bindPoseRotation);
        ExporterLib::CopyVector(jointChunk.m_bindPoseScale, bindPoseScale);

        // Setup the flags.
        AZ::u8 flags = 0;
        if (motionData->IsJointAnimated(jointDataIndex)) { flags |= File_UniformMotionData_Flags::IsAnimated; }
        if (motionData->IsJointPositionAnimated(jointDataIndex)) { flags |= File_UniformMotionData_Flags::IsPositionAnimated; }
        if (motionData->IsJointRotationAnimated(jointDataIndex)) { flags |= File_UniformMotionData_Flags::IsRotationAnimated; }
        EMFX_SCALECODE
        (
            if (motionData->IsJointScaleAnimated(jointDataIndex)) { flags |= File_UniformMotionData_Flags::IsScaleAnimated; }
        )
        jointChunk.m_flags = flags;

        if (saveSettings.m_logDetails)
        {
            // Create an uncompressed version of the quaternions, for logging.
            const AZ::Quaternion uncompressedPoseRot = MCore::Compressed16BitQuaternion(jointChunk.m_staticRot.m_x, jointChunk.m_staticRot.m_y, jointChunk.m_staticRot.m_z, jointChunk.m_staticRot.m_w).ToQuaternion().GetNormalized();
            const AZ::Quaternion uncompressedBindPoseRot = MCore::Compressed16BitQuaternion(jointChunk.m_bindPoseRot.m_x, jointChunk.m_bindPoseRot.m_y, jointChunk.m_bindPoseRot.m_z, jointChunk.m_bindPoseRot.m_w).ToQuaternion().GetNormalized();

            MCore::LogDetailedInfo("- Motion Joint: %s", motionData->GetJointName(jointDataIndex).c_str());
            MCore::LogDetailedInfo("   + Static Translation:    x=%f y=%f z=%f", jointChunk.m_staticPos.m_x, jointChunk.m_staticPos.m_y, jointChunk.m_staticPos.m_z);
            MCore::LogDetailedInfo("   + Static Rotation:       x=%f y=%f z=%f w=%f", static_cast<float>(uncompressedPoseRot.GetX()), static_cast<float>(uncompressedPoseRot.GetY()), static_cast<float>(uncompressedPoseRot.GetZ()), static_cast<float>(uncompressedPoseRot.GetW()));
            MCore::LogDetailedInfo("   + Static Scale:          x=%f y=%f z=%f", jointChunk.m_staticScale.m_x, jointChunk.m_staticScale.m_y, jointChunk.m_staticScale.m_z);
            MCore::LogDetailedInfo("   + Bind Pose Translation: x=%f y=%f z=%f", jointChunk.m_bindPosePos.m_x, jointChunk.m_bindPosePos.m_y, jointChunk.m_bindPosePos.m_z);
            MCore::LogDetailedInfo("   + Bind Pose Rotation:    x=%f y=%f z=%f w=%f", static_cast<float>(uncompressedBindPoseRot.GetX()), static_cast<float>(uncompressedBindPoseRot.GetY()), static_cast<float>(uncompressedBindPoseRot.GetZ()), static_cast<float>(uncompressedBindPoseRot.GetW()));
            MCore::LogDetailedInfo("   + Bind Pose Scale:       x=%f y=%f z=%f", jointChunk.m_bindPoseScale.m_x, jointChunk.m_bindPoseScale.m_y, jointChunk.m_bindPoseScale.m_z);
            MCore::LogDetailedInfo("   + Position Animated:     %s", (flags & File_UniformMotionData_Flags::IsPositionAnimated) ? "Yes" : "No");
            MCore::LogDetailedInfo("   + Rotation Animated:     %s", (flags & File_UniformMotionData_Flags::IsRotationAnimated) ? "Yes" : "No");
            MCore::LogDetailedInfo("   + Scale Animated:        %s", (flags & File_UniformMotionData_Flags::IsScaleAnimated) ? "Yes" : "No");
        }

        // Convert endian.
        const MCore::Endian::EEndianType targetEndianType = saveSettings.m_targetEndianType;
        ExporterLib::ConvertFileVector3(&jointChunk.m_staticPos, targetEndianType);
        ExporterLib::ConvertFile16BitQuaternion(&jointChunk.m_staticRot, targetEndianType);
        ExporterLib::ConvertFileVector3(&jointChunk.m_staticScale, targetEndianType);

        ExporterLib::ConvertFileVector3(&jointChunk.m_bindPosePos, targetEndianType);
        ExporterLib::ConvertFile16BitQuaternion(&jointChunk.m_bindPoseRot, targetEndianType);
        ExporterLib::ConvertFileVector3(&jointChunk.m_bindPoseScale, targetEndianType);

        stream->Write(&jointChunk, sizeof(File_UniformMotionData_Joint));

        // Write the joint name.
        ExporterLib::SaveString(motionData->GetJointName(jointDataIndex), stream, targetEndianType);

        // Write position samples.
        const size_t numPositionSamples = motionData->IsJointPositionAnimated(jointDataIndex) ? motionData->GetNumSamples() : 0;
        for (size_t s = 0; s < numPositionSamples; ++s)
        {
            FileFormat::FileVector3 sampleValue;
            ExporterLib::CopyVector(sampleValue, AZ::PackedVector3f(motionData->GetJointPositionSample(jointDataIndex, s).m_value));
            ExporterLib::ConvertFileVector3(&sampleValue, targetEndianType);
            if (stream->Write(&sampleValue, sizeof(FileFormat::FileVector3)) == 0)
            {
                return false;
            }
        }

        // Write rotation samples.
        const size_t numRotationSamples = motionData->IsJointRotationAnimated(jointDataIndex) ? motionData->GetNumSamples() : 0;
        for (size_t s = 0; s < numRotationSamples; ++s)
        {
            FileFormat::File16BitQuaternion sampleValue;
            ExporterLib::Copy16BitQuaternion(sampleValue, motionData->GetJointRotationSample(jointDataIndex, s).m_value);
            ExporterLib::ConvertFile16BitQuaternion(&sampleValue, targetEndianType);
            if (stream->Write(&sampleValue, sizeof(FileFormat::File16BitQuaternion)) == 0)
            {
                return false;
            }
        }

        // Write scale samples.
        EMFX_SCALECODE
        (
            const size_t numScaleSamples = motionData->IsJointScaleAnimated(jointDataIndex) ? motionData->GetNumSamples() : 0;
            for (size_t s = 0; s < numScaleSamples; ++s)
            {
                FileFormat::FileVector3 sampleValue;
                ExporterLib::CopyVector(sampleValue, AZ::PackedVector3f(motionData->GetJointScaleSample(jointDataIndex, s).m_value));
                ExporterLib::ConvertFileVector3(&sampleValue, targetEndianType);
                if (stream->Write(&sampleValue, sizeof(FileFormat::FileVector3)) == 0)
                {
                    return false;
                }
            }
        )

        return true;
    }

    bool SaveMorph(MCore::Stream* stream, const UniformMotionData* motionData, size_t dataIndex, const MotionData::SaveSettings& saveSettings)
    {
        if (saveSettings.m_logDetails)
        {
            MCore::LogInfo("Saving morph with name '%s'", motionData->GetMorphName(dataIndex).c_str());
        }

        const AZStd::string& channelName = motionData->GetMorphName(dataIndex);
        if (channelName.empty())
        {
            MCore::LogError("Cannot save morph channel with empty name.");
            return false;
        }

        File_UniformMotionData_Float floatChunk;
        floatChunk.m_staticValue = motionData->GetMorphStaticValue(dataIndex);
        floatChunk.m_flags = motionData->IsMorphAnimated(dataIndex) ? File_UniformMotionData_Flags::IsAnimated : 0;

        if (saveSettings.m_logDetails)
        {
            MCore::LogDetailedInfo("    - Morph: '%s'", channelName.c_str());
            MCore::LogDetailedInfo("       + Static Weight = %f", floatChunk.m_staticValue);
            MCore::LogDetailedInfo("       + IsAnimated    = %s", motionData->IsMorphAnimated(dataIndex) ? "Yes" : "No");
        }

        // convert endian
        const MCore::Endian::EEndianType targetEndianType = saveSettings.m_targetEndianType;
        ExporterLib::ConvertFloat(&floatChunk.m_staticValue, targetEndianType);
        if (stream->Write(&floatChunk, sizeof(File_UniformMotionData_Float)) == 0)
        {
            return false;
        }
        ExporterLib::SaveString(channelName, stream, targetEndianType);

        // Save the samples.
        const size_t numSamples = motionData->IsMorphAnimated(dataIndex) ? motionData->GetNumSamples() : 0;
        for (size_t s = 0; s < numSamples; ++s)
        {
            float sampleValue = motionData->GetMorphSample(dataIndex, s).m_value;
            ExporterLib::ConvertFloat(&sampleValue, targetEndianType);
            if (stream->Write(&sampleValue, sizeof(float)) == 0)
            {
                return false;
            }
        }

        return true;
    }

    bool SaveFloat(MCore::Stream* stream, const UniformMotionData* motionData, size_t dataIndex, const MotionData::SaveSettings& saveSettings)
    {
        if (saveSettings.m_logDetails)
        {
            MCore::LogInfo("Saving float with name '%s'", motionData->GetFloatName(dataIndex).c_str());
        }

        const AZStd::string& channelName = motionData->GetFloatName(dataIndex);
        if (channelName.empty())
        {
            MCore::LogError("Cannot save float channel with empty name.");
            return false;
        }

        File_UniformMotionData_Float floatChunk;
        floatChunk.m_staticValue = motionData->GetFloatStaticValue(dataIndex);
        floatChunk.m_flags = motionData->IsFloatAnimated(dataIndex) ? File_UniformMotionData_Flags::IsAnimated : 0;

        if (saveSettings.m_logDetails)
        {
            MCore::LogDetailedInfo("    - Float Channel: '%s'", channelName.c_str());
            MCore::LogDetailedInfo("       + Static Weight = %f", floatChunk.m_staticValue);
            MCore::LogDetailedInfo("       + IsAnimated    = %s", motionData->IsFloatAnimated(dataIndex) ? "Yes" : "No");
        }

        // convert endian
        const MCore::Endian::EEndianType targetEndianType = saveSettings.m_targetEndianType;
        ExporterLib::ConvertFloat(&floatChunk.m_staticValue, targetEndianType);
        if (stream->Write(&floatChunk, sizeof(File_UniformMotionData_Float)) == 0)
        {
            return false;
        }
        ExporterLib::SaveString(channelName, stream, targetEndianType);

        // Save the samples.
        const size_t numSamples = motionData->IsFloatAnimated(dataIndex) ? motionData->GetNumSamples() : 0;
        for (size_t s = 0; s < numSamples; ++s)
        {
            float sampleValue = motionData->GetFloatSample(dataIndex, s).m_value;
            ExporterLib::ConvertFloat(&sampleValue, targetEndianType);
            if (stream->Write(&sampleValue, sizeof(float)) == 0)
            {
                return false;
            }
        }

        return true;
    }

    size_t UniformMotionData::CalcStreamSaveSizeInBytes([[maybe_unused]] const SaveSettings& saveSettings) const
    {
        size_t numBytes = 0;

        numBytes += sizeof(File_UniformMotionData_Info);

        // Add the joints to the size.
        const size_t numSamples = GetNumSamples();
        const size_t numJoints = GetNumJoints();
        for (size_t i = 0; i < numJoints; ++i)
        {
            numBytes += sizeof(File_UniformMotionData_Joint);
            numBytes += ExporterLib::GetStringChunkSize(GetJointName(i));
            numBytes += IsJointPositionAnimated(i) ? static_cast<AZ::u32>(numSamples * sizeof(FileFormat::FileVector3)) : 0;
            numBytes += IsJointRotationAnimated(i) ? static_cast<AZ::u32>(numSamples * sizeof(FileFormat::File16BitQuaternion)) : 0;
            EMFX_SCALECODE
            (
                numBytes += IsJointScaleAnimated(i) ? static_cast<AZ::u32>(numSamples * sizeof(FileFormat::FileVector3)) : 0;
            )
        }

        // Add the morphs channels to the size.
        const size_t numMorphs = GetNumMorphs();
        for (size_t i = 0; i < numMorphs; ++i)
        {
            numBytes += sizeof(File_UniformMotionData_Float);
            numBytes += ExporterLib::GetStringChunkSize(GetMorphName(i));
            numBytes += IsMorphAnimated(i) ? static_cast<AZ::u32>(numSamples * sizeof(float)) : 0;
        }

        // Add the float channels to the size.
        const size_t numFloats = GetNumFloats();
        for (size_t i = 0; i < numFloats; ++i)
        {
            numBytes += sizeof(File_UniformMotionData_Float);
            numBytes += ExporterLib::GetStringChunkSize(GetFloatName(i));
            numBytes += IsFloatAnimated(i) ? static_cast<AZ::u32>(numSamples * sizeof(float)) : 0;
        }

        return numBytes;
    }

    AZ::u32 UniformMotionData::GetStreamSaveVersion() const
    {
        return 1;
    }

    bool UniformMotionData::Save(MCore::Stream* stream, const SaveSettings& saveSettings) const
    {
        // Write the info chunk.
        File_UniformMotionData_Info info;
        info.m_numJoints = static_cast<AZ::u32>(GetNumJoints());
        info.m_numMorphs = static_cast<AZ::u32>(GetNumMorphs());
        info.m_numFloats = static_cast<AZ::u32>(GetNumFloats());
        info.m_numSamples = static_cast<AZ::u32>(GetNumSamples());
        info.m_sampleRate = GetSampleRate();
        const MCore::Endian::EEndianType targetEndianType = saveSettings.m_targetEndianType;
        ExporterLib::ConvertUnsignedInt(&info.m_numJoints, targetEndianType);
        ExporterLib::ConvertUnsignedInt(&info.m_numMorphs, targetEndianType);
        ExporterLib::ConvertUnsignedInt(&info.m_numFloats, targetEndianType);
        ExporterLib::ConvertUnsignedInt(&info.m_numSamples, targetEndianType);
        ExporterLib::ConvertFloat(&info.m_sampleRate, targetEndianType);
        if (stream->Write(&info, sizeof(File_UniformMotionData_Info)) == 0)
        {
            return false;
        }

        // Write the joints channels.
        for (size_t i = 0; i < GetNumJoints(); i++)
        {
            if (!SaveJoint(stream, this, i, saveSettings))
            {
                return false;
            }
        }

        // Write the morph channels.
        for (size_t i = 0; i < GetNumMorphs(); i++)
        {
            if (!SaveMorph(stream, this, i, saveSettings))
            {
                return false;
            }
        }

        // Write the float channels.
        for (size_t i = 0; i < GetNumFloats(); i++)
        {
            if (!SaveFloat(stream, this, i, saveSettings))
            {
                return false;
            }
        }

        return true;
    }

    bool ReadVersion1(MCore::Stream* stream, UniformMotionData* motionData, const MotionData::ReadSettings& readSettings)
    {
        // Read the info header.
        File_UniformMotionData_Info info;
        if (stream->Read(&info, sizeof(File_UniformMotionData_Info)) == 0)
        {
            return false;
        }
        const MCore::Endian::EEndianType sourceEndianType = readSettings.m_sourceEndianType;
        MCore::Endian::ConvertUnsignedInt32(&info.m_numJoints, sourceEndianType);
        MCore::Endian::ConvertUnsignedInt32(&info.m_numMorphs, sourceEndianType);
        MCore::Endian::ConvertUnsignedInt32(&info.m_numFloats, sourceEndianType);
        MCore::Endian::ConvertUnsignedInt32(&info.m_numSamples, sourceEndianType);
        MCore::Endian::ConvertFloat(&info.m_sampleRate, sourceEndianType);

        if (readSettings.m_logDetails)
        {
            MCore::LogDetailedInfo("- NonUniformMotionData:");
            MCore::LogDetailedInfo("  + NumJoints  = %d", info.m_numJoints);
            MCore::LogDetailedInfo("  + NumMorphs  = %d", info.m_numMorphs);
            MCore::LogDetailedInfo("  + NumFloats  = %d", info.m_numFloats);
            MCore::LogDetailedInfo("  + SampleRate = %f", info.m_sampleRate);
        }

        // Initialize the motion data.
        UniformMotionData::InitSettings initSettings;
        initSettings.m_numJoints = info.m_numJoints;
        initSettings.m_numMorphs = info.m_numMorphs;
        initSettings.m_numFloats = info.m_numFloats;
        initSettings.m_numSamples = info.m_numSamples;
        initSettings.m_sampleRate = info.m_sampleRate;
        motionData->Init(initSettings);

        // Read all joints.
        AZStd::string name;
        for (size_t i = 0; i < motionData->GetNumJoints(); ++i)
        {
            File_UniformMotionData_Joint jointInfo;
            if (stream->Read(&jointInfo, sizeof(File_UniformMotionData_Joint)) == 0)
            {
                return false;
            }

            // Convert endian.
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

            // Update the values.
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
                MCore::LogDetailedInfo("    - IsAnimated      = %s", (jointInfo.m_flags & File_UniformMotionData_Flags::IsAnimated) ? "Yes" : "No");
                MCore::LogDetailedInfo("    - IsPosAnimated   = %s", (jointInfo.m_flags & File_UniformMotionData_Flags::IsAnimated) ? "Yes" : "No");
                MCore::LogDetailedInfo("    - IsRotAnimated   = %s", (jointInfo.m_flags & File_UniformMotionData_Flags::IsAnimated) ? "Yes" : "No");
                MCore::LogDetailedInfo("    - IsScaleAnimated = %s", (jointInfo.m_flags & File_UniformMotionData_Flags::IsAnimated) ? "Yes" : "No");
            }

            // Read the position samples.
            const size_t numPositionSamples = (jointInfo.m_flags & File_UniformMotionData_Flags::IsPositionAnimated) ? info.m_numSamples : 0;
            if (numPositionSamples > 0)
            {
                motionData->AllocateJointPositionSamples(i);
                for (size_t s = 0; s < numPositionSamples; ++s)
                {
                    FileFormat::FileVector3 fileVector;
                    if (stream->Read(&fileVector, sizeof(FileFormat::FileVector3)) == 0) // Optimization idea: read all samples in a single Read call instead.
                    {
                        return false;
                    }
                    MCore::Endian::ConvertFloat(&fileVector.m_x, sourceEndianType, /*numFloats=*/3);
                    motionData->SetJointPositionSample(i, s, AZ::Vector3(fileVector.m_x, fileVector.m_y, fileVector.m_z));
                }
            }

            // Read the rotation samples.
            const size_t numRotationSamples = (jointInfo.m_flags & File_UniformMotionData_Flags::IsRotationAnimated) ? info.m_numSamples : 0;
            if (numRotationSamples > 0)
            {
                motionData->AllocateJointRotationSamples(i);
                for (size_t s = 0; s < numRotationSamples; ++s)
                {
                    FileFormat::File16BitQuaternion fileQuat;
                    if (stream->Read(&fileQuat, sizeof(FileFormat::File16BitQuaternion)) == 0) // Optimization idea: read all samples in a single Read call instead.
                    {
                        return false;
                    }
                    MCore::Compressed16BitQuaternion compressedQuat(fileQuat.m_x, fileQuat.m_y, fileQuat.m_z, fileQuat.m_w);
                    MCore::Endian::Convert16BitQuaternion(&compressedQuat, sourceEndianType);
                    motionData->SetJointRotationSample(i, s, compressedQuat.ToQuaternion().GetNormalized());
                }
            }

            // Read the scale samples.
            const size_t numScaleSamples = (jointInfo.m_flags & File_UniformMotionData_Flags::IsScaleAnimated) ? info.m_numSamples : 0;
            if (numScaleSamples > 0)
            {
                EMFX_SCALECODE
                (
                    motionData->AllocateJointScaleSamples(i);
                )
                for (size_t s = 0; s < numScaleSamples; ++s)
                {
                    FileFormat::FileVector3 fileVector;
                    if (stream->Read(&fileVector, sizeof(FileFormat::FileVector3)) == 0) // Optimization idea: read all samples in a single Read call instead.
                    {
                        return false;
                    }
                    EMFX_SCALECODE
                    (
                        MCore::Endian::ConvertFloat(&fileVector.m_x, sourceEndianType, /*numFloats=*/3);
                        motionData->SetJointScaleSample(i, s, AZ::Vector3(fileVector.m_x, fileVector.m_y, fileVector.m_z));
                    )
                }
            }
        } // For all joints.

        // Load morphs.
        for (size_t i = 0; i < motionData->GetNumMorphs(); ++i)
        {
            File_UniformMotionData_Float floatInfo;
            if (stream->Read(&floatInfo, sizeof(File_UniformMotionData_Float)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertFloat(&floatInfo.m_staticValue, sourceEndianType);
            name = MotionData::ReadStringFromStream(stream, sourceEndianType);

            if (readSettings.m_logDetails)
            {
                MCore::LogDetailedInfo("  + Morph: '%s'", name.c_str());
                MCore::LogDetailedInfo("       + IsAnimated   = %s", (floatInfo.m_flags & File_UniformMotionData_Flags::IsAnimated) ? "Yes" : "No");
                MCore::LogDetailedInfo("       + Static value = %f", floatInfo.m_staticValue);
            }

            motionData->SetMorphName(i, name);
            motionData->SetMorphStaticValue(i, floatInfo.m_staticValue);

            // Read samples.
            const size_t numSamples = (floatInfo.m_flags & File_UniformMotionData_Flags::IsAnimated) ? info.m_numSamples : 0;
            if (numSamples > 0)
            {
                motionData->AllocateMorphSamples(i);
                for (size_t s = 0; s < numSamples; ++s)
                {
                    float value;
                    if (stream->Read(&value, sizeof(float)) == 0) // Optimization idea: read all samples in a single Read call instead.
                    {
                        return false;
                    }
                    MCore::Endian::ConvertFloat(&value, sourceEndianType);
                    motionData->SetMorphSample(i, s, value);
                }
            }
        }

        // Load floats.
        for (size_t i = 0; i < motionData->GetNumFloats(); ++i)
        {
            File_UniformMotionData_Float floatInfo;
            if (stream->Read(&floatInfo, sizeof(File_UniformMotionData_Float)) == 0)
            {
                return false;
            }
            MCore::Endian::ConvertFloat(&floatInfo.m_staticValue, sourceEndianType);
            name = MotionData::ReadStringFromStream(stream, sourceEndianType);

            if (readSettings.m_logDetails)
            {
                MCore::LogDetailedInfo("  + Float: '%s'", name.c_str());
                MCore::LogDetailedInfo("       + IsAnimated   = %s", (floatInfo.m_flags & File_UniformMotionData_Flags::IsAnimated) ? "Yes" : "No");
                MCore::LogDetailedInfo("       + Static value = %f", floatInfo.m_staticValue);
            }

            motionData->SetFloatName(i, name);
            motionData->SetFloatStaticValue(i, floatInfo.m_staticValue);

            // Read samples.
            const size_t numSamples = (floatInfo.m_flags & File_UniformMotionData_Flags::IsAnimated) ? info.m_numSamples : 0;
            if (numSamples > 0)
            {
                motionData->AllocateFloatSamples(i);
                for (size_t s = 0; s < numSamples; ++s)
                {
                    float value;
                    if (stream->Read(&value, sizeof(float)) == 0) // Optimization idea: read all samples in a single Read call instead.
                    {
                        return false;
                    }
                    MCore::Endian::ConvertFloat(&value, sourceEndianType);
                    motionData->SetFloatSample(i, s, value);
                }
            }
        }

        return true;
    } 

    bool UniformMotionData::Read(MCore::Stream* stream, const ReadSettings& readSettings)
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
                AZ_Error("EMotionFX", false, "Unsupported UniformMotionData version (version=%d), cannot load motion data.", readSettings.m_version);
            }
        }

        return false;
    }

} // namespace EMotionFX
