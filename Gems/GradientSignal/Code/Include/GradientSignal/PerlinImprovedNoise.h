/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/span.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace GradientSignal
{
    /**
    * Implementation of the Perlin improved noise algorithm
    * Perlin noise can be used for any sort of wave-like deterministic patterns
    */
    class PerlinImprovedNoise final
    {
    public:
        AZ_CLASS_ALLOCATOR(PerlinImprovedNoise, AZ::SystemAllocator);

        /**
        * Prepares the permutation table with a given random seed
        */          
        PerlinImprovedNoise(int seed);

        /**
         * Initializes the permutation table from a preset table
         */
        PerlinImprovedNoise(const AZStd::span<int, 512>& permutationTable);

        /**
        * Creates a Perlin 'natural' noise factor values based on a position with smoothing parameters
        */
        float GenerateOctaveNoise(float x, float y, float z, int octaves, float persistence, float initialFrequency = 1.0f);

        /**
        * Creates a Perlin noise factor value based on a position
        */
        float GenerateNoise(float x, float y, float z);

    private:
        void PrepareTable(int seed);

        AZStd::array<int, 512> m_permutationTable;
    };

} // namespace GradientSignal
