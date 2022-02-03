/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>

#include <EMotionFX/Source/EMotionFXConfig.h>

#include <Feature.h>
#include <FeatureSchema.h>
#include <FrameDatabase.h>
#include <KdTree.h>

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
    class EMFX_API MotionMatchingData
    {
    public:
        AZ_RTTI(MotionMatchingData, "{7BC3DFF5-8864-4518-B6F0-0553ADFAB5C1}")
        AZ_CLASS_ALLOCATOR_DECL

        MotionMatchingData(const FeatureSchema& featureSchema);
        virtual ~MotionMatchingData();

        struct EMFX_API InitSettings
        {
            ActorInstance* m_actorInstance = nullptr;
            AZStd::vector<Motion*> m_motionList;
            FrameDatabase::FrameImportSettings m_frameImportSettings;
            size_t m_maxKdTreeDepth = 20;
            size_t m_minFramesPerKdTreeNode = 1000;
            bool m_importMirrored = false;
        };
        bool Init(const InitSettings& settings);

        void Clear();

        const FrameDatabase& GetFrameDatabase() const { return m_frameDatabase; }
        FrameDatabase& GetFrameDatabase() { return m_frameDatabase; }
        const FeatureSchema& GetFeatureSchema() const { return m_featureSchema; }
        const FeatureMatrix& GetFeatureMatrix() const { return m_featureMatrix; }
        const KdTree& GetKdTree() const { return *m_kdTree.get(); }
        const AZStd::vector<Feature*>& GetFeaturesInKdTree() const { return m_featuresInKdTree; }

    protected:
        bool ExtractFeatures(ActorInstance* actorInstance, FrameDatabase* frameDatabase, size_t maxKdTreeDepth=20, size_t minFramesPerKdTreeNode=2000);

        FrameDatabase m_frameDatabase; //< The animation database with all the keyframes and joint transform data.

        const FeatureSchema& m_featureSchema;
        FeatureMatrix m_featureMatrix;

        AZStd::unique_ptr<KdTree> m_kdTree; //< The acceleration structure to speed up the search for lowest cost frames.
        AZStd::vector<Feature*> m_featuresInKdTree;
    };
} // namespace EMotionFX::MotionMatching
