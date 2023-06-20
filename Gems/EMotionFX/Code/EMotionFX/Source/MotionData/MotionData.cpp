/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <EMotionFX/Source/Algorithms.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/TransformData.h>

#include <MCore/Source/Endian.h>
#include <MCore/Source/Stream.h>

namespace EMotionFX
{
    MotionLinkCache::MotionLinkCache()
    {
        GetEMotionFX().GetEventManager()->AddEventHandler(this);
    }

    MotionLinkCache::~MotionLinkCache()
    {
        GetEMotionFX().GetEventManager()->RemoveEventHandler(this);
    }

    void MotionLinkCache::OnDeleteActor(Actor* actor)
    {
        AZStd::unique_lock<AZStd::shared_mutex> uniqueLock(m_mutex);
        m_motionLinkDataMap.erase(actor);
    }

    void MotionLinkCache::Register(const Actor* actor, AZStd::unique_ptr<const MotionLinkData> data)
    {
        AZ_Assert(data, "Expecting valid MotionLinkData pointer.");
        AZStd::unique_lock<AZStd::shared_mutex> uniqueLock(m_mutex);
        m_motionLinkDataMap.emplace(actor, AZStd::move(data));
    }

    //-------------------------------------------------------------------------

    const MotionLinkData* MotionLinkCache::FindMotionLinkData(const Actor* actor) const
    {
        AZStd::unique_lock<AZStd::shared_mutex> uniqueLock(m_mutex);
        const auto findResult = m_motionLinkDataMap.find(actor);
        if (findResult != m_motionLinkDataMap.end())
        {
            return findResult->second.get();
        }

        return nullptr;
    }

    const MotionLinkData* MotionData::FindMotionLinkData(const Actor* actor) const
    {
        AZStd::unique_lock<AZStd::shared_mutex> uniqueLock(m_mutex);

        const MotionLinkData* data = m_motionLinkCache.FindMotionLinkData(actor);
        if (data)
        {
            return data;
        }

        m_motionLinkCache.Register(actor, CreateMotionLinkData(actor));
        return m_motionLinkCache.FindMotionLinkData(actor);
    }

    AZStd::unique_ptr<const MotionLinkData> MotionData::CreateMotionLinkData(const Actor* actor) const
    {
        auto data = AZStd::make_unique<MotionLinkData>();
        const Skeleton* skeleton = actor->GetSkeleton();
        const size_t numJoints = skeleton->GetNumNodes();
        AZStd::vector<size_t>& jointLinks = data->GetJointDataLinks();
        jointLinks.resize(numJoints);
        for (size_t i = 0; i < numJoints; ++i)
        {
            const AZ::Outcome<size_t> findResult = FindJointIndexByNameId(skeleton->GetNode(i)->GetID());
            jointLinks[i] = findResult.IsSuccess() ? findResult.GetValue() : InvalidIndex;
        }
        return AZStd::move(data);
    }

    size_t MotionData::CalculateNumRequiredSamples(float duration, float sampleSpacing)
    {
        AZ_Assert(duration >= 0.0f, "Expecting the duration to be greater than or equal to zero.");
        AZ_Assert(sampleSpacing > 0.0f, "Expecting the sample spacing to be larger than zero.");
        if (sampleSpacing > duration)
        {
            return 2;
        }

        return static_cast<size_t>(duration / sampleSpacing) + 1;
    }

    void MotionData::CalculateSampleInformation(float duration, float& inOutSampleRate, size_t& numSamples, float& sampleSpacing)
    {
        if (inOutSampleRate == 0)
        {
            AZ_Assert(inOutSampleRate > 0.0f, "Sample rate cannot be zero.");
            inOutSampleRate = 1;
        }

        // Calculate the sample spacing before alignment correction/adjustments, and handle a special case where the sample spacing is larger than the duration.
        sampleSpacing = 1.0f / inOutSampleRate;
        if (duration < AZ::Constants::FloatEpsilon) // In case there is no animation present.
        {
            numSamples = 0;
            return;
        }
        if (sampleSpacing >= duration) // Short animation or low sample rate (or both), where sample spacing is bigger than the duration.
        {
            sampleSpacing = duration;
            numSamples = 2;
            inOutSampleRate = 1.0f / sampleSpacing;
            return;
        }

        // Distribute error over sample spacing.
        // For example if we have a sample rate of 5, with a duration of 11 seconds, we would adjust the sample rate from 5 to a slightly larger spacing
        // to perfectly align the last key to 11 seconds. This essentially reduces the sample rate slightly to perfectly align.
        numSamples = CalculateNumRequiredSamples(duration, sampleSpacing);
        const float timeStepError = AZ::GetMod(duration, sampleSpacing);
        const float beforeSampleTimeStep = sampleSpacing;
        sampleSpacing += timeStepError / static_cast<float>(numSamples - 1);
        if (static_cast<float>(sampleSpacing * (numSamples - 1)) > (duration + AZ::Constants::FloatEpsilon))
        {
            sampleSpacing = beforeSampleTimeStep;
        }

        inOutSampleRate = 1.0f / sampleSpacing;
    }

