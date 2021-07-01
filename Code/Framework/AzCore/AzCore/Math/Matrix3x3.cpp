/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/MathScriptHelpers.h>

namespace AZ
{
    namespace Internal
    {
        Matrix3x3 ConstructMatrix3x3(Vector3 row0, Vector3 row1, Vector3 row2)
        {
            Matrix3x3 result;
            result.SetRows(row0, row1, row2);
            return result;
        }


        AZ::Matrix3x3 ConstructMatrix3x3FromValues
            ( float v00, float v01, float v02
            , float v10, float v11, float v12
            , float v20, float v21, float v22)
        {
            return ConstructMatrix3x3(AZ::Vector3(v00, v01, v02), AZ::Vector3(v10, v11, v12), AZ::Vector3(v20, v21, v22));
        }


        void Matrix3x3DefaultConstructor(Matrix3x3* thisPtr)
        {
            new (thisPtr) Matrix3x3(Matrix3x3::CreateIdentity());
        }


        void Matrix3x3ScriptConstructor(Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            auto numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 0:
                *thisPtr = AZ::Matrix3x3::CreateIdentity();
                break;
            case 1:
                if (!(ConstructOnTypedArgument<Matrix3x3>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Quaternion>(*thisPtr, dc, 0)))
                {
                    AZ_Warning("Script", false, "Matrix3x3 constructor needs one Matrix3x3, or Quaternion to be constructed");
                }
                break;
            default:
                break;
            }
        }

        void Matrix3x3GetBasisMultipleReturn(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            Vector3 basisX, basisY, basisZ;
            thisPtr->GetBasis(&basisX, &basisY, &basisZ);
            dc.PushResult(basisX);
            dc.PushResult(basisY);
            dc.PushResult(basisZ);
        }


