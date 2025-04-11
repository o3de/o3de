/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/MathScriptHelpers.h>

namespace AZ
{
    namespace Internal
    {
        void QuaternionScriptConstructor(Quaternion* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number = 0;
                    dc.ReadArg(0, number);
                    *thisPtr = Quaternion(number);
                }
                else
                {
                    AZ_Warning("Script", false, "You should provide only 1 number/float to be splatted");
                }
            } break;
            case 4:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
                {
                    float x = 0;
                    float y = 0;
                    float z = 0;
                    float w = 0;
                    dc.ReadArg(0, x);
                    dc.ReadArg(1, y);
                    dc.ReadArg(2, z);
                    dc.ReadArg(3, w);
                    *thisPtr = Quaternion(x, y, z, w);
                }
            } break;
            }
        }

        void QuaternionDefaultConstructor(AZ::Quaternion* thisPtr)
        {
            new (thisPtr) AZ::Quaternion(AZ::Quaternion::CreateIdentity());
        }


        void QuaternionMultiplyGeneric(const Quaternion* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Quaternion result = *thisPtr * scalar;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Quaternion>(0))
                {
                    Quaternion quaternion = Quaternion::CreateZero();
                    dc.ReadArg(0, quaternion);
                    Quaternion result = *thisPtr * quaternion;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3(Vector3::CreateZero());
                    dc.ReadArg(0, vector3);
                    Vector3 result = thisPtr->TransformVector(vector3);
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Quaternion multiply should have only 1 argument - Quaternion or a Float/Number");
                dc.PushResult(Quaternion::CreateIdentity());
            }
        }


        void QuaternionDivideGeneric(const Quaternion* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Quaternion result = *thisPtr / scalar;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Quaternion divide should have only 1 argument - Float/Number");
                dc.PushResult(Quaternion::CreateIdentity());
            }
        }


        void QuaternionSetGeneric(Quaternion* thisPtr, ScriptDataContext& dc)
        {
            bool valueWasSet = false;

            if (dc.GetNumArguments() == 1 && dc.IsNumber(0))
            {
                float setValue = 0;
                dc.ReadArg(0, setValue);
                thisPtr->Set(setValue);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 2 && dc.IsClass<Vector3>(0) && dc.IsNumber(1))
            {
                Vector3 vector3 = Vector3::CreateZero();
                float w = 0;
                dc.ReadArg(0, vector3);
                dc.ReadArg(1, w);
                thisPtr->Set(vector3, w);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 4 && dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                float w = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                dc.ReadArg(3, w);
                thisPtr->Set(x, y, z, w);
                valueWasSet = true;
            }

            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Quaternion.Set only supports Set(number), Set(number, number, number, number), Set(Vector3, number)");
            }
        }

        Quaternion CreateFromValues(float x, float y, float z, float w)
        {
            return Quaternion(x, y, z, w);
        }
    }


    void Quaternion::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Quaternion>()->
                Serializer<FloatBasedContainerSerializer<Quaternion, &Quaternion::CreateFromFloat4, &Quaternion::StoreToFloat4, &GetTransformEpsilon, 4> >();
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Quaternion>()->
                Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)->
                Attribute(AZ::Script::Attributes::Module, "math")->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)->
                Constructor<float>()->
                Constructor<float, float, float, float>()->
                Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
                Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::QuaternionScriptConstructor)->
                Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::QuaternionDefaultConstructor)->
                Property("x", &Quaternion::GetX, &Quaternion::SetX)->
                Property("y", &Quaternion::GetY, &Quaternion::SetY)->
                Property("z", &Quaternion::GetZ, &Quaternion::SetZ)->
                Property("w", &Quaternion::GetW, &Quaternion::SetW)->
                Method<Quaternion(Quaternion::*)() const>("Unary", &Quaternion::operator-)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Unary)->
                Method("ToString", &QuaternionToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
                Method<Quaternion(Quaternion::*)(float) const>("MultiplyFloat", &Quaternion::operator*)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::QuaternionMultiplyGeneric)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
                Method<Quaternion(Quaternion::*)(const Quaternion&) const>("MultiplyQuaternion", &Quaternion::operator*)->
                    Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<Quaternion(Quaternion::*)(float) const>("DivideFloat", &Quaternion::operator/)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::QuaternionDivideGeneric)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Div)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Clone", [](const Quaternion& rhs) -> Quaternion { return rhs; })->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Equal", &Quaternion::operator==)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Add", &Quaternion::operator+)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Add)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<Quaternion(Quaternion::*)(const Quaternion&) const>("Subtract", &Quaternion::operator-)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Sub)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<void (Quaternion::*)(float, float, float, float)>("Set", &Quaternion::Set)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::QuaternionSetGeneric)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetElement", &Quaternion::GetElement)->
                Method("SetElement", &Quaternion::SetElement)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetLength", &Quaternion::GetLength)->
                    Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Length", "Math"))->
                Method("GetLengthSq", &Quaternion::GetLengthSq)->
                Method("GetLengthReciprocal", &Quaternion::GetLengthReciprocal)->
                Method("GetNormalized", &Quaternion::GetNormalized)->
                Method("Normalize", &Quaternion::Normalize)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("NormalizeWithLength", &Quaternion::NormalizeWithLength)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Lerp", &Quaternion::Lerp)->
                Method("Slerp", &Quaternion::Slerp)->
                Method("Squad", &Quaternion::Squad)->
                Method("GetConjugate", &Quaternion::GetConjugate)->
                Method("GetInverseFast", &Quaternion::GetInverseFast)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("InvertFast", &Quaternion::InvertFast)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetInverseFull", &Quaternion::GetInverseFull)->
                Method("InvertFull", &Quaternion::InvertFull)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Dot", &Quaternion::Dot)->
                Method("IsClose", &Quaternion::IsClose, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("IsIdentity", &Quaternion::IsIdentity, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("IsZero", &Quaternion::IsZero, behaviorContext->MakeDefaultValues(Constants::FloatEpsilon))->
                //! O3DE_DEPRECATION_NOTICE(GHI-10929)
                //! @deprecated use GetEulerRadiansXYZ()
                Method("GetEulerDegrees", &Quaternion::GetEulerDegrees)->
                    Attribute(AZ::Script::Attributes::Deprecated, true)->
                //! O3DE_DEPRECATION_NOTICE(GHI-10929)
                //! @deprecated use GetEulerRadiansXYZ()
                Method("GetEulerRadians", &Quaternion::GetEulerRadians)->
                    Attribute(AZ::Script::Attributes::Deprecated, true)->
                Method("GetEulerDegreesXYZ", &Quaternion::GetEulerDegreesXYZ)->
                Method("GetEulerRadiansXYZ", &Quaternion::GetEulerRadiansXYZ)->
                Method("GetEulerDegreesYXZ", &Quaternion::GetEulerDegreesYXZ)->
                Method("GetEulerRadiansYXZ", &Quaternion::GetEulerRadiansYXZ)->
                Method("GetEulerDegreesZYX", &Quaternion::GetEulerDegreesZYX)->
                Method("GetEulerRadiansZYX", &Quaternion::GetEulerRadiansZYX)->
                //! O3DE_DEPRECATION_NOTICE(GHI-10929)
                //! @deprecated use CreateFromEulerDegreesXYZ()
                Method("SetFromEulerDegrees", &Quaternion::SetFromEulerDegrees)->
                    Attribute(AZ::Script::Attributes::Deprecated, true)->
                //! O3DE_DEPRECATION_NOTICE(GHI-10929)
                //! @deprecated use CreateFromEulerRadiansXYZ()
                Method("SetFromEulerRadians", &Quaternion::SetFromEulerRadians)->
                    Attribute(AZ::Script::Attributes::Deprecated, true)->
                Method("GetImaginary", &Quaternion::GetImaginary)->
                Method("IsFinite", &Quaternion::IsFinite)->
                Method("GetAngle", &Quaternion::GetAngle)->
                Method("CreateIdentity", &Quaternion::CreateIdentity)->
                Method("CreateZero", &Quaternion::CreateZero)->
                Method("CreateRotationX", &Quaternion::CreateRotationX)->
                Method("CreateRotationY", &Quaternion::CreateRotationY)->
                Method("CreateRotationZ", &Quaternion::CreateRotationZ)->
                Method("CreateFromVector3", &Quaternion::CreateFromVector3)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromVector3AndValue", &Quaternion::CreateFromVector3AndValue)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromMatrix3x3", &Quaternion::CreateFromMatrix3x3)->
                Method("CreateFromMatrix4x4", &Quaternion::CreateFromMatrix4x4)->
                Method("CreateFromAxisAngle", &Quaternion::CreateFromAxisAngle)->
                Method("CreateFromScaledAxisAngle", &Quaternion::CreateFromScaledAxisAngle)->
                Method("CreateShortestArc", &Quaternion::CreateShortestArc)->
                //! O3DE_DEPRECATION_NOTICE(GHI-10929)
                //! @deprecated use CreateFromEulerDegreesXYZ()
                Method("CreateFromEulerAnglesDegrees", &Quaternion::CreateFromEulerAnglesDegrees)->
                    Attribute(AZ::Script::Attributes::Deprecated, true)->
                Method("CreateFromEulerRadiansXYZ", &Quaternion::CreateFromEulerRadiansXYZ)->
                Method("CreateFromEulerDegreesXYZ", &Quaternion::CreateFromEulerDegreesXYZ)->
                Method("CreateFromEulerRadiansYXZ", &Quaternion::CreateFromEulerRadiansYXZ)->
                Method("CreateFromEulerDegreesYXZ", &Quaternion::CreateFromEulerDegreesYXZ)->
                Method("CreateFromEulerRadiansZYX", &Quaternion::CreateFromEulerRadiansZYX)->
                Method("CreateFromEulerDegreesZYX", &Quaternion::CreateFromEulerDegreesZYX)->
                Method("CreateFromValues", &Internal::CreateFromValues)->
                Method("CreateFromBasis", &Quaternion::CreateFromBasis)
                ;
        }
    }

    Quaternion Quaternion::CreateFromMatrix3x3(const Matrix3x3& m)
    {
        return CreateFromBasis(m.GetBasisX(), m.GetBasisY(), m.GetBasisZ());
    }


    Quaternion Quaternion::CreateFromMatrix3x4(const Matrix3x4& m)
    {
        return CreateFromBasis(m.GetBasisX(), m.GetBasisY(), m.GetBasisZ());
    }


    Quaternion Quaternion::CreateFromMatrix4x4(const Matrix4x4& m)
    {
        return CreateFromBasis(m.GetBasisXAsVector3(), m.GetBasisYAsVector3(), m.GetBasisZAsVector3());
    }


    Quaternion Quaternion::CreateFromBasis(const Vector3& basisX, const Vector3& basisY, const Vector3& basisZ)
    {
        float trace;
        Quaternion result;
        if (basisZ.GetZ() < 0.0f)
        {
            if (basisX.GetX() > basisY.GetY())
            {
                trace = 1.0f + basisX.GetX() - basisY.GetY() - basisZ.GetZ();
                result = Quaternion(trace, basisX.GetY() + basisY.GetX(), basisZ.GetX() + basisX.GetZ(), basisY.GetZ() - basisZ.GetY());
            }
            else
            {
                trace = 1.0f - basisX.GetX() + basisY.GetY() - basisZ.GetZ();
                result = Quaternion(basisX.GetY() + basisY.GetX(), trace, basisY.GetZ() + basisZ.GetY(), basisZ.GetX() - basisX.GetZ());
            }
        }
        else
        {
            if (basisX.GetX() < -basisY.GetY())
            {
                trace = 1.0f - basisX.GetX() - basisY.GetY() + basisZ.GetZ();
                result = Quaternion(basisZ.GetX() + basisX.GetZ(), basisY.GetZ() + basisZ.GetY(), trace, basisX.GetY() - basisY.GetX());
            }
            else
            {
                trace = 1.0f + basisX.GetX() + basisY.GetY() + basisZ.GetZ();
                result = Quaternion(basisY.GetZ() - basisZ.GetY(), basisZ.GetX() - basisX.GetZ(), basisX.GetY() - basisY.GetX(), trace);
            }
        }
        return result * (0.5f * InvSqrt(trace));
    }


    Quaternion Quaternion::CreateShortestArc(const Vector3& v1, const Vector3& v2)
    {
        const float dotProduct = v1.Dot(v2);
        const Vector3 crossProduct = v1.Cross(v2);
        const float magnitudeProduct = Sqrt(v1.GetLengthSq() * v2.GetLengthSq());

        if (dotProduct >= 0.0f)
        {
            // The cross product is in the right direction for the vector part of the quaternion and has magnitude
            // |v1||v2|sin(t)
            // = 2|v1||v2|cos(t / 2) [sin(t / 2)]
            // Then, if we make a quaternion with the cross product as its vector part and scalar part given by
            // |v1||v2| + v1.v2
            // = |v1||v2| + |v1||v2|cos(t)
            // = |v1||v2|(1 + cos(t))
            // = |v1||v2|(2 cos(t / 2) ^ 2)
            // = 2|v1||v2|cos(t / 2) [cos(t / 2)]
            // then normalizing the quaternion will eliminate the common factor of 2|v1||v2|cos(t / 2) and give the
            // desired rotation.
            const float w = magnitudeProduct + dotProduct;
            return CreateFromVector3AndValue(crossProduct, w).GetNormalized();
        }

        // If the vectors aren't in the same hemisphere, return the product of a 180 degree rotation and the rotation
        // from -v1 to v2.  This gives much better accuracy in the case where v1 is very close to -v2.
        const float w = magnitudeProduct - dotProduct;
        const Vector3 orthogonalVector = v1.GetOrthogonalVector();
        Quaternion result = (CreateFromVector3AndValue(-crossProduct, w) * CreateFromVector3(orthogonalVector)).GetNormalized();
        return result.GetW() >= 0.0f ? result : -result;
    }

    const Quaternion Quaternion::CreateFromEulerAnglesDegrees(const Vector3& anglesInDegrees)
    {
        Quaternion result;
        result.SetFromEulerDegrees(anglesInDegrees);
        return result;
    }

    const Quaternion Quaternion::CreateFromEulerAnglesRadians(const Vector3& anglesInRadians)
    {
        Quaternion result;
        result.SetFromEulerRadians(anglesInRadians);
        return result;
    }

    Quaternion Quaternion::Slerp(const Quaternion& dest, float t) const
    {
        const float DestDot = Dot(dest);
        float cosom = (DestDot > 0.0f) ? DestDot : -DestDot;

        float sclA, sclB;
        if (cosom < 0.9999f)
        {
            float omega = Acos(cosom);

            const Simd::Vec3::FloatType angles = Simd::Vec3::LoadImmediate(omega, (1.0f - t) * omega, t * omega);
            const Simd::Vec3::FloatType sin = Simd::Vec3::Sin(angles);

            const float sinom = 1.0f / Simd::Vec3::SelectIndex0(sin);
            sclA = Simd::Vec3::SelectIndex1(sin) * sinom;
            sclB = Simd::Vec3::SelectIndex2(sin) * sinom;
        }
        else
        {
            // very close, just Lerp
            sclA = 1.0f - t;
            sclB = t;
        }

        if (DestDot < 0.0f)
        {
            sclA = -sclA;
        }

        return (*this) * sclA + dest * sclB;
    }

    // O3DE_DEPRECATION_NOTICE(GHI-10929)
    // @deprecated use GetEulerRadiansXYZ(), which is somewhat improved
    Vector3 Quaternion::GetEulerRadians() const
    {
        const float sinp = 2.0f * (m_w * m_y + m_z * m_x);
        if (sinp * sinp < 0.5f)
        {
            // roll (x-axis rotation)
            const float roll = Atan2(2.0f * (m_w * m_x - m_z * m_y), 1.0f - 2.0f * (m_x * m_x + m_y * m_y));
            // pitch (y-axis rotation)
            const float pitch = asinf(sinp);
            // yaw (z-axis rotation)
            const float yaw = Atan2(2.0f * (m_w * m_z - m_x * m_y), 1.0f - 2.0f * (m_y * m_y + m_z * m_z));
            return Vector3(roll, pitch, yaw);
        }
        // find the pitch from its cosine instead, to avoid issues with sensitivity of asin when the sine value is close to 1
        else
        {
            const float sign = sinp > 0.0f ? 1.0f : -1.0f;
            const float m12 = 2.0f * (m_z * m_y - m_w * m_x);
            const float m22 = 1.0f - 2.0f * (m_x * m_x + m_y * m_y);
            const float cospSq = m12 * m12 + m22 * m22;
            const float cosp = Sqrt(cospSq);
            const float pitch = sign * acosf(cosp);
            if (cospSq > Constants::FloatEpsilon)
            {
                const float roll = Atan2(-m12, m22);
                const float yaw = Atan2(2.0f * (m_w * m_z - m_x * m_y), 1.0f - 2.0f * (m_y * m_y + m_z * m_z));
                return Vector3(roll, pitch, yaw);
            }
            // if the pitch is close enough to +-pi/2, use a different approach because the terms used above lose roll and yaw information
            else
            {
                const float m21 = 2.0f * (m_y * m_z + m_x * m_w);
                const float m11 = 1.0f - 2.0f * (m_x * m_x + m_z * m_z);
                const float roll = Atan2(m21, m11); // this operation has very low precision an meaning
                return Vector3(roll, pitch, 0.0f);
            }
        }
    }

    /*
    * According to Matrix3x3::SetRotationPartFromQuaternion(const Quaternion& q)
    *
    * x2  = m_x * 2;  y2  = m_y * 2;  z2  = m_z * 2;
    * wx2 = m_w * x2; wy2 = m_w * y2; wz2 = m_w * z2;
    * xx2 = m_x * x2; xy2 = m_x * y2; xz2 = m_x * z2;
    * yy2 = m_y * y2; yz2 = m_y * z2; zz2 = m_y * z2;
    *
    * m11 = 1 - yy2 - zz2   m12 =     xy2 - wz2     m13 =     xz2 + wy2
    * m21 =     xy2 + wz2   m22 = 1 - xx2 - zz2     m23 =     yz2 - wx2
    * m31 =     xz2 - wy2   m32 =     yz2 + wx2     m33 = 1 - xx2 - yy2
    */
    Vector3 Quaternion::GetEulerRadiansXYZ() const
    {
        const float x2 = m_x * 2.0f;
        const float y2 = m_y * 2.0f;
        const float z2 = m_z * 2.0f;
        const float wx2 = m_w * x2;
        const float wy2 = m_w * y2;
        const float wz2 = m_w * z2;
        const float xx2 = m_x * x2;
        const float xy2 = m_x * y2;
        const float xz2 = m_x * z2;
        const float yy2 = m_y * y2;
        const float yz2 = m_y * z2;
        const float zz2 = m_z * z2;

        const float m11 = 1.f - yy2 - zz2;
        const float m12n = wz2 - xy2;
        const float m13 = wy2 + xz2;
        const float m23n = wx2 - yz2;
        const float m33 = 1.f - xx2 - yy2;

        const float cospSq = m23n * m23n + m33 * m33;
        const float signp = m13 >= 0.0f ? 1.f : -1.f;
        if (cospSq <= Constants::FloatEpsilon) // Is the pitch angle close to +-pi/2 value ?
        {
            // A gimbal lock occurs, information on roll and yaw is lost.
            // x-axis rotation (presumably roll) - deliberately zeroed.
            constexpr float x = 0.0f;
            // y-axis rotation (presumably pitch) - deliberately set near +-pi/2, but smaller in modulus.
            const float y = (AZ::Constants::HalfPi - AZ::Constants::FloatEpsilon) * signp;
            // z-axis rotation (presumably yaw) - undefined, as atan2(m21, m31) = roll - sign(pitch) * yaw;
            // because precision is low, and in order to save performance, is deliberately zeroed.
            constexpr float z = 0.0f;
            return Vector3(x, y, z);
        }

        // Normal computation using rotation matrix members
        // x-axis rotation (presumably roll)
        const float roll = Atan2(m23n, m33);
        // y-axis rotation (presumably pitch)
        // In case pitch modulus < 45 degrees, use which has reasonable precision with small angles,
        // otherwise find the pitch from its cosine for better precision.
        const float pitch = (m13 * m13 < 0.5f) ? asinf(m13) : signp * acosf(Sqrt(cospSq));
        // z-axis rotation (presumably yaw)
        const float yaw = Atan2(m12n, m11);
        return Vector3(roll, pitch, yaw);
    }

    Vector3 Quaternion::GetEulerRadiansYXZ() const
    {
        const float x2 = m_x * 2.0f;
        const float y2 = m_y * 2.0f;
        const float z2 = m_z * 2.0f;
        const float wx2 = m_w * x2;
        const float wy2 = m_w * y2;
        const float wz2 = m_w * z2;
        const float xx2 = m_x * x2;
        const float xy2 = m_x * y2;
        const float xz2 = m_x * z2;
        const float yy2 = m_y * y2;
        const float yz2 = m_y * z2;
        const float zz2 = m_z * z2;

        const float x = asinf(wx2 - yz2);
        const float y = Atan2(wy2 + xz2, 1.0f - xx2 -yy2);
        const float z = Atan2(wz2 + xy2, 1.0f - xx2 -zz2);
        return Vector3(x, y, z);
    }

    Vector3 Quaternion::GetEulerRadiansZYX() const
    {
        const float x2 = m_x * 2.0f;
        const float y2 = m_y * 2.0f;
        const float z2 = m_z * 2.0f;
        const float wx2 = m_w * x2;
        const float wy2 = m_w * y2;
        const float wz2 = m_w * z2;
        const float xx2 = m_x * x2;
        const float xy2 = m_x * y2;
        const float xz2 = m_x * z2;
        const float yy2 = m_y * y2;
        const float yz2 = m_y * z2;
        const float zz2 = m_z * z2;

        const float m11 = 1.f - yy2 - zz2;
        const float m21 = xy2 + wz2;
        const float m31n = wy2 - xz2;
        const float m32 = yz2 + wx2;
        const float m33 = 1.f - xx2 - yy2;

        const float cospSq = m32 * m32 + m33 * m33;
        const float signp = m31n >= 0.0f ? 1.f : -1.f;
        if (cospSq <= Constants::FloatEpsilon) // Is the pitch angle close to +-pi/2 value ?
        {
            // A gimbal lock occurs, information on roll and yaw is lost.
            // x-axis rotation (presumably roll) - deliberately zeroed, which is better for camera rotations.
            constexpr float x = 0.0f;
            // y-axis rotation (presumably pitch) - deliberately set near +-pi/2, but smaller in modulus.
            const float y = (AZ::Constants::HalfPi - AZ::Constants::FloatEpsilon) * signp;
            // z-axis rotation (presumably yaw) - undefined, as atan2(m21, m31) = roll - sign(pitch) * yaw;
            // because precision is low, and in order to save performance, is deliberately zeroed.
            constexpr float z = 0.0f;
            return Vector3(x, y, z);
        }

        // Normal computation using rotation matrix members
        const float x = Atan2(m32, m33);
        // In case y angle modulus < 45 degrees, use which has reasonable precision with small angles,
        // otherwise find the pitch from its cosine for better precision.
        const float y = (m31n * m31n < 0.5f) ? asinf(m31n) : signp * acosf(Sqrt(cospSq));
        const float z = Atan2(m21, m11);
        return Vector3(x, y, z);
    }

    void Quaternion::SetFromEulerRadians(const Vector3& eulerRadians)
    {
        const Simd::Vec3::FloatType half = Simd::Vec3::Splat(0.5f);
        const Simd::Vec3::FloatType angles = Simd::Vec3::Mul(half, eulerRadians.GetSimdValue());
        Simd::Vec3::FloatType sin, cos;
        Simd::Vec3::SinCos(angles, sin, cos);

        const float sx = Simd::Vec3::SelectIndex0(sin);
        const float sy = Simd::Vec3::SelectIndex1(sin);
        const float sz = Simd::Vec3::SelectIndex2(sin);
        const float cx = Simd::Vec3::SelectIndex0(cos);
        const float cy = Simd::Vec3::SelectIndex1(cos);
        const float cz = Simd::Vec3::SelectIndex2(cos);

        // rot = rotx * roty * rotz
        const float w = cx * cy * cz - sx * sy * sz;
        const float x = cx * sy * sz + sx * cy * cz;
        const float y = cx * sy * cz - sx * cy * sz;
        const float z = cx * cy * sz + sx * sy * cz;

        Set(x, y, z, w);
    }

    Quaternion Quaternion::CreateFromEulerRadiansXYZ(const Vector3& eulerRadians)
    {
        const Simd::Vec3::FloatType half = Simd::Vec3::Splat(0.5f);
        const Simd::Vec3::FloatType angles = Simd::Vec3::Mul(half, eulerRadians.GetSimdValue());
        Simd::Vec3::FloatType sin, cos;
        Simd::Vec3::SinCos(angles, sin, cos);

        const float sx = Simd::Vec3::SelectIndex0(sin);
        const float cx = Simd::Vec3::SelectIndex0(cos);
        const float sy = Simd::Vec3::SelectIndex1(sin);
        const float cy = Simd::Vec3::SelectIndex1(cos);
        const float sz = Simd::Vec3::SelectIndex2(sin);
        const float cz = Simd::Vec3::SelectIndex2(cos);

        // rot = rotx * roty * rotz
        return AZ::Quaternion(
            cx * sy * sz + sx * cy * cz,
            cx * sy * cz - sx * cy * sz,
            cx * cy * sz + sx * sy * cz,
            cx * cy * cz - sx * sy * sz
        );
    }

    Quaternion Quaternion::CreateFromEulerRadiansYXZ(const Vector3& eulerRadians)
    {
        const Simd::Vec3::FloatType half = Simd::Vec3::Splat(0.5f);
        const Simd::Vec3::FloatType angles = Simd::Vec3::Mul(half, eulerRadians.GetSimdValue());
        Simd::Vec3::FloatType sin, cos;
        Simd::Vec3::SinCos(angles, sin, cos);

        const float sx = Simd::Vec3::SelectIndex0(sin);
        const float cx = Simd::Vec3::SelectIndex0(cos);
        const float sy = Simd::Vec3::SelectIndex1(sin);
        const float cy = Simd::Vec3::SelectIndex1(cos);
        const float sz = Simd::Vec3::SelectIndex2(sin);
        const float cz = Simd::Vec3::SelectIndex2(cos);

        // rot = roty * rotx * rotz
        return AZ::Quaternion(
            cy * sx * cz + sy * cx * sz,
            sy * cx * cz - cy * sx * sz,
            cy * cx * sz - sy * sx * cz,
            cy * cx * cz + sy * sx * sz
        );
    }

    Quaternion Quaternion::CreateFromEulerRadiansZYX(const Vector3& eulerRadians)
    {
        const Simd::Vec3::FloatType half = Simd::Vec3::Splat(0.5f);
        const Simd::Vec3::FloatType angles = Simd::Vec3::Mul(half, eulerRadians.GetSimdValue());
        Simd::Vec3::FloatType sin, cos;
        Simd::Vec3::SinCos(angles, sin, cos);

        const float sx = Simd::Vec3::SelectIndex0(sin);
        const float cx = Simd::Vec3::SelectIndex0(cos);
        const float sy = Simd::Vec3::SelectIndex1(sin);
        const float cy = Simd::Vec3::SelectIndex1(cos);
        const float sz = Simd::Vec3::SelectIndex2(sin);
        const float cz = Simd::Vec3::SelectIndex2(cos);
        
        // rot = rotz * roty * rotx
        return AZ::Quaternion(
            sx * cy * cz - cx * sy * sz,
            cx * sy * cz + sx * cy * sz,
            cx * cy * sz - sx * sy * cz,
            cx * cy * cz + sx * sy * sz
        );
    }

    void Quaternion::ConvertToAxisAngle(Vector3& outAxis, float& outAngle) const
    {
        outAngle = 2.0f * Acos(GetW());

        const float sinHalfAngle = Sin(outAngle * 0.5f);
        if (sinHalfAngle > 0.0f)
        {
            outAxis = GetImaginary() / sinHalfAngle;
        }
        else
        {
            outAxis.Set(0.0f, 1.0f, 0.0f);
            outAngle = 0.0f;
        }
    }


    Vector3 Quaternion::ConvertToScaledAxisAngle() const
    {
        // Take the log of the quaternion to convert it to the exponential map
        // and multiply it by 2.0 to bring it into the scaled axis-angle representation.
        const AZ::Vector3 imaginary = GetImaginary();
        const float length = imaginary.GetLength();
        if (length < AZ::Constants::FloatEpsilon)
        {
            return imaginary * 2.0f;
        }
        else
        {
            const float halfAngle = acosf(AZ::GetClamp(GetW(), -1.0f, 1.0f));

            // Multiply by 2.0 to convert the half angle into the full one.
            return halfAngle * 2.0f * (imaginary / length);
        }
    }
} // namespace AZ
