/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MathScriptHelpers.h>

namespace AZ
{
    namespace Internal
    {
        Matrix4x4 ConstructMatrix4x4(Vector4 row0, Vector4 row1, Vector4 row2, Vector4 row3)
        {
            Matrix4x4 result;
            result.SetRows(row0, row1, row2, row3);
            return result;
        }

        AZ::Matrix4x4 ConstructMatrix4x4FromValues
            ( float v00, float v01, float v02, float v03
            , float v10, float v11, float v12, float v13
            , float v20, float v21, float v22, float v23
            , float v30, float v31, float v32, float v33)
        {
            return ConstructMatrix4x4(AZ::Vector4(v00, v01, v02, v03), AZ::Vector4(v10, v11, v12, v13), AZ::Vector4(v20, v21, v22, v23), AZ::Vector4(v30, v31, v32, v33));
        }

        void Matrix4x4DefaultConstructor(Matrix4x4* thisPtr)
        {
            new (thisPtr) Matrix4x4(Matrix4x4::CreateIdentity());
        }

        void Matrix4x4SetRowGeneric(Matrix4x4* thisPtr, ScriptDataContext& dc)
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

            if (!rowIsSet)
            {
                AZ_Error("Script", false, "Matrix4x4 SetRow only supports the following signatures: SetRow(index,Vector4), SetRow(index,Vector3,number) or SetRow(index,x,y,z,w).");
            }
        }

