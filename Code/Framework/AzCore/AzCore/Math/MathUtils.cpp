/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        float u2 = rng.GetRandomFloat();
        float u3 = rng.GetRandomFloat();
        float x, y, z, w;
        SinCos(Constants::TwoPi * u2, x, y);
        SinCos(Constants::TwoPi * u3, z, w);
        return Quaternion(x, y, z, w);
    }
} // namespace AZ
