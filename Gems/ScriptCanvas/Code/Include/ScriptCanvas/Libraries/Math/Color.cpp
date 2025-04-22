/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Color.h"

#include <AzCore/Math/Color.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

#include <ScriptCanvas/AutoGen/ScriptCanvasAutoGenRegistry.h>

#include <Include/ScriptCanvas/Libraries/Math/Color.generated.h>

namespace ScriptCanvas
{
    namespace ColorFunctions
    {
        using namespace Data;

        NumberType Dot(ColorType a, ColorType b)
        {
            return a.Dot(b);
        }

        NumberType Dot3(ColorType a, ColorType b)
        {
            return a.Dot3(b);
        }

        ColorType FromValues(NumberType r, NumberType g, NumberType b, NumberType a)
        {
            return ColorType(
                aznumeric_cast<float>(r),
                aznumeric_cast<float>(g),
                aznumeric_cast<float>(b),
                aznumeric_cast<float>(a));
        }

        ColorType FromVector3(Vector3Type source)
        {
            return ColorType::CreateFromVector3(source);
        }

        ColorType FromVector3AndNumber(Vector3Type RGB, NumberType A)
        {
            return ColorType::CreateFromVector3AndFloat(RGB, aznumeric_cast<float>(A));
        }

        ColorType GammaToLinear(ColorType source)
        {
            return source.GammaToLinear();
        }

        BooleanType IsClose(ColorType a, ColorType b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }

        BooleanType IsZero(ColorType source, NumberType tolerance)
        {
            return source.IsZero(aznumeric_cast<float>(tolerance));
        }

        ColorType LinearToGamma(ColorType source)
        {
            return source.LinearToGamma();
        }

        ColorType MultiplyByNumber(ColorType source, NumberType multiplier)
        {
            return source * aznumeric_cast<float>(multiplier);
        }

        ColorType One()
        {
            return ColorType::CreateOne();
        }
    } // namespace ColorFunctions
} // namespace ScriptCanvas
