/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/Transform.h>

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>

#include <MCore/Source/CompressedQuaternion.h>

namespace EMotionFX
{
    class Pose;

    class EMFX_API UniformMotionData
        : public MotionData
    {
    public:
        AZ_CLASS_ALLOCATOR(UniformMotionData, MotionAllocator)
        AZ_RTTI(UniformMotionData, "{822B1269-FF6F-4406-A3A5-E1E39C289941}", MotionData)

        struct EMFX_API InitSettings
        {
            size_t m_numJoints = 0;
            size_t m_numMorphs = 0;
            size_t m_numFloats = 0;
            size_t m_numSamples = 0;
            float m_sampleRate = 30.0f;
        };

        UniformMotionData() = default;
        ~UniformMotionData() override;

        void InitFromNonUniformData(const NonUniformMotionData* motionData, bool keepSameSampleRate=true, float newSampleRate=30.0f, bool updateDuration=false) override;
        bool Read(MCore::Stream* stream, const ReadSettings& readSettings) override;
        bool Save(MCore::Stream* stream, const SaveSettings& saveSettings) const override;
        size_t CalcStreamSaveSizeInBytes(const SaveSettings& saveSettings) const override;
        AZ::u32 GetStreamSaveVersion() const override;
        bool GetSupportsOptimizeSettings() const override { return false; }
        const char* GetSceneSettingsName() const override;

        // Overloaded.
        Transform SampleJointTransform(const MotionDataSampleSettings& settings, size_t jointSkeletonIndex) const override;
        void SamplePose(const MotionDataSampleSettings& settings, Pose* outputPose) const override;
        float SampleMorph(float sampleTime, size_t morphDataIndex) const override;
        float SampleFloat(float sampleTime, size_t floatDataIndex) const override;
        Transform SampleJointTransform(float sampleTime, size_t jointDataIndex) const override;
        AZ::Vector3 SampleJointPosition(float sampleTime, size_t jointDataIndex) const override;
        AZ::Quaternion SampleJointRotation(float sampleTime, size_t jointDataIndex) const override;

        // Initialize, allocate and clear.
        void Init(const InitSettings& settings);
        void AllocateJointPositionSamples(size_t jointDataIndex);
        void AllocateJointRotationSamples(size_t jointDataIndex);
        void AllocateMorphSamples(size_t morphDataIndex);
        void AllocateFloatSamples(size_t floatDataIndex);

        void ClearAllJointTransformSamples() override;
        void ClearAllMorphSamples() override;
        void ClearAllFloatSamples() override;
        void ClearJointPositionSamples(size_t jointDataIndex) override;
        void ClearJointRotationSamples(size_t jointDataIndex) override;
        void ClearJointTransformSamples(size_t jointDataIndex) override;
        void ClearMorphSamples(size_t morphDataIndex) override;
        void ClearFloatSamples(size_t floatDataIndex) override;

        // Get data.
        Vector3Key GetJointPositionSample(size_t jointDataIndex, size_t sampleIndex) const;
        QuaternionKey GetJointRotationSample(size_t jointDataIndex, size_t sampleIndex) const;
        FloatKey GetMorphSample(size_t morphDataIndex, size_t sampleIndex) const;
        FloatKey GetFloatSample(size_t floatDataIndex, size_t sampleIndex) const;

        bool IsJointPositionAnimated(size_t jointDataIndex) const override;
        bool IsJointRotationAnimated(size_t jointDataIndex) const override;
        bool IsJointAnimated(size_t jointDataIndex) const override;
        bool IsMorphAnimated(size_t morphDataIndex) const override;
        bool IsFloatAnimated(size_t floatDataIndex) const override;

        void SetJointPositionSample(size_t jointDataIndex, size_t sampleIndex, const AZ::Vector3& position);
        void SetJointRotationSample(size_t jointDataIndex, size_t sampleIndex, const AZ::Quaternion& rotation);
        void SetMorphSample(size_t morphDataIndex, size_t sampleIndex, float value);
        void SetFloatSample(size_t floatDataIndex, size_t sampleIndex, float value);
        void SetJointPositionSamples(size_t jointDataIndex, const AZStd::vector<AZ::Vector3>& positions);
        void SetJointRotationSamples(size_t jointDataIndex, const AZStd::vector<AZ::Quaternion>& rotations);

#ifndef EMFX_SCALE_DISABLED
        void AllocateJointScaleSamples(size_t jointDataIndex);
        void ClearJointScaleSamples(size_t jointDataIndex) override;
        Vector3Key GetJointScaleSample(size_t jointDataIndex, size_t sampleIndex) const;
        bool IsJointScaleAnimated(size_t jointDataIndex) const override;
        void SetJointScaleSample(size_t jointDataIndex, size_t sampleIndex, const AZ::Vector3& scale);
        void SetJointScaleSamples(size_t jointDataIndex, const AZStd::vector<AZ::Vector3>& scales);
        AZ::Vector3 SampleJointScale(float sampleTime, size_t jointDataIndex) const override;
#endif

        size_t GetNumSamples() const;
        float GetSampleSpacing() const;
        void SetSampleRate(float sampleRate) override;
        void UpdateDuration() override;

    private:
        struct EMFX_API JointData
        {
            AZStd::vector<AZ::Vector3> m_positions;
            AZStd::vector<MCore::Compressed16BitQuaternion> m_rotations;
#ifndef EMFX_SCALE_DISABLED
            AZStd::vector<AZ::Vector3> m_scales;
#endif
        };

        struct EMFX_API FloatData
        {
            AZStd::vector<float> m_values;
        };

        MotionData* CreateNew() const override;
        void ResizeSampleData(size_t numJoints, size_t numMorphs, size_t numFloats) override;
        void ClearAllData() override;
        void AddJointSampleData(size_t jointDataIndex) override;
        void AddMorphSampleData(size_t morphDataIndex) override;
        void AddFloatSampleData(size_t floatDataIndex) override;
        void RemoveJointSampleData(size_t jointDataIndex) override;
        void RemoveMorphSampleData(size_t morphDataIndex) override;
        void RemoveFloatSampleData(size_t floatDataIndex) override;

    private:
        void ScaleData(float scaleFactor) override;
        void UpdateSampleSpacing();

        AZStd::vector<JointData> m_jointData;
        AZStd::vector<FloatData> m_morphData;
        AZStd::vector<FloatData> m_floatData;
        size_t m_numSamples = 0;
        float m_sampleSpacing = 1.0f / 30.0f;
    };
} // namespace EMotionFX
