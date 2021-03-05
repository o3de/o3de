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

// include required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include <MCore/Source/Array2D.h>


namespace EMotionFX
{
    class EMFX_API MeshBuilderSkinningInfo
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        struct Influence
        {
            AZ_CLASS_ALLOCATOR_DECL

            float   mWeight;
            uint32  mNodeNr;

            MCORE_INLINE Influence()
                : mWeight(1.0f)
                , mNodeNr(MCORE_INVALIDINDEX32) {}
            MCORE_INLINE Influence(uint32 nodeNr, float weight)
                : mWeight(weight)
                , mNodeNr(nodeNr) {}
        };

        static MeshBuilderSkinningInfo* Create(uint32 numOrgVertices);

        MCORE_INLINE void AddInfluence(uint32 orgVtxNr, uint32 nodeNr, float weight)            { AddInfluence(orgVtxNr, Influence(nodeNr, weight)); }
        MCORE_INLINE void AddInfluence(uint32 orgVtxNr, const Influence& influence)             { mInfluences.Add(orgVtxNr, influence); }
        MCORE_INLINE void RemoveInfluence(uint32 orgVtxNr, uint32 influenceNr)                  { mInfluences.Remove(orgVtxNr, influenceNr); }
        MCORE_INLINE Influence& GetInfluence(uint32 orgVtxNr, uint32 influenceNr)               { return mInfluences.GetElement(orgVtxNr, influenceNr); }
        MCORE_INLINE uint32 GetNumInfluences(uint32 orgVtxNr) const                             { return mInfluences.GetNumElements(orgVtxNr); }
        MCORE_INLINE uint32 GetNumOrgVertices() const                                           { return mInfluences.GetNumRows(); }
        MCORE_INLINE void OptimizeMemoryUsage()                                                 { mInfluences.Shrink(); }
        MCORE_INLINE uint32 CalcTotalNumInfluences() const                                      { return mInfluences.CalcTotalNumElements(); }

        // optimize the weight data
        void Optimize(uint32 maxNumWeightsPerVertex = 4, float weightThreshold = 0.0001f);

        // optimize weights
        static void OptimizeSkinningInfluences(MCore::Array<Influence>& influences, float tolerance, uint32 maxWeights);

        // sort the influences, starting with the biggest weight
        static void SortInfluences(MCore::Array<Influence>& influences);

    public:
        MCore::Array2D<Influence>   mInfluences;

    private:
        MeshBuilderSkinningInfo(uint32 numOrgVertices);
        ~MeshBuilderSkinningInfo();
    };

    void CopySkinningInfo(MeshBuilderSkinningInfo* from, MeshBuilderSkinningInfo* to);
} // namespace EMotionFX