    AZ::Outcome<size_t> MotionData::FindFloatDataIndexById(const AZStd::vector<StaticFloatData>& data, AZ::u32 id)
    {
        // TODO: optimize using hashmap or so.
        const size_t numItems = data.size();
        for (size_t i = 0; i < numItems; ++i)
        {
            if (data[i].m_nameId == id)
            {
                return AZ::Success(i);
            }
        }
        return AZ::Failure();
    }

    size_t MotionData::GetNumJoints() const
    {
        return m_staticJointData.size();
    }

    size_t MotionData::GetNumMorphs() const
    {
        return m_staticMorphData.size();
    }

    size_t MotionData::GetNumFloats() const
    {
        return m_staticFloatData.size();
    }

    float MotionData::GetDuration() const
    {
        return m_duration;
    }

    AZ::Outcome<size_t> MotionData::FindJointIndexByName(const AZStd::string& name) const
    {
        return FindJointIndexByNameId(MCore::GetStringIdPool().GenerateIdForString(name));
    }

    AZ::Outcome<size_t> MotionData::FindMorphIndexByName(const AZStd::string& name) const
    {
        return FindMorphIndexByNameId(MCore::GetStringIdPool().GenerateIdForString(name));
    }

    AZ::Outcome<size_t> MotionData::FindFloatIndexByName(const AZStd::string& name) const
    {
        return FindFloatIndexByNameId(MCore::GetStringIdPool().GenerateIdForString(name));
    }

    AZ::Outcome<size_t> MotionData::FindJointIndexByNameId(size_t id) const
    {
        return FindIndexIf(m_staticJointData, [id](const StaticJointData& item) { return item.m_nameId == id; });
    }

    AZ::Outcome<size_t> MotionData::FindMorphIndexByNameId(AZ::u32 id) const
    {
        return FindIndexIf(m_staticMorphData, [id](const StaticFloatData& item) { return item.m_nameId == id; });
    }

    AZ::Outcome<size_t> MotionData::FindFloatIndexByNameId(AZ::u32 id) const
    {
        return FindIndexIf(m_staticFloatData, [id](const StaticFloatData& item) { return item.m_nameId == id; });
    }

    const AZStd::string& MotionData::GetJointName(size_t jointDataIndex) const
    {
        return MCore::GetStringIdPool().GetName(m_staticJointData[jointDataIndex].m_nameId);
    }

    const AZStd::string& MotionData::GetMorphName(size_t morphDataIndex) const
    {
        return MCore::GetStringIdPool().GetName(m_staticMorphData[morphDataIndex].m_nameId);
    }

    const AZStd::string& MotionData::GetFloatName(size_t floatDataIndex) const
    {
        return MCore::GetStringIdPool().GetName(m_staticFloatData[floatDataIndex].m_nameId);
    }

    AZ::Vector3 MotionData::GetJointStaticPosition(size_t jointDataIndex) const
    {
        return m_staticJointData[jointDataIndex].m_staticTransform.m_position;
    }

    AZ::Quaternion MotionData::GetJointStaticRotation(size_t jointDataIndex) const
    {
        return m_staticJointData[jointDataIndex].m_staticTransform.m_rotation;
    }

#ifndef EMFX_SCALE_DISABLED
    AZ::Vector3 MotionData::GetJointStaticScale(size_t jointDataIndex) const
    {
        return m_staticJointData[jointDataIndex].m_staticTransform.m_scale;
    }
#endif

    Transform MotionData::GetJointStaticTransform(size_t jointDataIndex) const
    {
        return m_staticJointData[jointDataIndex].m_staticTransform;
    }

