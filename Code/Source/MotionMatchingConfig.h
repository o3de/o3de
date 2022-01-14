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

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Pose.h>

#include <FeatureDatabase.h>
#include <FrameDatabase.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class ActorInstance;
}

namespace EMotionFX::MotionMatching
{
    class MotionMatchingInstance;

    class EMFX_API MotionMatchingConfig
    {
    public:
        AZ_RTTI(MotionMatchingConfig, "{7BC3DFF5-8864-4518-B6F0-0553ADFAB5C1}")
        AZ_CLASS_ALLOCATOR_DECL

        MotionMatchingConfig() = default;
        virtual ~MotionMatchingConfig() = default;

        struct EMFX_API InitSettings
        {
            ActorInstance* m_actorInstance = nullptr;
            AZStd::vector<Motion*> m_motionList;
            FrameDatabase::FrameImportSettings m_frameImportSettings;
            size_t m_maxKdTreeDepth = 20;
            size_t m_minFramesPerKdTreeNode = 1000;
            bool m_importMirrored = false;
        };
        bool RegisterFeatures(const InitSettings& settings);
        bool Init(const InitSettings& settings);
        void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay, MotionMatchingInstance* instance);

        size_t FindLowestCostFrameIndex(MotionMatchingInstance* instance, const Feature::FrameCostContext& context, AZStd::vector<float>& tempCosts, AZStd::vector<float>& minCosts);

        static void Reflect(AZ::ReflectContext* context);

        const FrameDatabase& GetFrameDatabase() const { return m_frameDatabase; }
        FrameDatabase& GetFrameDatabase() { return m_frameDatabase; }

        const FeatureDatabase& GetFeatureDatabase() const { return m_featureDatabase; }
        FeatureDatabase& GetFeatureDatabase() { return m_featureDatabase; }

    protected:
        FrameDatabase m_frameDatabase; /**< The frames and their data. */
        FeatureDatabase m_featureDatabase;
    };
} // namespace EMotionFX::MotionMatching
