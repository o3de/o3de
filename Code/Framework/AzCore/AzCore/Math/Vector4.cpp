/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Color.h>
#include <AzCore/Math/MathScriptHelpers.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector4.h>

namespace AZ
{
    namespace Internal
    {
        Vector4 ConstructVector4(float xValue, float yValue, float zValue, float wValue)
        {
            return Vector4(xValue, yValue, zValue, wValue);
        }

        void Vector4ScriptConstructor(Vector4* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 0:
            {
                *thisPtr = Vector4::CreateZero();
            }break;
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number = 0;
                    dc.ReadArg(0, number);
                    *thisPtr = Vector4(number);
                }
                else if (!(ConstructOnTypedArgument<Vector4>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Vector2>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Vector3>(*thisPtr, dc, 0)))
                {
                    if (dc.IsClass<AZ::Color>(0))
                    {
                        AZ::Color argument;
                        dc.ReadArg(0, argument);
                        *thisPtr = argument.GetAsVector4();
                    }
                    else
                    {
                        dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When only providing 1 argument to Vector4(), it must be a number, Vector2,3,4, or Color");
                    }
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
                    *thisPtr = Vector4(x, y, z, w);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 4 arguments to Vector4(), all must be numbers!");
                }
            } break;
            default:
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Vector4() accepts only 1 or 4 arguments, not %d!", numArgs);
            } break;
            }
        }

        void Vector4DefaultConstructor(Vector4* thisPtr)
        {
            new (thisPtr) Vector4(Vector4::CreateZero());
        }

        void Vector4MultiplyGeneric(const Vector4* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector4 result = *thisPtr * scalar;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector4>(0))
                {
                    Vector4 vector4 = Vector4::CreateZero();
                    dc.ReadArg(0, vector4);
                    Vector4 result = *thisPtr * vector4;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Matrix4x4>(0))
                {
                    Matrix4x4 matrix4x4 = Matrix4x4::CreateZero();
                    dc.ReadArg(0, matrix4x4);
                    Vector4 result = *thisPtr * matrix4x4;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector4 multiply should have only 1 argument - Vector4, Matrix4x4, Transform, or a Float/Number!");
                dc.PushResult(Vector4::CreateZero());
            }
        }

        void Vector4DivideGeneric(const Vector4* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector4 result = *thisPtr / scalar;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector4>(0))
                {
                    Vector4 vector4 = Vector4::CreateZero();
                    dc.ReadArg(0, vector4);
                    Vector4 result = *thisPtr / vector4;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector4 divide should have only 1 argument - Vector4 or a Float/Number!");
                dc.PushResult(Vector4::CreateZero());
            }
        }


        void Vector4SetGeneric(Vector4* thisPtr, ScriptDataContext& dc)
        {
            bool valueWasSet = false;

            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float setValue = 0;
                    dc.ReadArg(0, setValue);
                    thisPtr->Set(setValue);
                    valueWasSet = true;
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 setValue = Vector3::CreateZero();
                    dc.ReadArg(0, setValue);
                    thisPtr->Set(setValue);
                    valueWasSet = true;
                }
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
                AZ_Error("Script", false, "Vector4.Set only supports Set(number,number,number,number), Set(number), Set(Vector3), Set(Vector3,float)");
            }
        }
    }


    void Vector4::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Vector4>()->
                Serializer<FloatBasedContainerSerializer<Vector4, &Vector4::CreateFromFloat4, &Vector4::StoreToFloat4, &GetTransformEpsilon, 4> >();
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Vector4>()->
                Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Common)->
                Attribute(Script::Attributes::Module, "math")->
                Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Constructor<float>()->
                Constructor<float, float, float, float>()->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Attribute(Script::Attributes::ConstructorOverride, &Internal::Vector4ScriptConstructor)->
                Attribute(Script::Attributes::GenericConstructorOverride, &Internal::Vector4DefaultConstructor)->
                Property("x", &Vector4::GetX, &Vector4::SetX)->
                Property("y", &Vector4::GetY, &Vector4::SetY)->
                Property("z", &Vector4::GetZ, &Vector4::SetZ)->
                Property("w", &Vector4::GetW, &Vector4::SetW)->
                Method<Vector4(Vector4::*)() const>("Unary", &Vector4::operator-)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Unary)->
                Method("ToString", &Vector4ToString)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::ToString)->
                Method<Vector4(Vector4::*)(float) const>("MultiplyFloat", &Vector4::operator*)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector4MultiplyGeneric)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Mul)->
                Method<Vector4(Vector4::*)(const Vector4&) const>("MultiplyVector4", &Vector4::operator*)->
                    Attribute(Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                Method<Vector4(Vector4::*)(float) const>("DivideFloat", &Vector4::operator/)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector4DivideGeneric)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Div)->
                Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method<Vector4(Vector4::*)(const Vector4&) const>("DivideVector4", &Vector4::operator/)->
                    Attribute(Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic divide above
                Method("Clone", [](const Vector4& rhs) -> Vector4 { return rhs; })->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Equal", &Vector4::operator==)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Equal)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("LessThan", &Vector4::IsLessThan)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::LessThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("LessEqualThan", &Vector4::IsLessEqualThan)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::LessEqualThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Add", &Vector4::operator+)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Add)->
                Method<Vector4(Vector4::*)(const Vector4&) const>("Subtract", &Vector4::operator-)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Sub)->
                Method<void (Vector4::*)(float, float, float, float)>("Set", &Vector4::Set)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector4SetGeneric)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetElement", &Vector4::GetElement)->
                Method("SetElement", &Vector4::SetElement)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetLength", &Vector4::GetLength)->
                    Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Length", "Math"))->
                Method("GetLengthSq", &Vector4::GetLengthSq)->
                Method("GetLengthReciprocal", &Vector4::GetLengthReciprocal)->
                Method("GetNormalized", &Vector4::GetNormalized)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Normalize", &Vector4::Normalize)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("NormalizeWithLength", &Vector4::NormalizeWithLength)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetNormalizedSafe", &Vector4::GetNormalizedSafe, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("NormalizeSafe", &Vector4::NormalizeSafe, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("NormalizeSafeWithLength", &Vector4::NormalizeSafeWithLength, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsNormalized", &Vector4::IsNormalized, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("Dot", &Vector4::Dot)->
                Method("Dot3", &Vector4::Dot3)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Homogenize", &Vector4::Homogenize)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetHomogenized", &Vector4::GetHomogenized)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsClose", &Vector4::IsClose, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("IsZero", &Vector4::IsZero, behaviorContext->MakeDefaultValues(Constants::FloatEpsilon))->
                Method("IsLessThan", &Vector4::IsLessThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsLessEqualThan", &Vector4::IsLessEqualThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsGreaterThan", &Vector4::IsGreaterThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsGreaterEqualThan", &Vector4::IsGreaterEqualThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetAbs", &Vector4::GetAbs)->
                Method("GetReciprocal", &Vector4::GetReciprocal)->
                Method("IsFinite", &Vector4::IsFinite)->
                Method("CreateAxisX", &Vector4::CreateAxisX, behaviorContext->MakeDefaultValues(1.0f))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateAxisY", &Vector4::CreateAxisY, behaviorContext->MakeDefaultValues(1.0f))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateAxisZ", &Vector4::CreateAxisZ, behaviorContext->MakeDefaultValues(1.0f))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateAxisW", &Vector4::CreateAxisW, behaviorContext->MakeDefaultValues(1.0f))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromVector3", &Vector4::CreateFromVector3)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromVector3AndFloat", &Vector4::CreateFromVector3AndFloat)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateOne", &Vector4::CreateOne)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateZero", &Vector4::CreateZero)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("ConstructFromValues", &Internal::ConstructVector4)->
                Method<Vector4(Vector4::*)(float) const>("DivideFloatExplicit", &Vector4::operator/)->
                    Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Divide By Number (/)", "Math"))->
                    Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "DivideGroup", "" }, { "DivideGroup" }))
                ;
        }
    }
}