    AZ::Vector3 MotionData::GetJointBindPosePosition(size_t jointDataIndex) const
    {
        return m_staticJointData[jointDataIndex].m_bindTransform.m_position;
    }

    AZ::Quaternion MotionData::GetJointBindPoseRotation(size_t jointDataIndex) const
    {
        return m_staticJointData[jointDataIndex].m_bindTransform.m_rotation;
    }

#ifndef EMFX_SCALE_DISABLED
    AZ::Vector3 MotionData::GetJointBindPoseScale(size_t jointDataIndex) const
    {
        return m_staticJointData[jointDataIndex].m_bindTransform.m_scale;
    }
#endif

    Transform MotionData::GetJointBindPoseTransform(size_t jointDataIndex) const
    {
        return m_staticJointData[jointDataIndex].m_bindTransform;
    }

    float MotionData::GetMorphStaticValue(size_t morphDataIndex) const
    {
        return m_staticMorphData[morphDataIndex].m_staticValue;
    }

    float MotionData::GetFloatStaticValue(size_t floatDataIndex) const
    {
        return m_staticFloatData[floatDataIndex].m_staticValue;
    }

    AZ::u32 MotionData::GetJointNameId(size_t jointDataIndex) const
    {
        return m_staticJointData[jointDataIndex].m_nameId;
    }

    AZ::u32 MotionData::GetMorphNameId(size_t morphDataIndex) const
    {
        return m_staticMorphData[morphDataIndex].m_nameId;
    }

    AZ::u32 MotionData::GetFloatNameId(size_t floatDataIndex) const
    {
        return m_staticFloatData[floatDataIndex].m_nameId;
    }

    void MotionData::SetJointName(size_t jointDataIndex, const AZStd::string& name)
    {
        m_staticJointData[jointDataIndex].m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
    }

    void MotionData::SetMorphName(size_t morphDataIndex, const AZStd::string& name)
    {
        m_staticMorphData[morphDataIndex].m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
    }

    void MotionData::SetFloatName(size_t floatDataIndex, const AZStd::string& name)
    {
        m_staticFloatData[floatDataIndex].m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
    }

    void MotionData::SetJointNameId(size_t jointDataIndex, AZ::u32 id)
    {
        m_staticJointData[jointDataIndex].m_nameId = id;
    }

    void MotionData::SetMorphNameId(size_t morphDataIndex, AZ::u32 id)
    {
        m_staticMorphData[morphDataIndex].m_nameId = id;
    }

    void MotionData::SetFloatNameId(size_t floatDataIndex, AZ::u32 id)
    {
        m_staticFloatData[floatDataIndex].m_nameId = id;
    }

    void MotionData::SetJointStaticTransform(size_t jointDataIndex, const Transform& transform)
    {
        m_staticJointData[jointDataIndex].m_staticTransform = transform;
    }

    void MotionData::SetJointStaticPosition(size_t jointDataIndex, const AZ::Vector3& position)
    {
        m_staticJointData[jointDataIndex].m_staticTransform.m_position = position;
    }

    void MotionData::SetJointStaticRotation(size_t jointDataIndex, const AZ::Quaternion& rotation)
    {
        m_staticJointData[jointDataIndex].m_staticTransform.m_rotation = rotation;
    }

#ifndef EMFX_SCALE_DISABLED
    void MotionData::SetJointStaticScale(size_t jointDataIndex, const AZ::Vector3& scale)
    {
        m_staticJointData[jointDataIndex].m_staticTransform.m_scale = scale;
    }
#endif

    void MotionData::SetJointBindPoseTransform(size_t jointDataIndex, const Transform& transform)
    {
        m_staticJointData[jointDataIndex].m_bindTransform = transform;
    }

    void MotionData::SetJointBindPosePosition(size_t jointDataIndex, const AZ::Vector3& position)
    {
        m_staticJointData[jointDataIndex].m_bindTransform.m_position = position;
    }

    void MotionData::SetJointBindPoseRotation(size_t jointDataIndex, const AZ::Quaternion& rotation)
    {
        m_staticJointData[jointDataIndex].m_bindTransform.m_rotation = rotation;
    }

#ifndef EMFX_SCALE_DISABLED
    void MotionData::SetJointBindPoseScale(size_t jointDataIndex, const AZ::Vector3& scale)
    {
        m_staticJointData[jointDataIndex].m_bindTransform.m_scale = scale;
    }
#endif

