#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class SCENE_DATA_CLASS AnimationData
                : public SceneAPI::DataTypes::IAnimationData
            {
            public:
                AZ_RTTI(AnimationData, "{D350732E-4727-41C8-95E0-FBAF5F2AC074}", SceneAPI::DataTypes::IAnimationData);

                static void Reflect(ReflectContext* context);

                SCENE_DATA_API AnimationData();
                SCENE_DATA_API ~AnimationData() override = default;
                SCENE_DATA_API virtual void AddKeyFrame(const SceneAPI::DataTypes::MatrixType& keyFrameTransform);
                SCENE_DATA_API virtual void ReserveKeyFrames(size_t count);
                SCENE_DATA_API virtual void SetTimeStepBetweenFrames(double timeStep);

                SCENE_DATA_API size_t GetKeyFrameCount() const override;
                SCENE_DATA_API const SceneAPI::DataTypes::MatrixType& GetKeyFrame(size_t index) const override;

                SCENE_DATA_API double GetTimeStepBetweenFrames() const override;

                SCENE_DATA_API void GetDebugOutput(SceneAPI::Utilities::DebugOutput& output) const override;
            protected:
                AZStd::vector<SceneAPI::DataTypes::MatrixType>    m_keyFrames;
                double                                            m_timeStepBetweenFrames;
            };

            class BlendShapeAnimationData
                : public SceneAPI::DataTypes::IBlendShapeAnimationData
            {
            public:
                AZ_RTTI(BlendShapeAnimationData, "{02766CCF-BDA7-46B6-9BB1-58A90C1AD6AA}", SceneAPI::DataTypes::IBlendShapeAnimationData);

                static void Reflect(ReflectContext* context);

                SCENE_DATA_API BlendShapeAnimationData();
                SCENE_DATA_API ~BlendShapeAnimationData() override = default;
                SCENE_DATA_API void CloneAttributesFrom(const IGraphObject* sourceObject) override;
                SCENE_DATA_API virtual void SetBlendShapeName(const char* name);
                SCENE_DATA_API virtual void AddKeyFrame(double keyFrameValue);
                SCENE_DATA_API virtual void ReserveKeyFrames(size_t count);
                SCENE_DATA_API virtual void SetTimeStepBetweenFrames(double timeStep);

                SCENE_DATA_API const char* GetBlendShapeName() const override;
                SCENE_DATA_API size_t GetKeyFrameCount() const override;
                SCENE_DATA_API double GetKeyFrame(size_t index) const override;

                SCENE_DATA_API double GetTimeStepBetweenFrames() const override;

                SCENE_DATA_API void GetDebugOutput(SceneAPI::Utilities::DebugOutput& output) const override;
            protected:
                AZStd::string            m_blendShapeName;
                AZStd::vector<double>    m_keyFrames;
                double                   m_timeStepBetweenFrames;
            };
        }
    }
}
