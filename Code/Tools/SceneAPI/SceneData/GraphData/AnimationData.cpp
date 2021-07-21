/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/GraphData/AnimationData.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            void AnimationData::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AnimationData, SceneAPI::DataTypes::IAnimationData>()
                        ->Version(1);
                }

                BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<SceneAPI::DataTypes::IAnimationData>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("GetKeyFrameCount", &SceneAPI::DataTypes::IAnimationData::GetKeyFrameCount)
                        ->Method("GetKeyFrame", &SceneAPI::DataTypes::IAnimationData::GetKeyFrame)
                        ->Method("GetTimeStepBetweenFrames", &SceneAPI::DataTypes::IAnimationData::GetTimeStepBetweenFrames);

                    behaviorContext->Class<AnimationData>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene");
                }
            }

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


            void BlendShapeAnimationData::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<BlendShapeAnimationData, SceneAPI::DataTypes::IBlendShapeAnimationData>()
                        ->Version(1);
                }

                BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<SceneAPI::DataTypes::IBlendShapeAnimationData>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("GetBlendShapeName", &SceneAPI::DataTypes::IBlendShapeAnimationData::GetBlendShapeName)
                        ->Method("GetKeyFrameCount", &SceneAPI::DataTypes::IBlendShapeAnimationData::GetKeyFrameCount)
                        ->Method("GetKeyFrame", &SceneAPI::DataTypes::IBlendShapeAnimationData::GetKeyFrame)
                        ->Method("GetTimeStepBetweenFrames", &SceneAPI::DataTypes::IBlendShapeAnimationData::GetTimeStepBetweenFrames);

                    behaviorContext->Class<BlendShapeAnimationData>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene");
                }
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
