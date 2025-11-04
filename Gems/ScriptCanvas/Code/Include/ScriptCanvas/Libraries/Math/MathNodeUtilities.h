/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once



#include <AzCore/Math/Vector4.h>
#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace MathNodeUtilities
    {
        Data::NumberType GetRandom(Data::NumberType lhs, Data::NumberType rhs);

        AZ::s64 GetRandom(AZ::s64 lhs, AZ::s64 rhs);

        template<typename NumberType>
        AZ_INLINE NumberType GetRandomIntegral(NumberType lhs, NumberType rhs)
        {
            return aznumeric_caster(GetRandom(aznumeric_cast<AZ::s64>(lhs), aznumeric_cast<AZ::s64>(rhs)));
        }

        template<>
        AZ_INLINE Data::NumberType GetRandomIntegral<Data::NumberType>(Data::NumberType lhs, Data::NumberType rhs)
        {
            return GetRandom(lhs, rhs);
        }

        template<typename NumberType>
        AZ_INLINE NumberType GetRandomReal(NumberType lhs, NumberType rhs)
        {
            return aznumeric_caster(GetRandom(aznumeric_cast<Data::NumberType>(lhs), aznumeric_cast<Data::NumberType>(rhs)));
        }

        template<>
        AZ_INLINE Data::NumberType GetRandomReal<Data::NumberType>(Data::NumberType lhs, Data::NumberType rhs)
        {
            return GetRandom(lhs, rhs);
        }

        void RandomEngineInit();
        void RandomEngineReset();
    }
}

