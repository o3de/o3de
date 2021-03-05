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

// include the required headers
#include "StandardHeaders.h"
#include <AzCore/Math/Vector2.h>
#include <MCore/Source/AzCoreConversions.h>


namespace MCore
{
    /**
     * The class used to check differences between given objects. The differences are all represented as floats.
     * This class can be useful in some specific algorithms used for optimizing data sets.
     * Partial template specialization is used to calculate differences between custom object types.
     * There is already support provided for a number of types, such as floats, ints and vectors.
     */
    template <class T>
    class Compare
    {
    public:
        /**
         * Check if two given values are close to eachother or not.
         * @param a The first value.
         * @param b The second value.
         * @param threshold The maximum error threshold value.
         * @note Partial template specialization MUST be used to define behavior for the given type.
         */
        static MCORE_INLINE bool CheckIfIsClose(const T& a, const T& b, float threshold);
    };


    // include the inline code
#include "Compare.inl"
}
