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

#include <MCore/Source/CompressedQuaternion.h>

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>

namespace EMotionFX
{
    class Pose;
    class MotionInstance;

    class EMFX_API NonUniformMotionData
        : public MotionData
    {
    public:
        AZ_CLASS_ALLOCATOR(NonUniformMotionData, MotionAllocator, 0)
        AZ_RTTI(NonUniformMotionData, "{AD5CF6AD-A805-4F4C-BDBD-517538A9CB14}", MotionData)

        template <class T>
        struct EMFX_API KeyTrack
        {
            AZStd::vector<float> m_times;
            AZStd::vector<T> m_values;
        };
        using FloatTrack = KeyTrack<float>;
        using Vector3Track = KeyTrack<AZ::Vector3>;
        using QuaternionTrack = KeyTrack<MCore::Compressed16BitQuaternion>;

        NonUniformMotionData() = default;
        ~NonUniformMotionData() override;

        void InitFromNonUniformData(const NonUniformMotionData* motionData, bool keepSameSampleRate=true, float newSampleRate=30.0f, bool updateDuration=false) override;
        void RemoveRedundantKeyframes(bool updateDurationAfterwards=false);
        void Optimize(const OptimizeSettings& settings) override;
        bool Read(MCore::Stream* stream, const ReadSettings& readSettings) override;
        bool Save(MCore::Stream* stream, const SaveSettings& saveSettings) const override;
        size_t CalcStreamSaveSizeInBytes(const SaveSettings& saveSettings) const override;
        AZ::u32 GetStreamSaveVersion() const override;
        const char* GetSceneSettingsName() const override;

        Transform SampleJointTransform(const MotionDataSampleSettings& settings, size_t jointSkeletonIndex) const override;
        void SamplePose(const MotionDataSampleSettings& settings, Pose* outputPose) const override;

        Transform SampleJointTransform(float sampleTime, size_t jointDataIndex) const override;
        AZ::Vector3 SampleJointPosition(float sampleTime, size_t jointDataIndex) const override;
        AZ::Quaternion SampleJointRotation(float sampleTime, size_t jointDataIndex) const override;
        float SampleMorph(float sampleTime, size_t morphDataIndex) const override;
        float SampleFloat(float sampleTime, size_t floatDataIndex) const override;

        void AllocateJointPositionSamples(size_t jointDataIndex, size_t numSamples);
        void AllocateJointRotationSamples(size_t jointDataIndex, size_t numSamples);
        void AllocateMorphSamples(size_t morphDataIndex, size_t numSamples);
        void AllocateFloatSamples(size_t floatDataIndex, size_t numSamples);

        void ClearAllJointTransformSamples() override;
        void ClearAllMorphSamples() override;
        void ClearAllFloatSamples() override;
        void ClearJointPositionSamples(size_t jointDataIndex) override;
        void ClearJointRotationSamples(size_t jointDataIndex) override;
        void ClearJointTransformSamples(size_t jointDataIndex) override;
        void ClearMorphSamples(size_t morphDataIndex) override;
        void ClearFloatSamples(size_t floatDataIndex) override;

        bool VerifyIntegrity() const override;

        //! Animation tracks in the DCC tool formats are often stored individually, each having its own duration.
        //! For the motion data, it is required to have tracks with the same duration and e.g. a position track
        //! has to match the duration of a morph track. This will be automatically fixed by adding missing
        //! keyframes at the end of the tracks to match the animation's global duration. The value of these
        //! are the same as the last one of the given track so that they freeze at that value.
        void FixMissingEndKeyframes();

        void ScaleData(float scaleFactor) override;
        void UpdateDuration() override;

        Vector3Key GetJointPositionSample(size_t jointDataIndex, size_t sampleIndex) const;
        QuaternionKey GetJointRotationSample(size_t jointDataIndex, size_t sampleIndex) const;
        FloatKey GetMorphSample(size_t morphDataIndex, size_t sampleIndex) const;
        FloatKey GetFloatSample(size_t floatDataIndex, size_t sampleIndex) const;

        bool IsJointPositionAnimated(size_t jointDataIndex) const override;
        bool IsJointRotationAnimated(size_t jointDataIndex) const override;
        bool IsJointAnimated(size_t jointDataIndex) const override;
        bool IsMorphAnimated(size_t morphDataIndex) const override;
        bool IsFloatAnimated(size_t floatDataIndex) const override;

        size_t GetNumJointPositionSamples(size_t jointDataIndex) const;
        size_t GetNumJointRotationSamples(size_t jointDataIndex) const;
        size_t GetNumMorphSamples(size_t morphDataIndex) const;
        size_t GetNumFloatSamples(size_t floatDataIndex) const;

        const Vector3Track& GetJointPositionTrack(size_t jointDataIndex) const;
        const QuaternionTrack& GetJointRotationTrack(size_t jointDataIndex) const;
        const FloatTrack& GetMorphTrack(size_t morphDataIndex) const;
        const FloatTrack& GetFloatTrack(size_t floatDataIndex) const;

        void SetJointPositionSample(size_t jointDataIndex, size_t sampleIndex, const Vector3Key& key);
        void SetJointRotationSample(size_t jointDataIndex, size_t sampleIndex, const QuaternionKey& key);
        void SetMorphSample(size_t morphDataIndex, size_t sampleIndex, const FloatKey& key);
        void SetFloatSample(size_t floatDataIndex, size_t sampleIndex, const FloatKey& key);
        void SetJointPositionSamples(size_t jointDataIndex, const Vector3Track& track);
        void SetJointRotationSamples(size_t jointDataIndex, const QuaternionTrack& track);

#ifndef EMFX_SCALE_DISABLED
        void AllocateJointScaleSamples(size_t jointDataIndex, size_t numSamples);
        void ClearJointScaleSamples(size_t jointDataIndex) override;
        Vector3Key GetJointScaleSample(size_t jointDataIndex, size_t sampleIndex) const;
        bool IsJointScaleAnimated(size_t jointDataIndex) const override;
        size_t GetNumJointScaleSamples(size_t jointDataIndex) const;
        const Vector3Track& GetJointScaleTrack(size_t jointDataIndex) const;
        void SetJointScaleSample(size_t jointDataIndex, size_t sampleIndex, const Vector3Key& key);
        void SetJointScaleSamples(size_t jointDataIndex, const Vector3Track& track);
        AZ::Vector3 SampleJointScale(float sampleTime, size_t jointDataIndex) const override;
#endif

        NonUniformMotionData& operator=(const NonUniformMotionData& sourceMotionData);

    private:
        struct EMFX_API JointData
        {
            Vector3Track m_positionTrack;
            QuaternionTrack m_rotationTrack;
#ifndef EMFX_SCALE_DISABLED
            Vector3Track m_scaleTrack;
#endif
        };

        struct EMFX_API FloatData
        {
            FloatTrack m_track;
        };

        MotionData* CreateNew() const override;
        static bool VerifyKeyTrackTimeIntegrity(const AZStd::vector<float>& timeValues);
        bool VerifyStartEndTimeIntegrity(const AZStd::vector<float>& timeValues, bool& firstCheck, float& startTime, float& endTime) const;
        void ResizeSampleData(size_t numJoints, size_t numMorphs, size_t numFloats) override;
        void ClearAllData() override;
        void AddJointSampleData(size_t jointDataIndex) override;
        void AddMorphSampleData(size_t morphDataIndex) override;
        void AddFloatSampleData(size_t floatDataIndex) override;
        void RemoveJointSampleData(size_t jointDataIndex) override;
        void RemoveMorphSampleData(size_t morphDataIndex) override;
        void RemoveFloatSampleData(size_t floatDataIndex) override;

        template<class KeyTrackType>
        void FixMissingEndKeyframes(KeyTrackType& keytrack, float endTimeToMatch);

    private:
        AZStd::vector<JointData> m_jointData;
        AZStd::vector<FloatData> m_morphData;
        AZStd::vector<FloatData> m_floatData;
    };
} // namespace EMotionFX
