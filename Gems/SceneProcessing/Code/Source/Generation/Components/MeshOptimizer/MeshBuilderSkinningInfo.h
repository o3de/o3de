/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/base.h>
#include "MeshBuilderInvalidIndex.h"
#include "Array2D.h"

namespace AZ::MeshBuilder
{
    class MeshBuilderSkinningInfo
    {
    public:

        AZ_CLASS_ALLOCATOR_DECL

        struct Influence
        {
            size_t  mNodeNr = InvalidIndex;
            float   mWeight = 1.0f;

            Influence() = default;
            Influence(size_t nodeNr, float weight)
                : mNodeNr(nodeNr)
                , mWeight(weight)
            {}
        };

        MeshBuilderSkinningInfo(size_t numOrgVertices);

        void AddInfluence(size_t orgVtxNr, size_t nodeNr, float weight)            { AddInfluence(orgVtxNr, {nodeNr, weight}); }
        void AddInfluence(size_t orgVtxNr, const Influence& influence)             { mInfluences.Add(orgVtxNr, influence); }
        void RemoveInfluence(size_t orgVtxNr, size_t influenceNr)                  { mInfluences.Remove(orgVtxNr, influenceNr); }
        const Influence& GetInfluence(size_t orgVtxNr, size_t influenceNr) const   { return mInfluences.GetElement(orgVtxNr, influenceNr); }
        size_t GetNumInfluences(size_t orgVtxNr) const                             { return mInfluences.GetNumElements(orgVtxNr); }
        size_t GetNumOrgVertices() const                                           { return mInfluences.GetNumRows(); }
        void OptimizeMemoryUsage()                                                 { mInfluences.Shrink(); }
        size_t CalcTotalNumInfluences() const                                      { return mInfluences.CalcTotalNumElements(); }

        // optimize the weight data
        void Optimize(AZ::u32 maxNumWeightsPerVertex = 4, float weightThreshold = 0.0001f);

        // optimize weights
        static void OptimizeSkinningInfluences(AZStd::vector<Influence>& influences, float tolerance, size_t maxWeights);

        // sort the influences, starting with the biggest weight
        static void SortInfluences(AZStd::vector<Influence>& influences);

    public:
        MCore::Array2D<Influence> mInfluences;
    };
} // namespace AZ::MeshBuilder
