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
    namespace TransformFunctions
    {
        using namespace Data;

        TransformType FromMatrix3x3(Matrix3x3Type source);

        TransformType FromMatrix3x3AndTranslation(Matrix3x3Type matrix, Vector3Type translation);

        TransformType FromRotation(QuaternionType rotation);

        TransformType FromScale(NumberType scale);

        TransformType FromTranslation(Vector3Type translation);

        TransformType FromRotationAndTranslation(QuaternionType rotation, Vector3Type translation);

        TransformType FromRotationScaleAndTranslation(QuaternionType rotation, NumberType scale, Vector3Type translation);

        Vector3Type GetRight(const TransformType& source, NumberType scale);

        Vector3Type GetForward(const TransformType& source, NumberType scale);

        Vector3Type GetUp(const TransformType& source, NumberType scale);

        Vector3Type GetTranslation(const TransformType& source);

        BooleanType IsClose(const TransformType& a, const TransformType& b, NumberType tolerance);

        BooleanType IsFinite(const TransformType& source);

        BooleanType IsOrthogonal(const TransformType& source, NumberType tolerance);

        TransformType MultiplyByUniformScale(TransformType source, NumberType scale);

        Vector3Type MultiplyByVector3(const TransformType& source, const Vector3Type multiplier);

        Vector4Type MultiplyByVector4(const TransformType& source, const Vector4Type multiplier);

        TransformType Orthogonalize(const TransformType& source);

        TransformType RotationXDegrees(NumberType degrees);

        TransformType RotationYDegrees(NumberType degrees);

        TransformType RotationZDegrees(NumberType degrees);

        NumberType ToScale(const TransformType& source);
    } // namespace TransformFunctions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/Transform.generated.h>