        void Matrix3x3SetRowGeneric(Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            bool rowIsSet = false;
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
                        thisPtr->SetRow(index, Vector3(x, y, z));
                        rowIsSet = true;
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
                        thisPtr->SetRow(index, vector3);
                        rowIsSet = true;
                    }
                }
            }

            if (!rowIsSet)
            {
                AZ_Error("Script", false, "Matrix3x3 SetRow should have two signatures SetRow(index,Vector3) or SetRaw(index,x,y,z)!");
                dc.PushResult(Matrix3x3::CreateIdentity());
            }
        }


        void Matrix3x3SetColumnGeneric(Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            bool rowIsSet = false;
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
                        rowIsSet = true;
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
                        rowIsSet = true;
                    }
                }
            }

            if (!rowIsSet)
            {
                AZ_Error("Script", false, "Matrix3x3 SetColumn should have two signatures SetColumn(index,Vector3) or SetColumn(index,x,y,z)!");
            }
        }


        void Matrix3x3GetRowsMultipleReturn(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            Vector3 row0, row1, row2;
            thisPtr->GetRows(&row0, &row1, &row2);
            dc.PushResult(row0);
            dc.PushResult(row1);
            dc.PushResult(row2);
        }


        void Matrix3x3GetColumnsMultipleReturn(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            Vector3 column0, column1, column2;
            thisPtr->GetColumns(&column0, &column1, &column2);
            dc.PushResult(column0);
            dc.PushResult(column1);
            dc.PushResult(column2);
        }


        void Matrix3x3GetPolarDecompositionMultipleReturn(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            Matrix3x3 orthogonal, symmetric;
            thisPtr->GetPolarDecomposition(&orthogonal, &symmetric);
            dc.PushResult(orthogonal);
            dc.PushResult(symmetric);
        }


        void Matrix3x3MultiplyGeneric(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Matrix3x3 result = *thisPtr * scalar;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Matrix3x3>(0))
                {
                    Matrix3x3 mat3x3 = Matrix3x3::CreateZero();
                    dc.ReadArg(0, mat3x3);
                    Matrix3x3 result = *thisPtr * mat3x3;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(0, vector3);
                    Vector3 result = *thisPtr * vector3;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Matrix3x3 multiply should have only 1 argument - Matrix3x3, Vector3 or a Float/Number!");
                dc.PushResult(Matrix3x3::CreateIdentity());
            }
        }


        void Matrix3x3DivideGeneric(const Matrix3x3* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() != 1 || !dc.IsNumber(0))
            {
                AZ_Error("Script", false, "Matrix3x3 divide should have only 1 argument - a Float/Number!");
                dc.PushResult(Matrix3x3::CreateIdentity());
            }
            float divisor = 0;
            dc.ReadArg(0, divisor);
            dc.PushResult(*thisPtr / divisor);
        }


        AZStd::string Matrix3x3ToString(const Matrix3x3& m)
        {
            return AZStd::string::format("%s,%s,%s", Vector3ToString(m.GetBasisX()).c_str(), Vector3ToString(m.GetBasisY()).c_str(), Vector3ToString(m.GetBasisZ()).c_str());
        }
    }


    void Matrix3x3::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Matrix3x3>()->
                Serializer<FloatBasedContainerSerializer<Matrix3x3, &Matrix3x3::CreateFromColumnMajorFloat9, &Matrix3x3::StoreToColumnMajorFloat9, &GetTransformEpsilon, 9>>();
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Matrix3x3>()->
                Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Common)->
                Attribute(Script::Attributes::Module, "math")->
                Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::Matrix3x3ScriptConstructor)->
                Attribute(Script::Attributes::GenericConstructorOverride, &Internal::Matrix3x3DefaultConstructor)->
                Property<Vector3(Matrix3x3::*)() const, void (Matrix3x3::*)(const Vector3&)>("basisX", &Matrix3x3::GetBasisX, &Matrix3x3::SetBasisX)->
                Property<Vector3(Matrix3x3::*)() const, void (Matrix3x3::*)(const Vector3&)>("basisY", &Matrix3x3::GetBasisY, &Matrix3x3::SetBasisY)->
                Property<Vector3(Matrix3x3::*)() const, void (Matrix3x3::*)(const Vector3&)>("basisZ", &Matrix3x3::GetBasisZ, &Matrix3x3::SetBasisZ)->
                Method<Matrix3x3(Matrix3x3::*)() const>("Unary", &Matrix3x3::operator-)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Unary)->
                Method("ToString", &Internal::Matrix3x3ToString)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::ToString)->
                Method<Matrix3x3(Matrix3x3::*)(float) const>("MultiplyFloat", &Matrix3x3::operator*)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix3x3MultiplyGeneric)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Mul)->
                Method<Vector3(Matrix3x3::*)(const Vector3&) const>("MultiplyVector3", &Matrix3x3::operator*)->            
                    Attribute(Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                Method<Matrix3x3(Matrix3x3::*)(const Matrix3x3&) const>("MultiplyMatrix3x3", &Matrix3x3::operator*)->
                    Attribute(Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                Method<Matrix3x3(Matrix3x3::*)(float) const>("DivideFloat", &Matrix3x3::operator/)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix3x3DivideGeneric)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Div)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Clone", [](const Matrix3x3& rhs) -> Matrix3x3 { return rhs; })->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Equal", &Matrix3x3::operator==)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Equal)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Add", &Matrix3x3::operator+)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Add)->                
                Method<Matrix3x3(Matrix3x3::*)(const Matrix3x3&) const>("Subtract", &Matrix3x3::operator-)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Sub)->                
                Method("GetBasis", &Matrix3x3::GetBasis)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix3x3GetBasisMultipleReturn)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("SetBasis", &Matrix3x3::SetBasis)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetElement", &Matrix3x3::GetElement)->
                Method("SetElement", &Matrix3x3::SetElement)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetRow", &Matrix3x3::GetRow)->
                Method("GetRows", &Matrix3x3::GetRows)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix3x3GetRowsMultipleReturn)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method<void (Matrix3x3::*)(int, const Vector3&)>("SetRow", &Matrix3x3::SetRow)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix3x3SetRowGeneric)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("SetRows", &Matrix3x3::SetRows)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetColumn", &Matrix3x3::GetColumn)->
                Method("GetColumns", &Matrix3x3::GetColumn)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix3x3GetColumnsMultipleReturn)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method<void (Matrix3x3::*)(int, const Vector3&)>("SetColumn", &Matrix3x3::SetColumn)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix3x3SetColumnGeneric)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("SetColumns", &Matrix3x3::SetColumns)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("TransposedMultiply", &Matrix3x3::TransposedMultiply)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetTranspose", &Matrix3x3::GetTranspose)->
                Method("Transpose", &Matrix3x3::Transpose)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetInverseFull", &Matrix3x3::GetInverseFull)->
                Method("InvertFull", &Matrix3x3::InvertFull)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetInverseFast", &Matrix3x3::GetInverseFast)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("InvertFast", &Matrix3x3::InvertFast)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("RetrieveScale", &Matrix3x3::RetrieveScale)->                
                Method("ExtractScale", &Matrix3x3::ExtractScale)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("MultiplyByScale", &Matrix3x3::MultiplyByScale)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method<Matrix3x3(Matrix3x3::*)()const>("GetPolarDecomposition", &Matrix3x3::GetPolarDecomposition)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Matrix3x3GetPolarDecompositionMultipleReturn)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsOrthogonal", &Matrix3x3::IsOrthogonal, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("GetOrthogonalized", &Matrix3x3::GetOrthogonalized)->
                Method("Orthogonalize", &Matrix3x3::Orthogonalize)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsClose", &Matrix3x3::IsClose, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("SetRotationPartFromQuaternion", &Matrix3x3::SetRotationPartFromQuaternion)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetDiagonal", &Matrix3x3::GetDiagonal)->
                Method("GetDeterminant", &Matrix3x3::GetDeterminant)->
                Method("GetAdjugate", &Matrix3x3::GetAdjugate)->
                Method("IsFinite", &Matrix3x3::IsFinite)->
                Method("CreateIdentity", &Matrix3x3::CreateIdentity)->
                Method("CreateZero", &Matrix3x3::CreateZero)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromValue", &Matrix3x3::CreateFromValue)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromRows", &Matrix3x3::CreateFromRows)->
                Method("CreateFromColumns", &Matrix3x3::CreateFromColumns)->
                Method("CreateRotationX", &Matrix3x3::CreateRotationX)->
                Method("CreateRotationY", &Matrix3x3::CreateRotationY)->
                Method("CreateRotationZ", &Matrix3x3::CreateRotationZ)->
                Method("CreateFromTransform", &Matrix3x3::CreateFromTransform)->
                Method("CreateFromMatrix4x4", &Matrix3x3::CreateFromMatrix4x4)->
                Method("CreateFromQuaternion", &Matrix3x3::CreateFromQuaternion)->
                Method("CreateScale", &Matrix3x3::CreateScale)->
                Method("CreateDiagonal", &Matrix3x3::CreateDiagonal)->
                Method("CreateCrossProduct", &Matrix3x3::CreateCrossProduct)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("ConstructFromValues", &Internal::ConstructMatrix3x3)->
                Method("ConstructFromValuesNumeric", &Internal::ConstructMatrix3x3FromValues)
                ;
        }
    }


    void Matrix3x3::SetRotationPartFromQuaternion(const Quaternion& q)
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

        SetElement(0, 0, 1.0f - (tyy + tzz)); // 1.0-2yy-2zz    2xy-2wz    2xz+2wy
        SetElement(0, 1, txy - twz);
        SetElement(0, 2, txz + twy);

        SetElement(1, 0, txy + twz); // 2xy+2wz    1.0-2xx-2zz    2yz-2wx
        SetElement(1, 1, 1.0f - (txx + tzz));
        SetElement(1, 2, tyz - twx);

        SetElement(2, 0, txz - twy); // 2xz-2wy    2yz+2wx    1.0-2xx-2yy
        SetElement(2, 1, tyz + twx);
        SetElement(2, 2, 1.0f - (txx + tyy));
    }


    Matrix3x3 Matrix3x3::GetPolarDecomposition() const
    {
        const float precision = 0.00001f;
        const float epsilon = 0.0000001f;
        const int32_t maxIterations = 16;
        Matrix3x3 u = (*this);// / GetDiagonal().GetLength(); - NR: this was failing, diagonal could be zero and still a valid matrix! Not sure where this came from originally...
        float det = u.GetDeterminant();
        if (det * det > epsilon)
        {
            for (int32_t i = 0; i < maxIterations; ++i)
            {
                //already have determinant, so can use adjugate/det to get inverse
                u = 0.5f * (u + (u.GetAdjugate() / det).GetTranspose());
                float newDet = u.GetDeterminant();
                float diff = newDet - det;
                if (diff * diff < precision)
                {
                    break;
                }
                det = newDet;
            }
            u = u.GetOrthogonalized();
        }
        else
        {
            u = Matrix3x3::CreateIdentity();
        }

        return u;
    }


    bool Matrix3x3::IsOrthogonal(float tolerance) const
    {
        float toleranceSq = tolerance * tolerance;
        if (!AZ::IsClose(GetRow(0).GetLengthSq(), 1.0f, toleranceSq) ||
            !AZ::IsClose(GetRow(1).GetLengthSq(), 1.0f, toleranceSq) ||
            !AZ::IsClose(GetRow(2).GetLengthSq(), 1.0f, toleranceSq))
        {
            return false;
        }

        if (!AZ::IsClose(GetRow(0).Dot(GetRow(1)), 0.0f, tolerance) ||
            !AZ::IsClose(GetRow(0).Dot(GetRow(2)), 0.0f, tolerance) ||
            !AZ::IsClose(GetRow(1).Dot(GetRow(2)), 0.0f, tolerance))
        {
            return false;
        }

        return true;
    }
}
