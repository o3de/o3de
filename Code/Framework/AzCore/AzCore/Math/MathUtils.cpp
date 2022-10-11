/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Random.h>

namespace AZ
{
    Quaternion CreateRandomQuaternion(SimpleLcgRandom& rng)
    {
        // Marsaglia, G., 1972. Choosing a point from the surface of a sphere. The Annals of Mathematical Statistics, 43(2), pp.645-646.
        const float u0 = rng.GetRandomFloat();
        const float u1 = rng.GetRandomFloat();
        const float u2 = rng.GetRandomFloat();
        float x, y, z, w;
        SinCos(Constants::TwoPi * u1, x, y);
        SinCos(Constants::TwoPi * u2, z, w);
        const float m0 = Sqrt(u0);
        const float m1 = Sqrt(1 - u0);
        return Quaternion(m0 * x, m0 * y, m1 * z, m1 * w);
    }
} // namespace AZ