    void MotionData::SetMorphStaticValue(size_t morphDataIndex, float poseValue)
    {
        m_staticMorphData[morphDataIndex].m_staticValue = poseValue;
    }

    void MotionData::SetFloatStaticValue(size_t floatDataIndex, float poseValue)
    {
        m_staticFloatData[floatDataIndex].m_staticValue = poseValue;
    }

    size_t MotionData::AddJoint(const AZStd::string& name, const Transform& poseTransform, const Transform& bindPoseTransform)
    {
        StaticJointData joint;
        joint.m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
        joint.m_staticTransform = poseTransform;
        joint.m_bindTransform = bindPoseTransform;
        m_staticJointData.emplace_back(joint);
        const size_t newIndex = m_staticJointData.size() - 1;
        AddJointSampleData(newIndex);
        return newIndex;
    }

    size_t MotionData::AddMorph(const AZStd::string& name, float poseValue)
    {
        StaticFloatData morph;
        morph.m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
        morph.m_staticValue = poseValue;
        m_staticMorphData.emplace_back(morph);
        const size_t newIndex = m_staticMorphData.size() - 1;
        AddMorphSampleData(newIndex);
        return newIndex;
    }

    size_t MotionData::AddFloat(const AZStd::string& name, float poseValue)
    {
        StaticFloatData f;
        f.m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
        f.m_staticValue = poseValue;
        m_staticFloatData.emplace_back(f);
        const size_t newIndex = m_staticFloatData.size() - 1;
        AddFloatSampleData(newIndex);
        return newIndex;
    }

    void MotionData::Resize(size_t numJoints, size_t numMorphs, size_t numFloats)
    {
        m_staticJointData.resize(numJoints);
        m_staticMorphData.resize(numMorphs);
        m_staticFloatData.resize(numFloats);
        ResizeSampleData(numJoints, numMorphs, numFloats);
    }

    void MotionData::Clear()
    {
        ClearBaseData();
        ClearAllData();
    }

    void MotionData::SetDuration(float duration)
    {
        m_duration = duration;
    }

    void MotionData::RemoveJoint(size_t jointDataIndex)
    {
        m_staticJointData.erase(m_staticJointData.begin() + jointDataIndex);
        RemoveJointSampleData(jointDataIndex);
    }

    void MotionData::RemoveMorph(size_t morphDataIndex)
    {
        m_staticMorphData.erase(m_staticMorphData.begin() + morphDataIndex);
        RemoveMorphSampleData(morphDataIndex);
    }

    void MotionData::RemoveFloat(size_t floatDataIndex)
    {
        m_staticFloatData.erase(m_staticFloatData.begin() + floatDataIndex);
        RemoveFloatSampleData(floatDataIndex);
    }

    void MotionData::ClearBaseData()
    {
        m_staticJointData.clear();
        m_staticJointData.shrink_to_fit();
        m_staticMorphData.clear();
        m_staticMorphData.shrink_to_fit();
        m_staticFloatData.clear();
        m_staticFloatData.shrink_to_fit();
        m_duration = 0.0f;
        m_additive = false;
        m_sampleRate = 30.0f;
    }

