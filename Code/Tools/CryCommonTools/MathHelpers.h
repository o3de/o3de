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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_MATHHELPERS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_MATHHELPERS_H
#pragma once


#include <float.h>
#if (_M_IX86_FP > 0)
#include <intrin.h>
#endif

namespace MathHelpers
{
#if (_M_IX86_FP > 0)
    inline int FastRoundFloatTowardZero(float f)
    {
        return _mm_cvtt_ss2si(_mm_set_ss(f));
    }
#else
    inline int FastRoundFloatTowardZero(float f)
    {
        return int(f);
    }
#endif

#if defined(AZ_PLATFORM_WINDOWS)

    inline unsigned int EnableFloatingPointExceptions(unsigned int mask)
    {
        _clearfp();
        unsigned int oldMask;
        _controlfp_s(&oldMask, 0, 0);
        unsigned int newMask;
        _controlfp_s(&newMask, ~mask, _MCW_EM);
        return ~oldMask;
    }

    class AutoFloatingPointExceptions
    {
    public:
        AutoFloatingPointExceptions(const unsigned int mask)
            : m_mask(EnableFloatingPointExceptions(mask))
        {
        }

        ~AutoFloatingPointExceptions()
        {
            EnableFloatingPointExceptions(m_mask);
        }

    private:
        unsigned int m_mask;
    };
    
#endif //AZ_PLATFORM_WINDOWS
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_MATHHELPERS_H


