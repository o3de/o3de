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

#include <SceneAPI/SceneData/GraphData/AnimationData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            AnimationData::AnimationData()
                : m_timeStepBetweenFrames(1.0/30.0) // default value
            {
            }

            void AnimationData::AddKeyFrame(const SceneAPI::DataTypes::MatrixType& keyFrameTransform)
            {
                m_keyFrames.push_back(keyFrameTransform);
            }

            void AnimationData::ReserveKeyFrames(size_t count)
            {
                m_keyFrames.reserve(count);
            }

            void AnimationData::SetTimeStepBetweenFrames(double timeStep)
            {
                m_timeStepBetweenFrames = timeStep;
            }

            size_t AnimationData::GetKeyFrameCount() const
            {
                return m_keyFrames.size();
            }

            const SceneAPI::DataTypes::MatrixType& AnimationData::GetKeyFrame(size_t index) const
            {
                AZ_Assert(index < m_keyFrames.size(), "GetTranslationKeyFrame index %i is out of range for frame size %i", index, m_keyFrames.size());
                return m_keyFrames[index];
            }

            double AnimationData::GetTimeStepBetweenFrames() const
            {
                return m_timeStepBetweenFrames;
            }

            void AnimationData::GetDebugOutput(SceneAPI::Utilities::DebugOutput& output) const
            {
                output.Write("KeyFrames", m_keyFrames);
                output.Write("TimeStepBetweenFrames", m_timeStepBetweenFrames);
            }


            BlendShapeAnimationData::BlendShapeAnimationData()
                : m_timeStepBetweenFrames(1 / 30.0) // default value
            {
            }

            void BlendShapeAnimationData::CloneAttributesFrom(const IGraphObject* sourceObject)
            {
                IBlendShapeAnimationData::CloneAttributesFrom(sourceObject);
                if (const auto* typedSource = azrtti_cast<const BlendShapeAnimationData*>(sourceObject))
                {
                    SetBlendShapeName(typedSource->GetBlendShapeName());
                }
            }

            void BlendShapeAnimationData::SetBlendShapeName(const char* blendShapeName)
            {
                m_blendShapeName = blendShapeName;
            }

            void BlendShapeAnimationData::AddKeyFrame(double keyFrameValue)
            {
                m_keyFrames.push_back(keyFrameValue);
            }

            void BlendShapeAnimationData::ReserveKeyFrames(size_t count)
            {
                m_keyFrames.reserve(count);
            }

            void BlendShapeAnimationData::SetTimeStepBetweenFrames(double timeStep)
            {
                m_timeStepBetweenFrames = timeStep;
            }

            const char* BlendShapeAnimationData::GetBlendShapeName() const
            {
                return m_blendShapeName.c_str();
            }

            size_t BlendShapeAnimationData::GetKeyFrameCount() const
            {
                return m_keyFrames.size();
            }

            double BlendShapeAnimationData::GetKeyFrame(size_t index) const
            {
                AZ_Assert(index < m_keyFrames.size(), "BlendShapeAnimationData::GetKeyFrame index %i is out of range for frame count %i", index, m_keyFrames.size());
                return m_keyFrames[index];
            }

            double BlendShapeAnimationData::GetTimeStepBetweenFrames() const
            {
                return m_timeStepBetweenFrames;
            }

            void BlendShapeAnimationData::GetDebugOutput(SceneAPI::Utilities::DebugOutput& output) const
            {
                output.Write("BlendShapeName", m_blendShapeName);
                output.Write("KeyFrames", m_keyFrames);
                output.Write("TimeStepBetweenFrames", m_timeStepBetweenFrames);
            }
        }
    }
}