        void Matrix4x4SetColumnGeneric(Matrix4x4* thisPtr, ScriptDataContext& dc)
        {
            bool columnIsSet = false;
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
                        thisPtr->SetColumn(index, Vector4(x, y, z, w));
                        columnIsSet = true;
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
                    thisPtr->SetColumn(index, vector3, w);
                    columnIsSet = true;
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
                        thisPtr->SetColumn(index, vector4);
                        columnIsSet = true;
                    }
                }
            }

            if (!columnIsSet)
            {
                AZ_Error("Script", false, "Matrix4x4 SetColumn only supports the following signatures: SetColumn(index,Vector4), SetColumn(index,Vector3,number) or SetColumn(index,x,y,z,w).");
            }
        }

        void Matrix4x4SetTranslationGeneric(Matrix4x4* thisPtr, ScriptDataContext& dc)
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
            else if (dc.GetNumArguments() == 1 && dc.IsClass<Vector3>(0))
            {
                Vector3 translation = Vector3::CreateZero();
                dc.ReadArg(0, translation);
                thisPtr->SetTranslation(translation);
                translationIsSet = true;
            }

            if (!translationIsSet)
            {
                AZ_Error("Script", false, "Matrix4x4 SetTranslation only supports the following signatures: SetTranslation(Vector3), SetTranslation(number,number,number)");
            }
        }

        void Matrix4x4GetRowsMultipleReturn(const Matrix4x4* thisPtr, ScriptDataContext& dc)
        {
            Vector4 row0(Vector4::CreateZero()), row1(Vector4::CreateZero()), row2(Vector4::CreateZero()), row3(Vector4::CreateZero());
            thisPtr->GetRows(&row0, &row1, &row2, &row3);
            dc.PushResult(row0);
            dc.PushResult(row1);
            dc.PushResult(row2);
            dc.PushResult(row3);
        }

        void Matrix4x4GetColumnsMultipleReturn(const Matrix4x4* thisPtr, ScriptDataContext& dc)
        {
            Vector4 column0(Vector4::CreateZero()), column1(Vector4::CreateZero()), column2(Vector4::CreateZero()), column3(Vector4::CreateZero());
            thisPtr->GetColumns(&column0, &column1, &column2, &column3);
            dc.PushResult(column0);
            dc.PushResult(column1);
            dc.PushResult(column2);
            dc.PushResult(column3);
        }

        void Matrix4x4MultiplyGeneric(const Matrix4x4* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<Matrix4x4>(0))
                {
                    Matrix4x4 mat4x4 = Matrix4x4::CreateZero();
                    dc.ReadArg(0, mat4x4);
                    Matrix4x4 result = *thisPtr * mat4x4;
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
                AZ_Error("Script", false, "Matrix4x4 multiply should have only 1 argument - Matrix4x4, Vector3 or Vector4!");
                dc.PushResult(Matrix4x4::CreateIdentity());
            }
        }

        AZStd::string Matrix4x4ToString(const Matrix4x4& m)
        {
            return AZStd::string::format("%s,%s,%s,%s",
                Vector4ToString(m.GetColumn(0)).c_str(), Vector4ToString(m.GetColumn(1)).c_str(),
                Vector4ToString(m.GetColumn(2)).c_str(), Vector4ToString(m.GetColumn(3)).c_str());
        }
    }

    void Matrix4x4::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Matrix4x4>()->
                Serializer<FloatBasedContainerSerializer<Matrix4x4, &Matrix4x4::CreateFromColumnMajorFloat16, &Matrix4x4::StoreToColumnMajorFloat16, &GetTransformEpsilon, 16> >();
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Matrix4x4>()->
                Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Common)->
                Attribute(Script::Attributes::Module, "math")->
                Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Attribute(Script::Attributes::GenericConstructorOverride, &Internal::Matrix4x4DefaultConstructor)->
                Property<Vector4(Matrix4x4::*)() const, void (Matrix4x4::*)(const Vector4&)>("basisX", &Matrix4x4::GetBasisX, &Matrix4x4::SetBasisX)->
                Property<Vector4(Matrix4x4::*)() const, void (Matrix4x4::*)(const Vector4&)>("basisY", &Matrix4x4::GetBasisY, &Matrix4x4::SetBasisY)->
                Property<Vector4(Matrix4x4::*)() const, void (Matrix4x4::*)(const Vector4&)>("basisZ", &Matrix4x4::GetBasisZ, &Matrix4x4::SetBasisZ)->
                Property<Vector3(Matrix4x4::*)() const, void (Matrix4x4::*)(const Vector3&)>("translation", &Matrix4x4::GetTranslation, &Matrix4x4::SetTranslation)->
                Method("ToString", &Internal::Matrix4x4ToString)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::ToString)->
                Method<Vector3(Matrix4x4::*)(const Vector3&) const>("MultiplyVector3", &Matrix4x4::operator*)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix4x4MultiplyGeneric)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Mul)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method<Vector4(Matrix4x4::*)(const Vector4&) const>("MultiplyVector4", &Matrix4x4::operator*)->
                    Attribute(Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                Method<Matrix4x4(Matrix4x4::*)(const Matrix4x4&) const>("MultiplyMatrix4x4", &Matrix4x4::operator*)->
                    Attribute(Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                Method<Matrix4x4(Matrix4x4::*)(const Matrix4x4&) const>("Add", &Matrix4x4::operator+)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Add)->
                Method<Matrix4x4(Matrix4x4::*)(const Matrix4x4&) const>("Subtract", &Matrix4x4::operator-)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Sub)->
                Method("Equal", &Matrix4x4::operator==)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Equal)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Clone", [](const Matrix4x4& rhs) -> Matrix4x4 { return rhs; })->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetElement", &Matrix4x4::GetElement)->
                Method("SetElement", &Matrix4x4::SetElement)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetRow", &Matrix4x4::GetRow)->
                Method("GetRowAsVector3", &Matrix4x4::GetRowAsVector3)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetColumnAsVector3", &Matrix4x4::GetColumnAsVector3)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetRows", &Matrix4x4::GetRows)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix4x4GetRowsMultipleReturn)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method<void (Matrix4x4::*)(int, const Vector4&)>("SetRow", &Matrix4x4::SetRow)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix4x4SetRowGeneric)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("SetRows", &Matrix4x4::SetRows)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetColumn", &Matrix4x4::GetColumn)->
                Method("GetColumns", &Matrix4x4::GetColumn)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix4x4GetColumnsMultipleReturn)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method<void (Matrix4x4::*)(int, const Vector4&)>("SetColumn", &Matrix4x4::SetColumn)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix4x4SetColumnGeneric)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("SetColumns", &Matrix4x4::SetColumns)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("SetBasisAndTranslation", &Matrix4x4::SetBasisAndTranslation)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("TransposedMultiply3x3", &Matrix4x4::TransposedMultiply3x3)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Multiply3x3", &Matrix4x4::Multiply3x3)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetTranslation", &Matrix4x4::GetTranslation)->
                Method<void (Matrix4x4::*)(const Vector3&)>("SetTranslation", &Matrix4x4::SetTranslation)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix4x4SetTranslationGeneric)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetTranspose", &Matrix4x4::GetTranspose)->
                Method("Transpose", &Matrix4x4::Transpose)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetInverseFull", &Matrix4x4::GetInverseFull)->
                Method("InvertFull", &Matrix4x4::InvertFull)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetInverseTransform", &Matrix4x4::GetInverseTransform)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("InvertTransform", &Matrix4x4::InvertTransform)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetInverseFast", &Matrix4x4::GetInverseFast)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("InvertFast", &Matrix4x4::InvertFast)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("RetrieveScale", &Matrix4x4::RetrieveScale)->
                Method("ExtractScale", &Matrix4x4::ExtractScale)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("MultiplyByScale", &Matrix4x4::MultiplyByScale)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsClose", &Matrix4x4::IsClose, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("SetRotationPartFromQuaternion", &Matrix4x4::SetRotationPartFromQuaternion)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetDiagonal", &Matrix4x4::GetDiagonal)->
                Method("IsFinite", &Matrix4x4::IsFinite)->
                Method("CreateIdentity", &Matrix4x4::CreateIdentity)->
                Method("CreateZero", &Matrix4x4::CreateZero)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromValue", &Matrix4x4::CreateFromValue)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromRows", &Matrix4x4::CreateFromRows)->
                Method("CreateFromColumns", &Matrix4x4::CreateFromColumns)->
                Method("CreateRotationX", &Matrix4x4::CreateRotationX)->
                Method("CreateRotationY", &Matrix4x4::CreateRotationY)->
                Method("CreateRotationZ", &Matrix4x4::CreateRotationZ)->
                Method("CreateFromQuaternion", &Matrix4x4::CreateFromQuaternion)->
                Method("CreateFromQuaternionAndTranslation", &Matrix4x4::CreateFromQuaternionAndTranslation)->
                Method("CreateScale", &Matrix4x4::CreateScale)->
                Method("CreateDiagonal", &Matrix4x4::CreateDiagonal)->
                Method("CreateTranslation", &Matrix4x4::CreateTranslation)->
                Method("CreateProjection", &Matrix4x4::CreateProjection)->
                Method("CreateProjectionFov", &Matrix4x4::CreateProjectionFov)->
                Method("CreateProjectionOffset", &Matrix4x4::CreateProjectionOffset)->
                Method("ConstructFromValues", &Internal::ConstructMatrix4x4)->
                Method("ConstructFromValuesNumeric", &Internal::ConstructMatrix4x4FromValues)
                ;
        }
    }

    void Matrix4x4::SetRotationPartFromQuaternion(const Quaternion& q)
    {
        float tx = q.GetX() * 2.0f;
        float ty = q.GetY() * 2.0f;
        float tz = q.GetZ() * 2.0f;
        float twx = q.GetW() * tx;
        float twy = q.GetW() * ty;
        float twz = q.GetW() * tz;
        float txx = q.GetX() * tx;
        float txy = q.GetX() * ty;
        float txz = q.GetX() * tz;
        float tyy = q.GetY() * ty;
        float tyz = q.GetY() * tz;
        float tzz = q.GetZ() * tz;

        SetElement(0, 0, 1.0f - (tyy + tzz)); // 1.0-2yy-2zz   2xy-2wz       2xz+2wy
        SetElement(0, 1, txy - twz);
        SetElement(0, 2, txz + twy);

        SetElement(1, 0, txy + twz);          // 2xy+2wz   1.0-2xx-2zz       2yz-2wx
        SetElement(1, 1, 1.0f - (txx + tzz));
        SetElement(1, 2, tyz - twx);

        SetElement(2, 0, txz - twy);          // 2xz-2wy       2yz+2wx   1.0-2xx-2yy
        SetElement(2, 1, tyz + twx);
        SetElement(2, 2, 1.0f - (txx + tyy));
    }

    Matrix4x4 Matrix4x4::CreateProjection(float fovY, float aspectRatio, float nearDist, float farDist)
    {
        // This section contains some notes about camera matrices and field of view, because there are some subtle differences
        // between the convention Open 3D Engine uses and what you might be used to from other software packages.
        // Our camera space has the camera looking down the *positive* z-axis, the x-axis points towards the left of the screen,
        // and the y-axis points towards the top of the screen.
        //
        // The projection matrix transforms vectors in camera space into clip space. In clip space, x=-1 is the
        // left of the screen, x=+1 is the right of the screen, y=-1 is the bottom of the screen, y=+1 is the top
        // of the screen. Points at the near plane are mapped to z=-1, and points at the far plane are mapped to
        // z=+1. There is also an alternative projection matrix below which maps to the z=0,1 range instead, which
        // is used by Direct3D.
        //
        // The relationship between fovX and fovY is non-linear. If we used a linear relationship when constructing
        // the projection matrix there would be some slight squashing. An alternative correct method which doesn't use
        // the fov constructs the projection using the screen window size projected onto the near plane.
        float sin, cos;
        SinCos(0.5f * fovY, sin, cos);
        float cotY = cos / sin;
        float cotX = cotY / aspectRatio;
        return CreateProjectionInternal(cotX, cotY, nearDist, farDist);
    }

    Matrix4x4 Matrix4x4::CreateProjectionFov(float fovX, float fovY, float nearDist, float farDist)
    {
        // note that the relationship between fovX and fovY is not linear with the aspect ratio, so prefer the
        // aspect ratio version of the projection matrix, to avoid unwanted stretching, unless you really know both
        // FOV's exactly
        Simd::Vec4::FloatType angles = Simd::Vec4::LoadImmediate(0.5f * fovX, 0.5f * fovX, 0.5f * fovY, 0.5f * fovY);
        Simd::Vec4::FloatType values = Simd::Vec4::SinCos(angles);
        float cotX = Simd::Vec4::SelectIndex1(values) / Simd::Vec4::SelectIndex0(values);
        float cotY = Simd::Vec4::SelectIndex3(values) / Simd::Vec4::SelectIndex2(values);
        return CreateProjectionInternal(cotX, cotY, nearDist, farDist);
    }

    Matrix4x4 Matrix4x4::CreateProjectionInternal(float cotX, float cotY, float nearDist, float farDist)
    {
        AZ_MATH_ASSERT(nearDist > 0.0f, "Near plane distance must be greater than 0");
        AZ_MATH_ASSERT(farDist > nearDist, "Far plane distance must be greater than near plane distance");

        // construct projection matrix
        Matrix4x4 result;
        float invfn = 1.0f / (farDist - nearDist);
#if defined(RENDER_WINNT_DIRECTX)
        // this matrix maps the z values into the range [0,1]
        result.SetRow(0, -cotX, 0.0f, 0.0f,         0.0f);
        result.SetRow(1, 0.0f, cotY, 0.0f,          0.0f);
        result.SetRow(2, 0.0f, 0.0f, farDist * invfn, -farDist * nearDist * invfn);
        result.SetRow(3, 0.0f, 0.0f, 1.0f,          0.0f);
#else
        // this matrix maps the z values into the range [-1,1]
        result.SetRow(0, -cotX, 0.0f, 0.0f,                     0.0f);
        result.SetRow(1, 0.0f, cotY, 0.0f,                      0.0f);
        result.SetRow(2, 0.0f, 0.0f, (farDist + nearDist) * invfn,  -2.0f * farDist * nearDist * invfn);
        result.SetRow(3, 0.0f, 0.0f, 1.0f,                      0.0f);
#endif
        return result;
    }

    // Set the projection matrix with a volume offset
    Matrix4x4 Matrix4x4::CreateProjectionOffset(float left, float right, float bottom, float top, float nearDist, float farDist)
    {
        AZ_MATH_ASSERT(nearDist > 0.0f, "Near plane distance must be greater than 0");
        AZ_MATH_ASSERT(farDist > nearDist, "Far plane distance must be greater than near plane distance");

        // construct projection matrix
        Matrix4x4 result;
        float invfn = 1.0f / (farDist - nearDist);
#if defined(RENDER_WINNT_DIRECTX)
        // this matrix maps the z values into the range [0,1]
        result.SetRow(0, -2.0f * nearDist / (right - left),   0.0f, (left + right) / (left - right),  0.0f);
        result.SetRow(1, 0.0f, 2.0f * nearDist / (top - bottom), (top + bottom) / (bottom - top),  0.0f);
        result.SetRow(2, 0.0f, 0.0f, farDist * invfn, -farDist * nearDist * invfn);
        result.SetRow(3, 0.0f, 0.0f, 1.0f, 0.0f);
#else
        // this matrix maps the z values into the range [-1,1]
        result.SetRow(0, -2.0f * nearDist / (right - left),   0.0f, (left + right) / (left - right),  0.0f);
        result.SetRow(1, 0.0f, 2.0f * nearDist / (top - bottom), (top + bottom) / (bottom - top),  0.0f);
        result.SetRow(2, 0.0f, 0.0f, (farDist + nearDist) * invfn,   -2.0f * farDist * nearDist * invfn);
        result.SetRow(3, 0.0f, 0.0f, 1.0f, 0.0f);
#endif
        return result;
    }

    Matrix4x4 Matrix4x4::GetInverseTransform() const
    {
        Matrix4x4 out;

        AZ_MATH_ASSERT((*this)(3, 0) == 0.0f && (*this)(3, 1) == 0.0f && (*this)(3, 2) == 0.0f && (*this)(3, 3) == 1.0f, "For this matrix you should use GetInverseFull");

        // compute matrix of cofactors
        out.SetElement(0, 0, ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1)));
        out.SetElement(1, 0, ((*this)(1, 2) * (*this)(2, 0) - (*this)(1, 0) * (*this)(2, 2)));
        out.SetElement(2, 0, ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0)));

        out.SetElement(0, 1, ((*this)(2, 1) * (*this)(0, 2) - (*this)(2, 2) * (*this)(0, 1)));
        out.SetElement(1, 1, ((*this)(2, 2) * (*this)(0, 0) - (*this)(2, 0) * (*this)(0, 2)));
        out.SetElement(2, 1, ((*this)(2, 0) * (*this)(0, 1) - (*this)(2, 1) * (*this)(0, 0)));

        out.SetElement(0, 2, ((*this)(0, 1) * (*this)(1, 2) - (*this)(0, 2) * (*this)(1, 1)));
        out.SetElement(1, 2, ((*this)(0, 2) * (*this)(1, 0) - (*this)(0, 0) * (*this)(1, 2)));
        out.SetElement(2, 2, ((*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0)));

        // Calculate the determinant
        float det = (*this)(0, 0) * out(0, 0) + (*this)(1, 0) * out(0, 1) + (*this)(2, 0) * out(0, 2);

        // Run singularity test
        if (det == 0)
        {
            return CreateIdentity();
        }

        // Divide cofactors by determinant
        float f = 1.0f / det;

        out.SetRow(0, out.GetRow(0) * f);
        out.SetRow(1, out.GetRow(1) * f);
        out.SetRow(2, out.GetRow(2) * f);

        // Translation
        out.SetElement(0, 3, -((*this)(0, 3) * out(0, 0) + (*this)(1, 3) * out(0, 1) + (*this)(2, 3) * out(0, 2)));
        out.SetElement(1, 3, -((*this)(0, 3) * out(1, 0) + (*this)(1, 3) * out(1, 1) + (*this)(2, 3) * out(1, 2)));
        out.SetElement(2, 3, -((*this)(0, 3) * out(2, 0) + (*this)(1, 3) * out(2, 1) + (*this)(2, 3) * out(2, 2)));

        // The remaining parts of the 4x4 matrix
        out.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);

        return out;
    }

    Matrix4x4 Matrix4x4::GetInverseFull() const
    {
        Matrix4x4 out;

        // Inverse = adjoint / det.
        // pre-compute 2x2 dets for last two rows when computing
        // cofactors of first two rows.
        float d12 = ((*this)(2, 0) * (*this)(3, 1) - (*this)(3, 0) * (*this)(2, 1));
        float d13 = ((*this)(2, 0) * (*this)(3, 2) - (*this)(3, 0) * (*this)(2, 2));
        float d23 = ((*this)(2, 1) * (*this)(3, 2) - (*this)(3, 1) * (*this)(2, 2));
        float d24 = ((*this)(2, 1) * (*this)(3, 3) - (*this)(3, 1) * (*this)(2, 3));
        float d34 = ((*this)(2, 2) * (*this)(3, 3) - (*this)(3, 2) * (*this)(2, 3));
        float d41 = ((*this)(2, 3) * (*this)(3, 0) - (*this)(3, 3) * (*this)(2, 0));

        out.SetElement(0, 0,  ((*this)(1, 1) * d34 - (*this)(1, 2) * d24 + (*this)(1, 3) * d23));
        out.SetElement(1, 0, -((*this)(1, 0) * d34 + (*this)(1, 2) * d41 + (*this)(1, 3) * d13));
        out.SetElement(2, 0,  ((*this)(1, 0) * d24 + (*this)(1, 1) * d41 + (*this)(1, 3) * d12));
        out.SetElement(3, 0, -((*this)(1, 0) * d23 - (*this)(1, 1) * d13 + (*this)(1, 2) * d12));

        // Compute determinant as early as possible using these cofactors.
        float det = (*this)(0, 0) * out(0, 0) + (*this)(0, 1) * out(1, 0) + (*this)(0, 2) * out(2, 0) + (*this)(0, 3) * out(3, 0);

        // Run singularity test.
        if (det == 0.0f)
        {
            out = CreateIdentity();
        }
        else
        {
            float invDet = 1.0f / det;

            // Compute rest of inverse.
            out.SetElement(0, 0, out(0, 0) * invDet);
            out.SetElement(1, 0, out(1, 0) * invDet);
            out.SetElement(2, 0, out(2, 0) * invDet);
            out.SetElement(3, 0, out(3, 0) * invDet);

            out.SetElement(0, 1, -((*this)(0, 1) * d34 - (*this)(0, 2) * d24 + (*this)(0, 3) * d23) * invDet);
            out.SetElement(1, 1,  ((*this)(0, 0) * d34 + (*this)(0, 2) * d41 + (*this)(0, 3) * d13) * invDet);
            out.SetElement(2, 1, -((*this)(0, 0) * d24 + (*this)(0, 1) * d41 + (*this)(0, 3) * d12) * invDet);
            out.SetElement(3, 1,  ((*this)(0, 0) * d23 - (*this)(0, 1) * d13 + (*this)(0, 2) * d12) * invDet);

            // Pre-compute 2x2 dets for first two rows when computing cofactors of last two rows.
            d12 = (*this)(0, 0) * (*this)(1, 1) - (*this)(1, 0) * (*this)(0, 1);
            d13 = (*this)(0, 0) * (*this)(1, 2) - (*this)(1, 0) * (*this)(0, 2);
            d23 = (*this)(0, 1) * (*this)(1, 2) - (*this)(1, 1) * (*this)(0, 2);
            d24 = (*this)(0, 1) * (*this)(1, 3) - (*this)(1, 1) * (*this)(0, 3);
            d34 = (*this)(0, 2) * (*this)(1, 3) - (*this)(1, 2) * (*this)(0, 3);
            d41 = (*this)(0, 3) * (*this)(1, 0) - (*this)(1, 3) * (*this)(0, 0);

            out.SetElement(0, 2,  ((*this)(3, 1) * d34 - (*this)(3, 2) * d24 + (*this)(3, 3) * d23) * invDet);
            out.SetElement(1, 2, -((*this)(3, 0) * d34 + (*this)(3, 2) * d41 + (*this)(3, 3) * d13) * invDet);
            out.SetElement(2, 2,  ((*this)(3, 0) * d24 + (*this)(3, 1) * d41 + (*this)(3, 3) * d12) * invDet);
            out.SetElement(3, 2, -((*this)(3, 0) * d23 - (*this)(3, 1) * d13 + (*this)(3, 2) * d12) * invDet);
            out.SetElement(0, 3, -((*this)(2, 1) * d34 - (*this)(2, 2) * d24 + (*this)(2, 3) * d23) * invDet);
            out.SetElement(1, 3,  ((*this)(2, 0) * d34 + (*this)(2, 2) * d41 + (*this)(2, 3) * d13) * invDet);
            out.SetElement(2, 3, -((*this)(2, 0) * d24 + (*this)(2, 1) * d41 + (*this)(2, 3) * d12) * invDet);
            out.SetElement(3, 3,  ((*this)(2, 0) * d23 - (*this)(2, 1) * d13 + (*this)(2, 2) * d12) * invDet);
        }

        return out;
    }

    Matrix4x4 Matrix4x4::CreateInterpolated(const Matrix4x4& m1, const Matrix4x4& m2, float t)
    {
        Matrix3x3 m1Copy = Matrix3x3::CreateFromMatrix4x4(m1);
        Vector3 s1 = m1Copy.ExtractScale();
        Vector3 t1 = m1.GetTranslation();
        Quaternion q1 = Quaternion::CreateFromMatrix3x3(m1Copy);

        Matrix3x3 m2Copy = Matrix3x3::CreateFromMatrix4x4(m2);
        Vector3 s2 = m2Copy.ExtractScale();
        Vector3 t2 = m2.GetTranslation();
        Quaternion q2 = Quaternion::CreateFromMatrix3x3(m2Copy);

        s1 = s1.Lerp(s2, t);
        t1 = t1.Lerp(t2, t);
        q1 = q1.Slerp(q2, t);
        q1.Normalize();
        Matrix4x4 result = Matrix4x4::CreateFromQuaternionAndTranslation(q1, t1);
        result.MultiplyByScale(s1);
        return result;
    }
}
