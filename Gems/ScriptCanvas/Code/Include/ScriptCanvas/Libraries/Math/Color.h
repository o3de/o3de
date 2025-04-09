/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Data/NumericData.h>


namespace ScriptCanvas
{
    namespace ColorFunctions
    {
        using namespace Data;

        NumberType Dot(ColorType a, ColorType b);
        NumberType Dot3(ColorType a, ColorType b);
        ColorType FromValues(NumberType r, NumberType g, NumberType b, NumberType a);
        ColorType FromVector3(Vector3Type source);
        ColorType FromVector3AndNumber(Vector3Type RGB, NumberType A);
        ColorType GammaToLinear(ColorType source);
        BooleanType IsClose(ColorType a, ColorType b, NumberType tolerance);
        BooleanType IsZero(ColorType source, NumberType tolerance);
        ColorType LinearToGamma(ColorType source);
        ColorType MultiplyByNumber(ColorType source, NumberType multiplier);
        ColorType One();
    } // namespace ColorFunctions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/Color.generated.h>
