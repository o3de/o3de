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
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Feature.h>
#include <FeatureSchema.h>

namespace EMotionFX::MotionMatching
{
    class FrameDatabase;
    class KdTree;

    class EMFX_API FeatureDatabase
    {
    public:
        AZ_RTTI(FrameDatabase, "{4411C941-B4D9-4ABB-993F-5F44FA7345A4}")
        AZ_CLASS_ALLOCATOR_DECL

        FeatureDatabase();
        virtual ~FeatureDatabase();

        void Clear(); // Clear the data, so you can re-initialize it with new data.
        size_t CalcMemoryUsageInBytes() const;

        bool ExtractFeatures(ActorInstance* actorInstance, FrameDatabase* frameDatabase, size_t maxKdTreeDepth=20, size_t minFramesPerKdTreeNode=2000);
        void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay,
            MotionMatchingInstance* instance,
            size_t frameIndex);

        // KD-Tree
        KdTree& GetKdTree() { return *m_kdTree.get(); }
        const KdTree& GetKdTree() const { return *m_kdTree.get(); }
        size_t CalcNumDataDimensionsForKdTree(const FeatureDatabase& featureDatabase) const;
        void AddKdTreeFeature(Feature* feature) { m_featuresInKdTree.push_back(feature); }
        const AZStd::vector<Feature*>& GetFeaturesInKdTree() const { return m_featuresInKdTree; }

        FeatureSchema& GetFeatureSchema() { return m_featureSchema; }
        const FeatureMatrix& GetFeatureMatrix() const { return m_featureMatrix; }
        void SaveAsCsv(const AZStd::string& filename, Skeleton* skeleton);

    private:
        FeatureSchema m_featureSchema;
        FeatureMatrix m_featureMatrix;

        AZStd::unique_ptr<KdTree> m_kdTree; /**< The acceleration structure to speed up the search for lowest cost frames. */
        AZStd::vector<Feature*> m_featuresInKdTree;
    };
} // namespace EMotionFX::MotionMatching
