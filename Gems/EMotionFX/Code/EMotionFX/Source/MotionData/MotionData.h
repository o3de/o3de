/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Endian.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/Transform.h>
#include <EMotionFX/Source/MotionData/MotionDataSampleSettings.h>
#include <EMotionFX/Source/MotionData/RootMotionExtractionData.h>

namespace MCore
{
    class Stream;
} // namespace MCore

namespace EMotionFX
{
    class Pose;
    class MotionInstance;
    class ActorInstance;
    class Actor;
    class NonUniformMotionData;

    class EMFX_API MotionLinkData
    {
    public:
        AZ_CLASS_ALLOCATOR(MotionLinkData, MotionAllocator)
        AZ_RTTI(MotionLinkData, "{4FE3628A-9F8D-4C55-9E9B-DF77405DC4F0}")

        MotionLinkData() = default;
        MotionLinkData(const MotionLinkData&) = default;
        MotionLinkData(MotionLinkData&&) = default;
        MotionLinkData& operator=(const MotionLinkData&) = default;
        MotionLinkData& operator=(MotionLinkData&&) = default;
        virtual ~MotionLinkData() = default;

        AZStd::vector<size_t>& GetJointDataLinks() { return m_jointDataLinks; }
        const AZStd::vector<size_t>& GetJointDataLinks() const { return m_jointDataLinks; }
        bool IsJointActive(size_t jointIndex) const { return (m_jointDataLinks[jointIndex] != InvalidIndex); }
        size_t GetJointDataLink(size_t jointIndex) const { return m_jointDataLinks[jointIndex]; }

    protected:
        AZStd::vector<size_t> m_jointDataLinks;
    };

    class EMFX_API MotionLinkCache
        : public EventHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(MotionLinkCache, MotionAllocator)

        MotionLinkCache();
        MotionLinkCache(const MotionLinkCache&) = delete;
        MotionLinkCache(MotionLinkCache&&) = delete;
        MotionLinkCache& operator=(const MotionLinkCache&) = delete;
        MotionLinkCache& operator=(MotionLinkCache&&) = delete;
        ~MotionLinkCache() override;

        void Register(const Actor* actor, AZStd::unique_ptr<const MotionLinkData> data);
        const MotionLinkData* FindMotionLinkData(const Actor* actor) const;

        size_t GetNumEntries() const { return m_motionLinkDataMap.size(); }

    private:
        const AZStd::vector<EventTypes> GetHandledEventTypes() const override { return { EVENT_TYPE_ON_DELETE_ACTOR }; }
        void OnDeleteActor(Actor* actor) override;

    private:
        AZStd::unordered_map<const Actor*,
            AZStd::unique_ptr<const MotionLinkData>,
            AZStd::hash<const Actor*>,
            AZStd::equal_to<const Actor*>,
            AZ::AZStdAlloc<MotionAllocator>>
            m_motionLinkDataMap;