    void MotionData::BasicRetarget(const ActorInstance* actorInstance, const MotionLinkData* motionLinkData, size_t jointIndex, Transform& inOutTransform) const
    {
        AZ_Assert(motionLinkData, "Expecting valid motionLinkData pointer.");

        const Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();
        const AZStd::vector<size_t>& jointLinks = motionLinkData->GetJointDataLinks();

        // Special case handling on translation of root nodes.
        // Scale the translation amount based on the height difference between the bind pose height of the
        // retarget root node and the bind pose of that node stored in the motion.
        // All other nodes get their translation data displaced based on the position difference between the
        // parent relative space positions in the actor instance's bind pose and the motion bind pose.
        const Actor* actor = actorInstance->GetActor();
        const size_t retargetRootIndex = actor->GetRetargetRootNodeIndex();
        const Node* joint = actor->GetSkeleton()->GetNode(jointIndex);
        bool needsDisplacement = true;
        if ((retargetRootIndex == jointIndex || joint->GetIsRootNode()) && retargetRootIndex != InvalidIndex)
        {
            const size_t retargetRootDataIndex = jointLinks[actor->GetRetargetRootNodeIndex()];
            if (retargetRootDataIndex != InvalidIndex)
            {
                const float subMotionHeight = m_staticJointData[retargetRootDataIndex].m_bindTransform.m_position.GetZ();
                if (AZ::GetAbs(subMotionHeight) >= AZ::Constants::FloatEpsilon)
                {
                    const float heightFactor = bindPose->GetLocalSpaceTransform(retargetRootIndex).m_position.GetZ() / subMotionHeight;
                    inOutTransform.m_position *= heightFactor;
                    needsDisplacement = false;
                }
            }
        }

        const size_t jointDataIndex = jointLinks[jointIndex];
        if (jointDataIndex != InvalidIndex)
        {
            const Transform& bindPoseTransform = bindPose->GetLocalSpaceTransform(jointIndex);
            const Transform& motionBindPose = m_staticJointData[jointDataIndex].m_bindTransform;
            if (needsDisplacement)
            {
                const AZ::Vector3 displacement = bindPoseTransform.m_position - motionBindPose.m_position;
                inOutTransform.m_position += displacement;
            }

            EMFX_SCALECODE
            (
                const AZ::Vector3 scaleOffset = bindPoseTransform.m_scale - motionBindPose.m_scale;
                inOutTransform.m_scale += scaleOffset;
            )
        }
    }

    // Based on a given time value, find the two keyframes to interpolate between, and calculate the t value, which is the interpolation weight between 0 and 1.
    void MotionData::CalculateInterpolationIndicesUniform(float sampleTime, float sampleSpacing, float duration, size_t numSamples, size_t& indexA, size_t& indexB, float& t)
    {
        if (sampleTime < 0.0f)
        {
            indexA = 0;
            indexB = 0;
            t = 0.0f;
            return;
        }

        if (sampleTime >= duration)
        {
            indexA = numSamples - 1;
            indexB = indexA;
            t = 0.0f;
            return;
        }

        indexA = static_cast<size_t>(floor(sampleTime / sampleSpacing));
        indexB = indexA + 1;

        if (indexB > numSamples - 1)
        {
            indexB = indexA;
            t = 0.0f;
            return;
        }

        t = (sampleTime - (indexA * sampleSpacing)) / sampleSpacing;
    }

    void MotionData::CalculateInterpolationIndicesNonUniform(const AZStd::vector<float>& timeValues, float sampleTime, size_t& indexA, size_t& indexB, float& t)
    {
        const auto upper = AZStd::upper_bound(timeValues.begin(), timeValues.end(), sampleTime);
        const size_t index = upper - timeValues.begin();

        if (index == 0)
        {
            indexA = 0;
            indexB = 0;
            t = 0.0f;
            return;
        }

        if (index >= timeValues.size())
        {
            indexA = timeValues.size() - 1;
            indexB = indexA;
            t = 0.0f;
            return;
        }

        indexA = index - 1;
        indexB = index;
        t = (sampleTime - timeValues[indexA]) / (timeValues[indexB] - timeValues[indexA]);
    }

    void MotionData::Scale(float scaleFactor)
    {
        // Scale the static data
        for (StaticJointData& jointData : m_staticJointData)
        {
            jointData.m_staticTransform.m_position *= scaleFactor;
            jointData.m_bindTransform.m_position *= scaleFactor;
        }

        // Scale all data stored by the inherited class.
        ScaleData(scaleFactor);
    }

    bool MotionData::IsAdditive() const
    {
        return m_additive;
    }

    void MotionData::SetAdditive(bool isAdditive)
    {
        m_additive = isAdditive;
    }

    float MotionData::GetSampleRate() const
    {
        return m_sampleRate;
    }

    void MotionData::SetSampleRate(float sampleRate)
    {
        AZ_Assert(sampleRate > 0.0f, "Expecting a sample rate larger than zero.");
        m_sampleRate = sampleRate;
    }

    void MotionData::CopyBaseMotionData(const MotionData* motionData)
    {
        AZ_Assert(motionData, "Expected a motionData pointer that is not a nullptr.");
        m_staticJointData = motionData->m_staticJointData;
        m_staticMorphData = motionData->m_staticMorphData;
        m_staticFloatData = motionData->m_staticFloatData;
        m_duration = motionData->m_duration;
        m_sampleRate = motionData->m_sampleRate;
        m_additive = motionData->m_additive;

        ResizeSampleData(GetNumJoints(), GetNumMorphs(), GetNumFloats());
    }

