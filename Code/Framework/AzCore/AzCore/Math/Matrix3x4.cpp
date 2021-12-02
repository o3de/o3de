/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/MathScriptHelpers.h>

namespace AZ
{
    namespace
    {
        const Matrix3x4 Matrix3x4Identity = Matrix3x4::CreateIdentity();
    }

    namespace Internal
    {
        void Matrix3x4DefaultConstructor(Matrix3x4* thisPtr)
        {
            new (thisPtr) Matrix3x4(Matrix3x4::CreateIdentity());
        }

        void Matrix3x4SetRowGeneric(Matrix3x4* thisPtr, ScriptDataContext& dc)
        {
            bool rowIsSet = false;
            if (dc.GetNumArguments() >= 5)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3) && dc.IsNumber(4))
                    {
                        float x = 0;
                        float y = 0;
                        float z = 0;
                        float w = 0;
                        dc.ReadArg(1, x);
                        dc.ReadArg(2, y);
                        dc.ReadArg(3, z);
                        dc.ReadArg(4, w);
                        thisPtr->SetRow(index, Vector4(x, y, z, w));
                        rowIsSet = true;
                    }
                }
            }
            else if (dc.GetNumArguments() == 3)
            {
                if (dc.IsNumber(0) && dc.IsClass<Vector3>(1) && dc.IsNumber(2))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(1, vector3);

                    float w = 0;
                    dc.ReadArg(2, w);
                    thisPtr->SetRow(index, vector3, w);
                    rowIsSet = true;
                }
            }
            else if (dc.GetNumArguments() >= 2)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsClass<Vector4>(1))
                    {
                        Vector4 vector4 = Vector4::CreateZero();
                        dc.ReadArg(1, vector4);
                        thisPtr->SetRow(index, vector4);
                        rowIsSet = true;
                    }
                }
            }

            AZ_Error("Script", rowIsSet,
                "Matrix3x4 SetRow only supports the following signatures: "
                "SetRow(index, Vector4), SetRow(index, Vector3, number) or SetRow(index, x, y, z, w).");
        }

        void Matrix3x4SetColumnGeneric(Matrix3x4* thisPtr, ScriptDataContext& dc)
        {
            bool columnIsSet = false;
            if (dc.GetNumArguments() >= 4)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
                    {
                        float x = 0;
                        float y = 0;
                        float z = 0;
                        dc.ReadArg(1, x);
                        dc.ReadArg(2, y);
                        dc.ReadArg(3, z);
                        thisPtr->SetColumn(index, Vector3(x, y, z));
                        columnIsSet = true;
                    }
                }
            }
            else if (dc.GetNumArguments() >= 2)
            {
                if (dc.IsNumber(0))
                {
                    int index = 0;
                    dc.ReadArg(0, index);

                    if (dc.IsClass<Vector3>(1))
                    {
                        Vector3 vector3 = Vector3::CreateZero();
                        dc.ReadArg(1, vector3);
                        thisPtr->SetColumn(index, vector3);
                        columnIsSet = true;
                    }
                }
            }

            AZ_Error("Script", columnIsSet,
                "Matrix3x4 SetColumn only supports the following signatures: "
                "SetColumn(index, Vector4), SetColumn(index, Vector3, number) or SetColumn(index, x, y, z, w).");
        }

        void Matrix3x4SetTranslationGeneric(Matrix3x4* thisPtr, ScriptDataContext& dc)
        {
            bool translationIsSet = false;

            if (dc.GetNumArguments() == 3 &&
                dc.IsNumber(0) &&
                dc.IsNumber(1) &&
                dc.IsNumber(2))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                thisPtr->SetTranslation(x, y, z);
                translationIsSet = true;
            }
            else if (dc.GetNumArguments() == 1 &&
                dc.IsClass<Vector3>(0))
            {
                Vector3 translation = Vector3::CreateZero();
                dc.ReadArg(0, translation);
                thisPtr->SetTranslation(translation);
                translationIsSet = true;
            }

            AZ_Error("Script", translationIsSet,
                "Matrix3x4 SetTranslation only supports the following signatures: "
                "SetTranslation(Vector3), SetTranslation(number, number, number).");
        }

        void Matrix3x4MultiplyGeneric(const Matrix3x4* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<Matrix3x4>(0))
                {
                    Matrix3x4 rhs = Matrix3x4::CreateZero();
                    dc.ReadArg(0, rhs);
                    Matrix3x4 result = *thisPtr * rhs;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(0, vector3);
                    Vector3 result = *thisPtr * vector3;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector4>(0))
                {
                    Vector4 vector4 = Vector4::CreateZero();
                    dc.ReadArg(0, vector4);
                    Vector4 result = *thisPtr * vector4;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Matrix3x4 Multiply should have only 1 argument - Matrix3x4, Vector3 or a Vector4.");
                dc.PushResult(Matrix3x4::CreateIdentity());
            }
        }

        void Matrix3x4GetRowsMultipleReturn(const Matrix3x4* thisPtr, ScriptDataContext& dc)
        {
            Vector4 row0(Vector4::CreateZero()), row1(Vector4::CreateZero()), row2(Vector4::CreateZero());
            thisPtr->GetRows(&row0, &row1, &row2);
            dc.PushResult(row0);
            dc.PushResult(row1);
            dc.PushResult(row2);
        }

        void Matrix3x4GetColumnsMultipleReturn(const Matrix3x4* thisPtr, ScriptDataContext& dc)
        {
            Vector3 column0(Vector3::CreateZero()), column1(Vector3::CreateZero()), column2(Vector3::CreateZero()),
                column3(Vector3::CreateZero());
            thisPtr->GetColumns(&column0, &column1, &column2, &column3);
            dc.PushResult(column0);
            dc.PushResult(column1);
            dc.PushResult(column2);
            dc.PushResult(column3);
        }

        void Matrix3x4GetBasisAndTranslationMultipleReturn(const Matrix3x4* thisPtr, ScriptDataContext& dc)
        {
            Vector3 basisX(Vector3::CreateZero()), basisY(Vector3::CreateZero()), basisZ(Vector3::CreateZero()),
                translation(Vector3::CreateZero());
            thisPtr->GetBasisAndTranslation(&basisX, &basisY, &basisZ, &translation);
            dc.PushResult(basisX);
            dc.PushResult(basisY);
            dc.PushResult(basisZ);
            dc.PushResult(translation);
        }

        AZStd::string Matrix3x4ToString(const Matrix3x4& t)
        {
            return AZStd::string::format("%s,%s,%s,%s",
                Vector3ToString(t.GetColumn(0)).c_str(), Vector3ToString(t.GetColumn(1)).c_str(),
                Vector3ToString(t.GetColumn(2)).c_str(), Vector3ToString(t.GetColumn(3)).c_str());
        }
    } // namespace Internal

    void Matrix3x4::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Matrix3x4>()
                ->Serializer<FloatBasedContainerSerializer<Matrix3x4, &Matrix3x4::CreateFromColumnMajorFloat12,
                    &Matrix3x4::StoreToColumnMajorFloat12, &GetTransformEpsilon, 12>>();
        }

        if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<Matrix3x4>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "math")
                ->Attribute(AZ::Script::Attributes::Category, "Math")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::Matrix3x4DefaultConstructor)
                ->Property<Vector3(Matrix3x4::*)() const, void (Matrix3x4::*)(const Vector3&)>("basisX",
                    &Matrix3x4::GetBasisX, &Matrix3x4::SetBasisX)
                ->Property<Vector3(Matrix3x4::*)() const, void (Matrix3x4::*)(const Vector3&)>("basisY",
                    &Matrix3x4::GetBasisY, &Matrix3x4::SetBasisY)
                ->Property<Vector3(Matrix3x4::*)() const, void (Matrix3x4::*)(const Vector3&)>("basisZ",
                    &Matrix3x4::GetBasisZ, &Matrix3x4::SetBasisZ)
                ->Property<Vector3(Matrix3x4::*)() const, void (Matrix3x4::*)(const Vector3&)>("translation",
                    &Matrix3x4::GetTranslation, &Matrix3x4::SetTranslation)
                ->Method("ToString", &Internal::Matrix3x4ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ->Method<Vector3(Matrix3x4::*)(const Vector3&) const>("MultiplyVector3", &Matrix3x4::operator*)
                ->Method<Vector4(Matrix3x4::*)(const Vector4&) const>("MultiplyVector4", &Matrix3x4::operator*)
                ->Method<Matrix3x4(Matrix3x4::*)(const Matrix3x4&) const>("MultiplyMatrix3x4", &Matrix3x4::operator*)
                    ->Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x4MultiplyGeneric)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("Equal", &Matrix3x4::operator==)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("Clone", [](const Matrix3x4& rhs) -> Matrix3x4 { return rhs; })
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetElement", &Matrix3x4::GetElement)
                ->Method("SetElement", &Matrix3x4::SetElement)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetRow", [](const Matrix3x4& matrix, int row) { return matrix.GetRow(AZ::GetClamp(row, 0, 2)); })
                ->Method("GetRows", &Matrix3x4::GetRows)
                    ->Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x4GetRowsMultipleReturn)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetRowAsVector3", [](const Matrix3x4& matrix, int row) { return matrix.GetRowAsVector3(AZ::GetClamp(row, 0, 2)); })
                ->Method<void (Matrix3x4::*)(int, const Vector4&)>("SetRow", &Matrix3x4::SetRow)
                    ->Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x4SetRowGeneric)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("SetRows", &Matrix3x4::SetRows)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetColumn", [](const Matrix3x4& matrix, int col) { return matrix.GetColumn(AZ::GetClamp(col, 0, 3)); })
                ->Method("GetColumns", &Matrix3x4::GetColumns)
                    ->Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x4GetColumnsMultipleReturn)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method<void (Matrix3x4::*)(int, const Vector3&)>("SetColumn", &Matrix3x4::SetColumn)
                    ->Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x4SetColumnGeneric)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("SetColumns", &Matrix3x4::SetColumns)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetTranslation", &Matrix3x4::GetTranslation)
                ->Method("SetBasisAndTranslation", &Matrix3x4::SetBasisAndTranslation)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetBasisAndTranslation", &Matrix3x4::GetBasisAndTranslation)
                    ->Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x4GetBasisAndTranslationMultipleReturn)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("Multiply3x3", &Matrix3x4::Multiply3x3)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetTranspose3x3", &Matrix3x4::GetTranspose3x3)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("Transpose3x3", &Matrix3x4::Transpose3x3)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method<void (Matrix3x4::*)(const Vector3&)>("SetTranslation", &Matrix3x4::SetTranslation)
                    ->Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Matrix3x4SetTranslationGeneric)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetTranspose", &Matrix3x4::GetTranspose)
                ->Method("Transpose", &Matrix3x4::Transpose)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetInverseFull", &Matrix3x4::GetInverseFull)
                ->Method("InvertFull", &Matrix3x4::InvertFull)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetInverseFast", &Matrix3x4::GetInverseFast)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("InvertFast", &Matrix3x4::InvertFast)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("RetrieveScale", &Matrix3x4::RetrieveScale)
                ->Method("ExtractScale", &Matrix3x4::ExtractScale)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetMultipliedByScale",
                    [](const Matrix3x4& matrix, const Vector3& scale){ auto result = matrix; result.MultiplyByScale(scale); return result; })
                ->Method("IsOrthogonal", &Matrix3x4::IsOrthogonal, behaviorContext->MakeDefaultValues(Constants::Tolerance))
                ->Method("GetOrthogonalized", &Matrix3x4::GetOrthogonalized)
                ->Method("Orthogonalize", &Matrix3x4::Orthogonalize)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("IsClose", &Matrix3x4::IsClose, behaviorContext->MakeDefaultValues(Constants::Tolerance))
                ->Method("SetRotationPartFromQuaternion", &Matrix3x4::SetRotationPartFromQuaternion)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetDeterminant3x3", &Matrix3x4::GetDeterminant3x3)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("IsFinite", &Matrix3x4::IsFinite)
                ->Method("CreateIdentity", &Matrix3x4::CreateIdentity)
                ->Method("CreateZero", &Matrix3x4::CreateZero)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("CreateFromValue", &Matrix3x4::CreateFromValue)
                ->Method("CreateFromRows", &Matrix3x4::CreateFromRows)
                ->Method("CreateFromColumns", &Matrix3x4::CreateFromColumns)
                ->Method("CreateRotationX", [](float angle) { return Matrix3x4::CreateRotationX(AZ::DegToRad(angle)); })
                ->Method("CreateRotationY", [](float angle) { return Matrix3x4::CreateRotationY(AZ::DegToRad(angle)); })
                ->Method("CreateRotationZ", [](float angle) { return Matrix3x4::CreateRotationZ(AZ::DegToRad(angle)); })
                ->Method("CreateFromQuaternion", &Matrix3x4::CreateFromQuaternion)
                ->Method("CreateFromQuaternionAndTranslation", &Matrix3x4::CreateFromQuaternionAndTranslation)
                ->Method("CreateFromMatrix3x3", &Matrix3x4::CreateFromMatrix3x3)
                ->Method("CreateFromMatrix3x3AndTranslation", &Matrix3x4::CreateFromMatrix3x3AndTranslation)
                ->Method("CreateScale", &Matrix3x4::CreateScale)
                ->Method("CreateDiagonal", &Matrix3x4::CreateDiagonal)
                ->Method("CreateTranslation", &Matrix3x4::CreateTranslation)
                ->Method("UnsafeCreateFromMatrix4x4", &Matrix3x4::UnsafeCreateFromMatrix4x4);
        }
    }


    Matrix3x4 Matrix3x4::CreateLookAt(const Vector3& from, const Vector3& to, Matrix3x4::Axis forwardAxis)
    {
        Matrix3x4 result = CreateIdentity();

        Vector3 targetForward = to - from;

        if (targetForward.IsZero())
        {
            AZ_Error("Matrix3x4", false, "Can't create look-at matrix when 'to' and 'from' positions are equal.");
            return result;
        }

        targetForward.Normalize();

        // Open 3D Engine is Z-up and is right-handed.
        Vector3 up = Vector3::CreateAxisZ();

        // We have a degenerate case if target forward is parallel to the up axis,
        // in which case we'll have to pick a new up vector
        const float absDot = Abs(targetForward.Dot(up));
        if (absDot > 1.0f - 0.001f)
        {
            up = targetForward.CrossYAxis();
        }

        Vector3 right = targetForward.Cross(up);
        right.Normalize();
        up = right.Cross(targetForward);
        up.Normalize();

        // Passing in forwardAxis allows you to force a particular local-space axis to look
        // at the target point.  In Open 3D Engine, the default is forward is along Y+.
        switch (forwardAxis)
        {
        case Axis::XPositive:
            result.SetBasisAndTranslation(targetForward, -right, up, from);
            break;
        case Axis::XNegative:
            result.SetBasisAndTranslation(-targetForward, right, up, from);
            break;
        case Axis::YPositive:
            result.SetBasisAndTranslation(right, targetForward, up, from);
            break;
        case Axis::YNegative:
            result.SetBasisAndTranslation(-right, -targetForward, up, from);
            break;
        case Axis::ZPositive:
            result.SetBasisAndTranslation(right, -up, targetForward, from);
            break;
        case Axis::ZNegative:
            result.SetBasisAndTranslation(right, up, -targetForward, from);
            break;
        default:
            result.SetBasisAndTranslation(right, targetForward, up, from);
            break;
        }

        return result;
    }


    Matrix3x4 Matrix3x4::GetInverseFull() const
    {
        Matrix3x4 result;

        // compute the first row of the matrix of cofactors
        result.SetRow(0,
            GetElement(1, 1) * GetElement(2, 2) - GetElement(1, 2) * GetElement(2, 1),
            GetElement(2, 1) * GetElement(0, 2) - GetElement(2, 2) * GetElement(0, 1),
            GetElement(0, 1) * GetElement(1, 2) - GetElement(0, 2) * GetElement(1, 1),
            0.0f
        );

        // calculate the determinant
        const float determinant = result.m_rows[0].Dot3(GetColumn(0));

        if (determinant != 0.0f)
        {
            float determinantInv = 1.0f / determinant;
            result.m_rows[0] *= determinantInv;
            result.SetRow(1,
                determinantInv * (GetElement(1, 2) * GetElement(2, 0) - GetElement(1, 0) * GetElement(2, 2)),
                determinantInv * (GetElement(2, 2) * GetElement(0, 0) - GetElement(2, 0) * GetElement(0, 2)),
                determinantInv * (GetElement(0, 2) * GetElement(1, 0) - GetElement(0, 0) * GetElement(1, 2)),
                0.0f
            );
            result.SetRow(2,
                determinantInv * (GetElement(1, 0) * GetElement(2, 1) - GetElement(1, 1) * GetElement(2, 0)),
                determinantInv * (GetElement(2, 0) * GetElement(0, 1) - GetElement(2, 1) * GetElement(0, 0)),
                determinantInv * (GetElement(0, 0) * GetElement(1, 1) - GetElement(0, 1) * GetElement(1, 0)),
                0.0f
            );
        }
        else
        {
            AZ_Warning("Matrix3x4", false, "GetInverseFull could not calculate inverse as determinant was zero");
            result = CreateIdentity();
        }

        // translation
        const Vector3& translation = GetTranslation();
        result.SetElement(0, 3, -result.m_rows[0].Dot3(translation));
        result.SetElement(1, 3, -result.m_rows[1].Dot3(translation));
        result.SetElement(2, 3, -result.m_rows[2].Dot3(translation));

        return result;
    }


    bool Matrix3x4::IsOrthogonal(float tolerance) const
    {
        const Vector3 row0 = m_rows[0].GetAsVector3();
        const Vector3 row1 = m_rows[1].GetAsVector3();
        const Vector3 row2 = m_rows[2].GetAsVector3();
        return
            row0.IsNormalized(tolerance) &&
            row1.IsNormalized(tolerance) &&
            row2.IsNormalized(tolerance) &&
            row0.IsPerpendicular(row1, tolerance) &&
            row0.IsPerpendicular(row2, tolerance) &&
            row1.IsPerpendicular(row2, tolerance);
    }


    Matrix3x4 Matrix3x4::GetOrthogonalized() const
    {
        Matrix3x4 result;
        const Vector3 translation = GetTranslation();
        const Vector3 row0 = GetRowAsVector3(1).Cross(GetRowAsVector3(2)).GetNormalizedSafe();
        const Vector3 row1 = GetRowAsVector3(2).Cross(row0).GetNormalizedSafe();
        const Vector3 row2 = row0.Cross(row1);
        result.SetRow(0, row0, translation.GetX());
        result.SetRow(1, row1, translation.GetY());
        result.SetRow(2, row2, translation.GetZ());
        return result;
    }


    Vector3 Matrix3x4::GetEulerRadians() const
    {
        Vector3 result;
        const float c2 = Vector2(GetElement(0, 0), GetElement(0, 1)).GetLength();
        result.SetX(-Atan2(GetElement(1, 2), GetElement(2, 2)));
        result.SetY(-Atan2(-GetElement(0, 2), c2));
        const Vector2 angles = Vector2(Simd::Vec2::Sin(Simd::Vec2::LoadImmediate(-result.GetX(), result.GetX() + Constants::HalfPi)));
        const float s1 = angles.GetX();
        const float c1 = angles.GetY();
        result.SetZ(-Atan2(-c1 * GetElement(1, 0) + s1 * GetElement(2, 0), c1 * GetElement(1, 1) - s1 * GetElement(2, 1)));
        return Vector3(Simd::Vec3::Wrap(result.GetSimdValue(), Simd::Vec3::ZeroFloat(), Simd::Vec3::Splat(Constants::TwoPi)));
    }


    void Matrix3x4::SetFromEulerRadians(const Vector3& eulerRadians)
    {
        Simd::Vec3::FloatType sin, cos;
        Simd::Vec3::SinCos(eulerRadians.GetSimdValue(), sin, cos);

        const float sx = Simd::Vec3::SelectFirst(sin);
        const float sy = Simd::Vec3::SelectSecond(sin);
        const float sz = Simd::Vec3::SelectThird(sin);
        const float cx = Simd::Vec3::SelectFirst(cos);
        const float cy = Simd::Vec3::SelectSecond(cos);
        const float cz = Simd::Vec3::SelectThird(cos);

        SetRow(0, cy * cz, -cy * sz, sy, 0.0f);
        SetRow(1, cx * sz + sx * sy * cz, cx * cz - sx * sy * sz, -sx * cy, 0.0f);
        SetRow(2, sx * sz - cx * sy * cz, sx * cz + cx * sy * sz, cx * cy, 0.0f);
    }


    void Matrix3x4::SetRotationPartFromQuaternion(const Quaternion& quaternion)
    {
        const float tx = quaternion.GetX() * 2.0f;
        const float ty = quaternion.GetY() * 2.0f;
        const float tz = quaternion.GetZ() * 2.0f;
        const float twx = quaternion.GetW() * tx;
        const float twy = quaternion.GetW() * ty;
        const float twz = quaternion.GetW() * tz;
        const float txx = quaternion.GetX() * tx;
        const float txy = quaternion.GetX() * ty;
        const float txz = quaternion.GetX() * tz;
        const float tyy = quaternion.GetY() * ty;
        const float tyz = quaternion.GetY() * tz;
        const float tzz = quaternion.GetZ() * tz;

        SetRow(0, 1.0f - (tyy + tzz), txy - twz, txz + twy, m_rows[0].GetW());
        SetRow(1, txy + twz, 1.0f - (txx + tzz), tyz - twx, m_rows[1].GetW());
        SetRow(2, txz - twy, tyz + twx, 1.0f - (txx + tyy), m_rows[2].GetW());
    }
} // namespace AZ
