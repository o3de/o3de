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

#include <AzCore/base.h>

namespace AZ
{
    /**
     * Provides a better yet more computationally costly random implementation.
     * To be used in less frequent scenarios, i.e. to get a good seed.
     */
    class BetterPseudoRandom_Windows
    {
    public:
        BetterPseudoRandom_Windows();
        ~BetterPseudoRandom_Windows();

        /// Fills a buffer with random data, if true is returned. Otherwise the random generator fails to generate random numbers.
        bool GetRandom(void* data, size_t dataSize);

        /// Template helper for value types \ref GetRandom
        template<class T>
        bool GetRandom(T& value)    { return GetRandom(&value, sizeof(T)); }

    private:
        AZ::u64 m_generatorHandle;
    };

    using BetterPseudoRandom_Platform = BetterPseudoRandom_Windows;
}