    bool MotionData::SampleMorph(float sampleTime, AZ::u32 id, float& resultValue) const
    {
        const AZ::Outcome<size_t> morphDataIndex = FindMorphIndexByNameId(id);
        if (morphDataIndex.IsSuccess())
        {
            resultValue = SampleMorph(sampleTime, morphDataIndex.GetValue());
            return true;
        }

        resultValue = 0.0f;
        return false;
    }

    bool MotionData::SampleFloat(float sampleTime, AZ::u32 id, float& resultValue) const
    {
        const AZ::Outcome<size_t> floatDataIndex = FindFloatIndexByNameId(id);
        if (floatDataIndex.IsSuccess())
        {
            resultValue = SampleFloat(sampleTime, floatDataIndex.GetValue());
            return true;
        }

        resultValue = 0.0f;
        return false;
    }

    bool MotionData::SampleJointPosition(float sampleTime, AZ::u32 jointNameId, AZ::Vector3& resultValue) const
    {
        const AZ::Outcome<size_t> dataIndex = FindJointIndexByNameId(jointNameId);
        if (dataIndex.IsSuccess())
        {
            resultValue = SampleJointPosition(sampleTime, dataIndex.GetValue());
            return true;
        }

        resultValue = AZ::Vector3::CreateZero();
        return false;
    }

    bool MotionData::SampleJointRotation(float sampleTime, AZ::u32 jointNameId, AZ::Quaternion& resultValue) const
    {
        const AZ::Outcome<size_t> dataIndex = FindJointIndexByNameId(jointNameId);
        if (dataIndex.IsSuccess())
        {
            resultValue = SampleJointRotation(sampleTime, dataIndex.GetValue());
            return true;
        }

        resultValue = AZ::Quaternion::CreateIdentity();
        return false;
    }

    bool MotionData::SampleJointScale(float sampleTime, AZ::u32 jointNameId, AZ::Vector3& resultValue) const
    {
        const AZ::Outcome<size_t> dataIndex = FindJointIndexByNameId(jointNameId);
        if (dataIndex.IsSuccess())
        {
            resultValue = SampleJointScale(sampleTime, dataIndex.GetValue());
            return true;
        }

        resultValue = AZ::Vector3::CreateOne();
        return false;
    }

    AZStd::string MotionData::ReadStringFromStream(MCore::Stream* stream, MCore::Endian::EEndianType sourceEndianType)
    {
        AZStd::string result;
        AZ::u32 numCharacters = 0;
        if (stream->Read(&numCharacters, sizeof(AZ::u32)) > 0)
        {
            MCore::Endian::ConvertUnsignedInt32(&numCharacters, sourceEndianType);
            result.resize(numCharacters);
            stream->Read(result.data(), numCharacters);
        }

        return result;
    }

    void MotionData::ExtractRootMotion(size_t sampleJointDataIndex, size_t rootJointDataIndex, const RootMotionExtractionData& data)
    {
        if (m_rootMotionExtracted)
        {
            AZ_Assert(false, "Root motion extraction already processed on this motion. Abort because running the extraction algorithm again could cause unexpected behavior.");
            return;
        }

        if (sampleJointDataIndex == rootJointDataIndex)
        {
            return;
        }

        if (m_staticJointData.size() > sampleJointDataIndex && m_staticJointData.size() > rootJointDataIndex)
        {
            m_staticJointData[rootJointDataIndex].m_staticTransform.m_position = m_staticJointData[sampleJointDataIndex].m_staticTransform.m_position;
            if (data.m_extractRotation)
            {
                m_staticJointData[rootJointDataIndex].m_staticTransform.m_rotation = m_staticJointData[sampleJointDataIndex].m_staticTransform.m_rotation;
            }
        }

        if (m_staticMorphData.size() > sampleJointDataIndex && m_staticMorphData.size() > rootJointDataIndex)
        {
            m_staticMorphData[rootJointDataIndex].m_staticValue = m_staticMorphData[sampleJointDataIndex].m_staticValue;
        }

        if (m_staticFloatData.size() > sampleJointDataIndex && m_staticFloatData.size() > rootJointDataIndex)
        {
            m_staticFloatData[rootJointDataIndex].m_staticValue = m_staticFloatData[sampleJointDataIndex].m_staticValue;
        }

        m_rootMotionExtracted = true;
    }

} // namespace EMotionFX
