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
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXConfig.h>

#include <Feature.h>

namespace EMotionFX
{
    namespace MotionMatching
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

            void RegisterFeature(Feature* frameData);
            const Feature* GetFeature(size_t index) const;
            const AZStd::vector<Feature*>& GetFeatures() const;

            size_t GetNumFeatureTypes() const;
            Feature* FindFeatureByType(const AZ::TypeId& frameDataTypeId) const;

            void Clear(); // Clear the data, so you can re-initialize it with new data.
            size_t CalcMemoryUsageInBytes() const;

            bool ExtractFeatures(ActorInstance* actorInstance, FrameDatabase* frameDatabase, size_t maxKdTreeDepth=20, size_t minFramesPerKdTreeNode=2000);
            void DebugDraw(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance, size_t frameIndex);

            // KD-Tree
            KdTree& GetKdTree() { return *m_kdTree.get(); }
            const KdTree& GetKdTree() const { return *m_kdTree.get(); }
			size_t CalcNumDataDimensionsForKdTree() const;

            const FeatureMatrix& GetFeatureMatrix() const { return m_featureMatrix; }

        private:
            static Feature* CreateFrameDataByType(const AZ::TypeId& typeId); // create from RTTI type

        private:
            AZStd::vector<Feature*> m_features; /**< This is a flat vector of all frame datas (Owner of the features). */
            AZStd::unordered_map<AZ::TypeId, Feature*> m_featuresByType; /**< The per frame additional data. (Weak ownership) */
            FeatureMatrix m_featureMatrix;

            AZStd::unique_ptr<KdTree> m_kdTree; /**< The acceleration structure to speed up the search for lowest cost frames. */
        };
    } // namespace MotionMatching
} // namespace EMotionFX