        mutable AZStd::shared_mutex m_mutex;
    };

    class EMFX_API MotionData
    {
    public:
        AZ_CLASS_ALLOCATOR(MotionData, MotionAllocator)
        AZ_RTTI(MotionData, "{3785996B-A523-475A-ADEF-58DDBB5E144B}")

        friend class MotionDataFactory;

        // Keyframes.
        template <class T>
        struct EMFX_API Key
        {
            float m_time;
            T m_value;
        };
        using Vector3Key = Key<AZ::Vector3>;
        using QuaternionKey = Key<AZ::Quaternion>;
        using FloatKey = Key<float>;

        struct EMFX_API OptimizeSettings
        {
            AZStd::vector<size_t> m_jointIgnoreList; // The joint data indices to skip optimization for.
            AZStd::vector<size_t> m_morphIgnoreList; // The morph data indices to skip optimization for.
            AZStd::vector<size_t> m_floatIgnoreList; // The float data indices to skip optimization for.
            float m_maxPosError = 0.001f;   // In units.
            float m_maxRotError = 0.01f;    // In degrees.
            float m_maxScaleError = 0.001f; // In scale factor.
            float m_maxMorphError = 0.001f; // Morph difference.
            float m_maxFloatError = 0.001f; // Float difference.
            bool m_updateDuration = false;
        };

        struct EMFX_API ReadSettings
        {
            MCore::Endian::EEndianType m_sourceEndianType = MCore::Endian::EEndianType::ENDIAN_LITTLE;
            AZ::u32 m_version = 1;
            bool m_logDetails = false;
        };

        struct EMFX_API SaveSettings
        {
            MCore::Endian::EEndianType m_targetEndianType = MCore::Endian::EEndianType::ENDIAN_LITTLE;
            bool m_logDetails = false;
        };

        struct RootMotionExtractionSettings
        {
            bool m_transitionZeroXAxis = false;
            bool m_transitionZeroYAxis = false;
            AZStd::string m_sampleJoint = "Hip";
        };

        MotionData() = default;
        MotionData(const MotionData&) = delete;
        MotionData(MotionData&&) = delete;
        MotionData& operator=(const MotionData&) = delete;
        MotionData& operator=(MotionData&&) = delete;
        virtual ~MotionData() = default;

        virtual void InitFromNonUniformData(const NonUniformMotionData* motionData, bool keepSameSampleRate=true, float newSampleRate=30.0f, bool updateDuration=false) = 0;
        virtual void Optimize([[maybe_unused]] const OptimizeSettings& settings) {}
        virtual bool Read(MCore::Stream* stream, const ReadSettings& readSettings) = 0;
        virtual bool Save(MCore::Stream* stream, const SaveSettings& saveSettings) const = 0;
        virtual size_t CalcStreamSaveSizeInBytes(const SaveSettings& saveSettings) const = 0;
        virtual AZ::u32 GetStreamSaveVersion() const = 0;
        virtual bool GetSupportsOptimizeSettings() const { return true; }
        virtual const char* GetSceneSettingsName() const = 0;

        // Sampling
        virtual Transform SampleJointTransform(const MotionDataSampleSettings& settings, size_t jointSkeletonIndex) const = 0;
        virtual void SamplePose(const MotionDataSampleSettings& settings, Pose* outputPose) const = 0;
        virtual float SampleMorph(float sampleTime, size_t morphDataIndex) const = 0;
        virtual float SampleFloat(float sampleTime, size_t morphDataIndex) const = 0;

        virtual Transform SampleJointTransform(float sampleTime, size_t jointIndex) const = 0;
        virtual AZ::Vector3 SampleJointPosition(float sampleTime, size_t jointDataIndex) const = 0;
        virtual AZ::Quaternion SampleJointRotation(float sampleTime, size_t jointDataIndex) const = 0;

        bool SampleJointPosition(float sampleTime, AZ::u32 jointNameId, AZ::Vector3& resultValue) const;
        bool SampleJointRotation(float sampleTime, AZ::u32 jointNameId, AZ::Quaternion& resultValue) const;
        bool SampleMorph(float sampleTime, AZ::u32 id, float& resultValue) const;
        bool SampleFloat(float sampleTime, AZ::u32 id, float& resultValue) const;

        virtual void ClearAllJointTransformSamples() = 0;
        virtual void ClearAllMorphSamples() = 0;
        virtual void ClearAllFloatSamples() = 0;
        virtual void ClearJointPositionSamples(size_t jointDataIndex) = 0;
        virtual void ClearJointRotationSamples(size_t jointDataIndex) = 0;
        virtual void ClearJointTransformSamples(size_t jointDataIndex) = 0;
        virtual void ClearMorphSamples(size_t morphDataIndex) = 0;
        virtual void ClearFloatSamples(size_t floatDataIndex) = 0;

        virtual bool IsJointPositionAnimated(size_t jointDataIndex) const = 0;
        virtual bool IsJointRotationAnimated(size_t jointDataIndex) const = 0;
        virtual bool IsJointAnimated(size_t jointDataIndex) const = 0;
        virtual bool IsMorphAnimated(size_t morphDataIndex) const = 0;
        virtual bool IsFloatAnimated(size_t floatDataIndex) const = 0;
        virtual void UpdateDuration() {}
        virtual bool VerifyIntegrity() const { return true; }

        void Resize(size_t numJoints, size_t numMorphs, size_t numFloats);
        void Clear();
        size_t AddJoint(const AZStd::string& name, const Transform& poseTransform, const Transform& bindPoseTransform);
        size_t AddMorph(const AZStd::string& name, float poseValue);
        size_t AddFloat(const AZStd::string& name, float poseValue);

        void RemoveJoint(size_t jointDataIndex);
        void RemoveMorph(size_t morphDataIndex);
        void RemoveFloat(size_t floatDataIndex);

        void Scale(float scaleFactor);

        float GetDuration() const;
        float GetSampleRate() const;

        void SetDuration(float duration);
        virtual void SetSampleRate(float sampleRate);

        AZ::Outcome<size_t> FindJointIndexByNameId(size_t nameId) const;
        AZ::Outcome<size_t> FindMorphIndexByNameId(AZ::u32 nameId) const;
        AZ::Outcome<size_t> FindFloatIndexByNameId(AZ::u32 nameId) const;

        size_t GetNumJoints() const;
        size_t GetNumMorphs() const;
        size_t GetNumFloats() const;

        const AZStd::string& GetJointName(size_t jointDataIndex) const;
        const AZStd::string& GetMorphName(size_t morphDataIndex) const;
        const AZStd::string& GetFloatName(size_t floatDataIndex) const;

        AZ::Vector3 GetJointStaticPosition(size_t jointDataIndex) const;
        AZ::Quaternion GetJointStaticRotation(size_t jointDataIndex) const;
        Transform GetJointStaticTransform(size_t jointDataIndex) const;

        AZ::Vector3 GetJointBindPosePosition(size_t jointDataIndex) const;
        AZ::Quaternion GetJointBindPoseRotation(size_t jointDataIndex) const;
        Transform GetJointBindPoseTransform(size_t jointDataIndex) const;

        float GetMorphStaticValue(size_t morphDataIndex) const;
        float GetFloatStaticValue(size_t floatDataIndex) const;

        AZ::u32 GetJointNameId(size_t jointDataIndex) const;
        AZ::u32 GetMorphNameId(size_t morphDataIndex) const;
        AZ::u32 GetFloatNameId(size_t floatDataIndex) const;

        AZ::Outcome<size_t> FindJointIndexByName(const AZStd::string& name) const;
        AZ::Outcome<size_t> FindMorphIndexByName(const AZStd::string& name) const;
        AZ::Outcome<size_t> FindFloatIndexByName(const AZStd::string& name) const;

        void SetJointName(size_t jointDataIndex, const AZStd::string& name);
        void SetMorphName(size_t morphDataIndex, const AZStd::string& name);
        void SetFloatName(size_t floatDataIndex, const AZStd::string& name);
        void SetJointNameId(size_t jointDataIndex, AZ::u32 id);
        void SetMorphNameId(size_t morphDataIndex, AZ::u32 id);
        void SetFloatNameId(size_t floatDataIndex, AZ::u32 id);
        void SetJointStaticTransform(size_t jointDataIndex, const Transform& transform);
        void SetJointStaticPosition(size_t jointDataIndex, const AZ::Vector3& position);
        void SetJointStaticRotation(size_t jointDataIndex, const AZ::Quaternion& rotation);
        void SetJointBindPoseTransform(size_t jointDataIndex, const Transform& transform);
        void SetJointBindPosePosition(size_t jointDataIndex, const AZ::Vector3& position);
        void SetJointBindPoseRotation(size_t jointDataIndex, const AZ::Quaternion& rotation);
        void SetMorphStaticValue(size_t morphDataIndex, float poseValue);
        void SetFloatStaticValue(size_t floatDataIndex, float poseValue);

        const MotionLinkData* FindMotionLinkData(const Actor* actor) const;
        size_t GetNumMotionLinkCacheEntries() const { return m_motionLinkCache.GetNumEntries(); }

        static void CalculateSampleInformation(float duration, float& inOutSampleRate, size_t& numSamples, float& sampleSpacing);
        static size_t CalculateNumRequiredSamples(float duration, float sampleSpacing);
        static void CalculateInterpolationIndicesNonUniform(const AZStd::vector<float>& timeValues, float sampleTime, size_t& indexA, size_t& indexB, float& t);
        static void CalculateInterpolationIndicesUniform(float sampleTime, float sampleSpacing, float duration, size_t numSamples, size_t& indexA, size_t& indexB, float& t);

        void BasicRetarget(const ActorInstance* actorInstance, const MotionLinkData* motionLinkData, size_t jointIndex, Transform& inOutTransform) const;

        bool IsAdditive() const;
        void SetAdditive(bool isAdditive);

#ifndef EMFX_SCALE_DISABLED
        virtual AZ::Vector3 SampleJointScale(float sampleTime, size_t jointDataIndex) const = 0;
        bool SampleJointScale(float sampleTime, AZ::u32 jointNameId, AZ::Vector3& resultValue) const;
        virtual void ClearJointScaleSamples(size_t jointDataIndex) = 0;
        virtual bool IsJointScaleAnimated(size_t jointDataIndex) const = 0;
        AZ::Vector3 GetJointStaticScale(size_t jointDataIndex) const;
        AZ::Vector3 GetJointBindPoseScale(size_t jointDataIndex) const;
        void SetJointStaticScale(size_t jointDataIndex, const AZ::Vector3& scale);
        void SetJointBindPoseScale(size_t jointDataIndex, const AZ::Vector3& scale);
#endif

        virtual void ExtractRootMotion(size_t sampleJointDataIndex, size_t rootJointDataIndex, const RootMotionExtractionData& data);

        static AZStd::string ReadStringFromStream(MCore::Stream* stream, MCore::Endian::EEndianType sourceEndianType);

    protected:
        struct EMFX_API StaticJointData
        {
            Transform m_staticTransform = Transform::CreateIdentity();
            Transform m_bindTransform = Transform::CreateIdentity();
            AZ::u32 m_nameId = InvalidIndex32;
        };

        struct EMFX_API StaticFloatData
        {
            float m_staticValue = 0.0f;
            AZ::u32 m_nameId = InvalidIndex32;
        };

        virtual void ResizeSampleData(size_t numJoints, size_t numMorphs, size_t numFloats) = 0;
        virtual void AddJointSampleData(size_t jointDataIndex) = 0;
        virtual void AddMorphSampleData(size_t morphDataIndex) = 0;
        virtual void AddFloatSampleData(size_t floatDataIndex) = 0;
        virtual void RemoveJointSampleData(size_t jointDataIndex) = 0;
        virtual void RemoveMorphSampleData(size_t morphDataIndex) = 0;
        virtual void RemoveFloatSampleData(size_t floatDataIndex) = 0;
        virtual void ClearAllData() = 0;
        virtual void ScaleData(float scaleFactor) = 0;

        virtual AZStd::unique_ptr<const MotionLinkData> CreateMotionLinkData(const Actor* actor) const;
        static AZ::Outcome<size_t> FindFloatDataIndexById(const AZStd::vector<StaticFloatData>& data, AZ::u32 id);

        void CopyBaseMotionData(const MotionData* motionData);

    protected:
        AZStd::vector<StaticJointData> m_staticJointData;
        AZStd::vector<StaticFloatData> m_staticMorphData;
        AZStd::vector<StaticFloatData> m_staticFloatData;
        float m_duration = 0.0f;
        float m_sampleRate = 30.0f;
        bool m_additive = false;
        bool m_rootMotionExtracted = false;

    private:
        void ClearBaseData();
        virtual MotionData* CreateNew() const = 0;

        mutable MotionLinkCache m_motionLinkCache;
        mutable AZStd::shared_mutex m_mutex;
    };
} // namespace EMotionFX
