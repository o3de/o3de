/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/tuple.h>
#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace VectorNFunctions
    {
        Data::VectorNType Zero(Data::NumberType size);

        Data::VectorNType One(Data::NumberType size);

        Data::VectorNType Random(Data::NumberType size);

        Data::NumberType GetDimensionality(const Data::VectorNType& source);

        Data::NumberType GetElement(const Data::VectorNType& source, const Data::NumberType index);

        Data::BooleanType IsClose(const Data::VectorNType& a, const Data::VectorNType& b, Data::NumberType tolerance);

        Data::BooleanType IsZero(const Data::VectorNType& source, Data::NumberType tolerance);

        Data::VectorNType GetMin(const Data::VectorNType& a, const Data::VectorNType& b);

        Data::VectorNType GetMax(const Data::VectorNType& a, const Data::VectorNType& b);

        Data::VectorNType GetClamp(const Data::VectorNType& source, const Data::VectorNType& min, const Data::VectorNType& max);

        Data::NumberType LengthSquared(const Data::VectorNType& source);

        Data::NumberType Length(const Data::VectorNType& source);

        Data::VectorNType& Normalize(Data::VectorNType& source);

        Data::VectorNType& Absolute(Data::VectorNType& source);

        Data::VectorNType& Square(Data::VectorNType& source);

        Data::NumberType Dot(const Data::VectorNType& lhs, const Data::VectorNType& rhs);
    } // namespace VectorNFunctions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/VectorN.generated.h>
