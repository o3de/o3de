/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Transform.h"

#include <Include/ScriptCanvas/Libraries/Math/Transform.generated.h>

namespace ScriptCanvas
{
    namespace TransformFunctions
    {
        using namespace Data;

        TransformType FromMatrix3x3(Matrix3x3Type source)
        {
            return TransformType::CreateFromMatrix3x3(source);
        }

        TransformType FromMatrix3x3AndTranslation(Matrix3x3Type matrix, Vector3Type translation)
        {
            return TransformType::CreateFromMatrix3x3AndTranslation(matrix, translation);
        }

        TransformType FromRotation(QuaternionType rotation)
        {
            return TransformType::CreateFromQuaternion(rotation);
        }

        TransformType FromScale(NumberType scale)
        {
            return TransformType::CreateUniformScale(static_cast<float>(scale));
        }

        TransformType FromTranslation(Vector3Type translation)
        {
            return TransformType::CreateTranslation(translation);
        }

        TransformType FromRotationAndTranslation(QuaternionType rotation, Vector3Type translation)
        {
            return TransformType::CreateFromQuaternionAndTranslation(rotation, translation);
        }

        TransformType FromRotationScaleAndTranslation(QuaternionType rotation, NumberType scale, Vector3Type translation)
        {
            return TransformType(translation, rotation, aznumeric_cast<float>(scale));
        }

        Vector3Type GetRight(const TransformType& source, NumberType scale)
        {
            Vector3Type vector = source.GetBasisX();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }

        Vector3Type GetForward(const TransformType& source, NumberType scale)
        {
            Vector3Type vector = source.GetBasisY();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }

        Vector3Type GetUp(const TransformType& source, NumberType scale)
        {
            Vector3Type vector = source.GetBasisZ();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }

        Vector3Type GetTranslation(const TransformType& source)
        {
            return source.GetTranslation();
        }

        BooleanType IsClose(const TransformType& a, const TransformType& b, NumberType tolerance)
        {
            return a.IsClose(b, aznumeric_cast<float>(tolerance));
        }

        BooleanType IsFinite(const TransformType& source)
        {
            return source.IsFinite();
        }

        BooleanType IsOrthogonal(const TransformType& source, NumberType tolerance)
        {
            return source.IsOrthogonal(aznumeric_cast<float>(tolerance));
        }

        TransformType MultiplyByUniformScale(TransformType source, NumberType scale)
        {
            source.MultiplyByUniformScale(static_cast<float>(scale));
            return source;
        }

        Vector3Type MultiplyByVector3(const TransformType& source, const Vector3Type multiplier)
        {
            return source.TransformPoint(multiplier);
        }

        Vector4Type MultiplyByVector4(const TransformType& source, const Vector4Type multiplier)
        {
            return source.TransformPoint(multiplier);
        }

        TransformType Orthogonalize(const TransformType& source)
        {
            return source.GetOrthogonalized();
        }

        TransformType RotationXDegrees(NumberType degrees)
        {
            return TransformType::CreateRotationX(AZ::DegToRad(aznumeric_caster(degrees)));
        }

        TransformType RotationYDegrees(NumberType degrees)
        {
            return TransformType::CreateRotationY(AZ::DegToRad(aznumeric_caster(degrees)));
        }

        TransformType RotationZDegrees(NumberType degrees)
        {
            return TransformType::CreateRotationZ(AZ::DegToRad(aznumeric_caster(degrees)));
        }

        NumberType ToScale(const TransformType& source)
        {
            return source.GetUniformScale();
        }
    } // namespace TransformFunctions
} // namespace ScriptCanvas
