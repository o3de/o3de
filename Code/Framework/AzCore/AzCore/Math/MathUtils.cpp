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
