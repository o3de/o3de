/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Color.h>
#include <AzCore/Math/MathScriptHelpers.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    namespace ScriptCanvas
    {
        Vector3 ConstructVector3(float xValue, float yValue, float zValue)
        {
            return Vector3(xValue, yValue, zValue);
        }
    }

    namespace Internal
    {
        void Vector3ScriptConstructor(Vector3* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 0:
            {
                *thisPtr = Vector3::CreateZero();
            } break;
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number = 0;
                    dc.ReadArg(0, number);
                    *thisPtr = Vector3(number);
                }
                else if (!(ConstructOnTypedArgument<Vector4>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Vector2>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Vector3>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Color>(*thisPtr, dc, 0)))
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When only providing 1 argument to Vector3(), it must be a number, Vector2,3,4 or Color");
                }
            } break;
            case 3:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2))
                {
                    float x = 0;
                    float y = 0;
                    float z = 0;
                    dc.ReadArg(0, x);
                    dc.ReadArg(1, y);
                    dc.ReadArg(2, z);
                    *thisPtr = Vector3(x, y, z);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 3 arguments to Vector3(), all must be numbers!");
                }
            } break;
            default:
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Vector3() accepts only 1 or 3 arguments, not %d!", numArgs);
            } break;
            }
        }

        void Vector3DefaultConstructor(Vector3* thisPtr)
        {
            new (thisPtr) Vector3(Vector3::CreateZero());
        }

        void Vector3MultiplyGeneric(const Vector3* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector3 result = *thisPtr * scalar;
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
                AZ_Error("Script", false, "Vector3 multiply should have only 1 argument - Vector3, Float/Number, Matrix3x3, Matrix4x4, or Transform!");
                dc.PushResult(Vector3::CreateZero());
            }
        }

        void Vector3SetGeneric(Vector3* thisPtr, ScriptDataContext& dc)
        {
            bool valueWasSet = false;

            if (dc.GetNumArguments() == 1 && dc.IsNumber(0))
            {
                float setValue = 0;
                dc.ReadArg(0, setValue);
                thisPtr->Set(setValue);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 3 && dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                thisPtr->Set(x, y, z);
                valueWasSet = true;
            }

            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Vector3.Set only supports Set(number,number,number), Set(number)");
            }
        }

        void Vector3DivideGeneric(const Vector3* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector3 result = *thisPtr / scalar;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 vector3 = Vector3::CreateZero();
                    dc.ReadArg(0, vector3);
                    Vector3 result = *thisPtr / vector3;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector3 divide should have only 1 argument - Vector3 or a Float/Number!");
                dc.PushResult(Vector3::CreateZero());
            }
        }

        void Vector3GetSinCosMultipleReturn(const Vector3* thisPtr, ScriptDataContext& dc)
        {
            Vector3 sin, cos;
            thisPtr->GetSinCos(sin, cos);
            dc.PushResult(sin);
            dc.PushResult(cos);
        }

        /// Implements multiple return values for BuildTangetBasis in Lua
        void Vector3BuildTangentBasis(const Vector3* thisPtr, ScriptDataContext& dc)
        {
            Vector3 v1, v2;
            thisPtr->BuildTangentBasis(v1, v2);
            dc.PushResult(v1);
            dc.PushResult(v2);
        }
    }

    Vector3::Vector3(const Vector2& source)
    {
        Set(source.GetX(), source.GetY(), 0.0f);
    }

    Vector3::Vector3(const Vector4& source)
    {
        *this = source.GetAsVector3();
    }

    void Vector3::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Vector3>()->
                Serializer<FloatBasedContainerSerializer<Vector3, &Vector3::CreateFromFloat3, &Vector3::StoreToFloat3, &GetTransformEpsilon, 3> >();
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Vector3>()->
                Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Common)->
                Attribute(Script::Attributes::Module, "math")->
                Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Constructor<float>()->
                Constructor<float, float, float>()->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Attribute(Script::Attributes::ConstructorOverride, &Internal::Vector3ScriptConstructor)->
                Attribute(Script::Attributes::GenericConstructorOverride, &Internal::Vector3DefaultConstructor)->
                Property("x", &Vector3::GetX, &Vector3::SetX)->
                Property("y", &Vector3::GetY, &Vector3::SetY)->
                Property("z", &Vector3::GetZ, &Vector3::SetZ)->
                Method<Vector3(Vector3::*)() const>("Unary", &Vector3::operator-)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Unary)->
                Method("ToString",&Vector3ToString)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::ToString)->
                Method<Vector3(Vector3::*)(float) const>("MultiplyFloat",&Vector3::operator*)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector3MultiplyGeneric)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Mul)->
                Method<Vector3(Vector3::*)(const Vector3&) const>("MultiplyVector3",&Vector3::operator*)->
                    Attribute(Script::Attributes::Ignore,0)-> // ignore for script since we already got the generic multiply above
                Method<Vector3(Vector3::*)(float) const>("DivideFloat",&Vector3::operator/)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector3DivideGeneric)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Div)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method<Vector3(Vector3::*)(const Vector3&) const>("DivideVector3",&Vector3::operator/)->
                    Attribute(Script::Attributes::Ignore,0)-> // ignore for script since we already got the generic divide above
                Method("Clone", [](const Vector3& rhs) -> Vector3 { return rhs; })->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Equal",&Vector3::operator==)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Equal)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("LessThan",&Vector3::IsLessThan)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::LessThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("LessEqualThan",&Vector3::IsLessEqualThan)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::LessEqualThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Add",&Vector3::operator+)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Add)->
                Method<Vector3(Vector3::*)(const Vector3&) const>("Subtract",&Vector3::operator-)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Sub)->
                Method<void (Vector3::*)(float, float, float)>("Set", &Vector3::Set)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector3SetGeneric)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetElement", &Vector3::GetElement)->
                Method("SetElement", &Vector3::SetElement)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetLength", &Vector3::GetLength)->
                    Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Length", "Math"))->
                Method("GetLengthSq", &Vector3::GetLengthSq)->
                Method("GetLengthReciprocal", &Vector3::GetLengthReciprocal)->
                Method("GetNormalized", &Vector3::GetNormalized)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Normalize", &Vector3::Normalize)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("NormalizeWithLength", &Vector3::NormalizeWithLength)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetNormalizedSafe", &Vector3::GetNormalizedSafe, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("NormalizeSafe", &Vector3::NormalizeSafe, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("NormalizeSafeWithLength", &Vector3::NormalizeSafeWithLength, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsNormalized", &Vector3::IsNormalized, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("SetLength", &Vector3::SetLength)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("SetLengthEstimate", &Vector3::SetLengthEstimate)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetDistance", &Vector3::GetDistance)->
                Method("GetDistanceSq", &Vector3::GetDistanceSq)->
                Method("Angle", &Vector3::Angle)->
                Method("AngleDeg", &Vector3::AngleDeg)->
                Method("AngleSafe", &Vector3::AngleSafe)->
                Method("AngleSafeDeg", &Vector3::AngleSafeDeg)->
                Method("Lerp", &Vector3::Lerp)->
                Method("Slerp", &Vector3::Slerp)->
                Method("Dot", &Vector3::Dot)->
                Method("Cross", &Vector3::Cross)->
                Method("CrossXAxis", &Vector3::CrossXAxis)->
                Method("CrossYAxis", &Vector3::CrossYAxis)->
                Method("CrossZAxis", &Vector3::CrossZAxis)->
                Method("XAxisCross", &Vector3::XAxisCross)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("YAxisCross", &Vector3::YAxisCross)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("ZAxisCross", &Vector3::ZAxisCross)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsClose", &Vector3::IsClose, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("IsZero", &Vector3::IsZero, behaviorContext->MakeDefaultValues(Constants::FloatEpsilon))->
                Method("IsLessThan", &Vector3::IsLessThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsLessEqualThan", &Vector3::IsLessEqualThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsGreaterThan", &Vector3::IsGreaterThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsGreaterEqualThan", &Vector3::IsGreaterEqualThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetMin", &Vector3::GetMin)->
                Method("GetMax", &Vector3::GetMax)->
                Method("GetClamp", &Vector3::GetClamp)->
                Method("GetSin", &Vector3::GetSin)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetCos", &Vector3::GetCos)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetSinCos", &Vector3::GetSinCos)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector3GetSinCosMultipleReturn)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetAngleMod", &Vector3::GetAngleMod)->
                Method("GetAbs", &Vector3::GetAbs)->
                Method("GetReciprocal", &Vector3::GetReciprocal)->
                Method("BuildTangentBasis", &Vector3::BuildTangentBasis)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector3BuildTangentBasis)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetMadd", &Vector3::GetMadd)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Madd", &Vector3::Madd)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsPerpendicular", &Vector3::IsPerpendicular, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("Project", &Vector3::Project)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("ProjectOnNormal", &Vector3::ProjectOnNormal)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetProjected", &Vector3::GetProjected)->
                Method("GetProjectedOnNormal", &Vector3::GetProjectedOnNormal)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsFinite", &Vector3::IsFinite)->
                Method("CreateAxisX", &Vector3::CreateAxisX, behaviorContext->MakeDefaultValues(1.0f))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateAxisY", &Vector3::CreateAxisY, behaviorContext->MakeDefaultValues(1.0f))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateAxisZ", &Vector3::CreateAxisZ, behaviorContext->MakeDefaultValues(1.0f))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateOne", &Vector3::CreateOne)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateZero", &Vector3::CreateZero)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("ConstructFromValues", &ScriptCanvas::ConstructVector3)->
                Method<Vector3(Vector3::*)(float) const>("DivideFloatExplicit", &Vector3::operator/)->
                    Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Divide By Number (/)", "Math"))->
                    Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "DivideGroup", "" }, { "DivideGroup" }))
                ;
        }
    }


    void Vector3::BuildTangentBasis(Vector3& tangent, Vector3& bitangent) const
    {
        AZ_MATH_ASSERT(IsNormalized(), "BuildTangentBasis needs normalized vectors");

        if (Abs(GetX()) < 0.9f)
        {
            tangent = CrossXAxis();
        }
        else
        {
            tangent = CrossYAxis();
        }
        tangent.Normalize();
        bitangent = Cross(tangent);
        bitangent.Normalize();
    }
}
