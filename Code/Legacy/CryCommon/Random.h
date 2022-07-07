/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "BaseTypes.h"
#include "LCGRandom.h"

namespace CryRandom_Internal
{
    // Private access point to the global random number generator.
    extern CRndGen g_random_generator;
}


inline uint32 cry_random_uint32()
{
    return CryRandom_Internal::g_random_generator.GenerateUint32();
}

// Ranged function returns random value within the *inclusive* range
// between minValue and maxValue.
// Any orderings work correctly: minValue <= maxValue and
// minValue >= minValue.
template <class T>
inline T cry_random(const T minValue, const T maxValue)
{
    return CryRandom_Internal::g_random_generator.GetRandom(minValue, maxValue);
}
