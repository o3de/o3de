/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Vector4.h"

#include <Include/ScriptCanvas/Libraries/Math/VectorN.generated.h>

namespace ScriptCanvas
{
    namespace VectorNFunctions
    {
        Data::VectorNType Zero(Data::NumberType size)
        {
            return Data::VectorNType::CreateZero(aznumeric_cast<AZStd::size_t>(size));
        }

        Data::VectorNType One(Data::NumberType size)
        {
            return Data::VectorNType::CreateOne(aznumeric_cast<AZStd::size_t>(size));
        }

        Data::VectorNType Random(Data::NumberType size)
        {
            return Data::VectorNType::CreateRandom(aznumeric_cast<AZStd::size_t>(size));
        }

        Data::NumberType GetDimensionality(const Data::VectorNType& source)
        {
            return static_cast<Data::NumberType>(source.GetDimensionality());
        }

        Data::NumberType GetElement(const Data::VectorNType& source, const Data::NumberType index)
        {
            return source.GetElement(AZ::GetClamp<AZStd::size_t>(aznumeric_cast<AZStd::size_t>(index), 0, source.GetDimensionality() - 1));
        }

        Data::BooleanType IsClose(const Data::VectorNType& a, const Data::VectorNType& b, Data::NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }

        Data::BooleanType IsZero(const Data::VectorNType& source, Data::NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }

        Data::VectorNType GetMin(const Data::VectorNType& a, const Data::VectorNType& b)
        {
            if (a.GetDimensionality() == b.GetDimensionality())
            {
                return a.GetMin(b);
            }
            return Data::VectorNType(0);
        }

        Data::VectorNType GetMax(const Data::VectorNType& a, const Data::VectorNType& b)
        {
            if (a.GetDimensionality() == b.GetDimensionality())
            {
                return a.GetMax(b);
            }
            return Data::VectorNType(0);
        }

        Data::VectorNType GetClamp(const Data::VectorNType& source, const Data::VectorNType& min, const Data::VectorNType& max)
        {
            if (source.GetDimensionality() == min.GetDimensionality() && source.GetDimensionality() == max.GetDimensionality())
            {
                return source.GetClamp(min, max);
            }
            return Data::VectorNType(0);
        }

        Data::NumberType LengthSquared(const Data::VectorNType& source)
        {
            return source.Dot(source);
        }

        Data::NumberType Length(const Data::VectorNType& source)
        {
            return source.L2Norm();
        }

        Data::VectorNType& Normalize(Data::VectorNType& source)
        {
            source.Normalize();
            return source;
        }

        Data::VectorNType& Absolute(Data::VectorNType& source)
        {
            source.Absolute();
            return source;
        }

        Data::VectorNType& Square(Data::VectorNType& source)
        {
            source.Square();
            return source;
        }

        Data::NumberType Dot(const Data::VectorNType& lhs, const Data::VectorNType& rhs)
        {
            return lhs.Dot(rhs);
        }
    } // namespace VectorNFunctions
} // namespace ScriptCanvas
