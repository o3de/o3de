/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/MathScriptHelpers.h>
#include <AzCore/Serialization/Locale.h>

namespace AZ
{
    namespace Internal
    {
        Vector2 ConstructVector2(float xValue, float yValue)
        {
            return Vector2(xValue, yValue);
        }


        void Vector2ScriptConstructor(Vector2* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 0:
            {
                *thisPtr = Vector2::CreateZero();
            } break;
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number = 0;
                    dc.ReadArg(0, number);
                    *thisPtr = Vector2(number);
                }
                else if (!(ConstructOnTypedArgument<Vector4>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Vector2>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Vector3>(*thisPtr, dc, 0)))
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When only providing 1 argument to Vector2(), it must be a number or a Vector2,3,4");
                }
            } break;
            case 2:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1))
                {
                    float x = 0;
                    float y = 0;
                    dc.ReadArg(0, x);
                    dc.ReadArg(1, y);
                    *thisPtr = Vector2(x, y);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 2 arguments to Vector2(), all must be numbers!");
                }
            } break;
            default:
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Vector2() accepts only 1 or 2 arguments, not %d!", numArgs);
            } break;
            }
        }

        void Vector2DefaultConstructor(Vector2* thisPtr)
        {
            new (thisPtr) Vector2(Vector2::CreateZero());
        }


        AZStd::string Vector2ToString(const Vector2* thisPtr)
        {
            AZ::Locale::ScopedSerializationLocale locale;

            return AZStd::string::format("(x=%.7f,y=%.7f)", static_cast<float>(thisPtr->GetX()), static_cast<float>(thisPtr->GetY()));
        }


        void Vector2MultiplyGeneric(const Vector2* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector2 result = *thisPtr * scalar;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector2>(0))
                {
                    Vector2 vector2 = Vector2::CreateZero();
                    dc.ReadArg(0, vector2);
                    Vector2 result = *thisPtr * vector2;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector2 multiply should have only 1 argument - Vector2 or a Float/Number!");
                dc.PushResult(Vector2::CreateZero());
            }
        }


        void Vector2DivideGeneric(const Vector2* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Vector2 result = *thisPtr / scalar;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Vector2>(0))
                {
                    Vector2 vector2 = Vector2::CreateZero();
                    dc.ReadArg(0, vector2);
                    Vector2 result = *thisPtr / vector2;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Vector2 divide should have only 1 argument - Vector2 or a Float/Number!");
                dc.PushResult(Vector2::CreateZero());
            }
        }


        void Vector2SetGeneric(Vector2* thisPtr, ScriptDataContext& dc)
        {
            bool valueWasSet = false;
            if (dc.GetNumArguments() == 1 && dc.IsNumber(0))
            {
                float setValue = 0;
                dc.ReadArg(0, setValue);
                thisPtr->Set(setValue);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 2 && dc.IsNumber(0) && dc.IsNumber(1))
            {
                float x = 0;
                float y = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                thisPtr->Set(x, y);
                valueWasSet = true;
            }

            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Vector2.Set only supports Set(number,number), Set(number)");
            }
        }
    }

    void Vector2::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Vector2>()->
                Serializer<FloatBasedContainerSerializer<Vector2, &Vector2::CreateFromFloat2, &Vector2::StoreToFloat2, &GetTransformEpsilon, 2> >();
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Vector2>()->
                Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Common)->
                Attribute(Script::Attributes::Module, "math")->
                Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Constructor<float>()->
                Constructor<float, float>()->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Attribute(Script::Attributes::ConstructorOverride, &Internal::Vector2ScriptConstructor)->
                Attribute(Script::Attributes::GenericConstructorOverride, &Internal::Vector2DefaultConstructor)->
                Property("x", &Vector2::GetX, &Vector2::SetX)->
                Property("y", &Vector2::GetY, &Vector2::SetY)->
                Method<Vector2(Vector2::*)() const>("Unary", &Vector2::operator-)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Unary)->
                Method("ToString",&Internal::Vector2ToString)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::ToString)->
                Method<Vector2(Vector2::*)(float) const>("MultiplyFloat",&Vector2::operator*)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector2MultiplyGeneric)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Mul)->
                Method<Vector2(Vector2::*)(const Vector2&) const>("MultiplyVector2",&Vector2::operator*)->
                    Attribute(Script::Attributes::Ignore,0)-> // ignore for script since we already got the generic multiply above
                Method<Vector2(Vector2::*)(float) const>("DivideFloat",&Vector2::operator/)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector2DivideGeneric)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Div)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method<Vector2(Vector2::*)(const Vector2&) const>("DivideVector2",&Vector2::operator/)->
                    Attribute(Script::Attributes::Ignore,0)-> // ignore for script since we already got the generic divide above
                Method("Equal",&Vector2::operator==)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Equal)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("LessThan",&Vector2::IsLessThan)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::LessThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("LessEqualThan",&Vector2::IsLessEqualThan)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::LessEqualThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Add",&Vector2::operator+)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Add)->
                Method<Vector2(Vector2::*)(const Vector2&) const>("Subtract",&Vector2::operator-)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Sub)->
                Method<void (Vector2::*)(float,float)>("Set", &Vector2::Set)->
                    Attribute(Script::Attributes::MethodOverride, &Internal::Vector2SetGeneric)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Clone", [](const Vector2& rhs) -> Vector2 { return rhs; }, {{{"Vector2",""}}})->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetElement", &Vector2::GetElement)->
                Method("SetElement", &Vector2::SetElement, { { {"Index", ""}, {"Value",""} } })->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetLength", &Vector2::GetLength, { "Source", "The source of the magnitude calculation." }, {})->
                    Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Length", "Math"))->
                Method("GetLengthSq", &Vector2::GetLengthSq)->
                Method("SetLength", &Vector2::SetLength)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetNormalized", &Vector2::GetNormalized)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetNormalizedSafe", &Vector2::GetNormalizedSafe)->
                Method("Normalize", &Vector2::Normalize)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("NormalizeWithLength", &Vector2::NormalizeWithLength)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("NormalizeSafe", &Vector2::NormalizeSafe, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("NormalizeSafeWithLength", &Vector2::NormalizeSafeWithLength, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsNormalized", &Vector2::IsNormalized, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("GetDistance", &Vector2::GetDistance)->
                Method("GetDistanceSq", &Vector2::GetDistanceSq)->
                Method("Angle", &Vector2::Angle)->
                Method("AngleDeg", &Vector2::AngleDeg)->
                Method("AngleSafe", &Vector2::AngleSafe)->
                Method("AngleSafeDeg", &Vector2::AngleSafeDeg)->
                Method("Lerp", &Vector2::Lerp)->
                Method("Slerp", &Vector2::Slerp)->
                Method("Dot", &Vector2::Dot)->
                Method("GetPerpendicular", &Vector2::GetPerpendicular)->
                Method("IsClose", &Vector2::IsClose, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("IsZero", &Vector2::IsZero, behaviorContext->MakeDefaultValues(Constants::FloatEpsilon))->
                Method("IsLessThan", &Vector2::IsLessThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsLessEqualThan", &Vector2::IsLessEqualThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsGreaterThan", &Vector2::IsGreaterThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsGreaterEqualThan", &Vector2::IsGreaterEqualThan)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetMin", &Vector2::GetMin)->
                Method("GetMax", &Vector2::GetMax)->
                Method("GetClamp", &Vector2::GetClamp)->
                Method("GetAbs", &Vector2::GetAbs)->
                Method("Project", &Vector2::Project)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("ProjectOnNormal", &Vector2::ProjectOnNormal)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetProjected", &Vector2::GetProjected)->
                Method("GetProjectedOnNormal", &Vector2::GetProjectedOnNormal)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("IsFinite", &Vector2::IsFinite)->
                Method("CreateAxisX", &Vector2::CreateAxisX, behaviorContext->MakeDefaultValues(1.0f))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateAxisY", &Vector2::CreateAxisY, behaviorContext->MakeDefaultValues(1.0f))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromAngle", &Vector2::CreateFromAngle, behaviorContext->MakeDefaultValues(0.0f))->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateOne", &Vector2::CreateOne)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateZero", &Vector2::CreateZero)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateSelectCmpEqual", &Vector2::CreateSelectCmpEqual)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateSelectCmpGreaterEqual", &Vector2::CreateSelectCmpGreaterEqual)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("CreateSelectCmpGreater", &Vector2::CreateSelectCmpGreater)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetSelect", &Vector2::GetSelect)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Select", &Vector2::Select)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("GetMadd", &Vector2::GetMadd)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("Madd", &Vector2::Madd)->
                    Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::All)->
                Method("ConstructFromValues", &Internal::ConstructVector2)->
                Method<Vector2(Vector2::*)(float) const>("DivideFloatExplicit", &Vector2::operator/, { "Source", "The source value gets divided." }, { { { "Divisor", "The value that divides Source." } } })->
                    Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Divide By Number (/)", "Math"))->
                    Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "DivideGroup", "" }, { "DivideGroup" }))
                ;
        }
    }
}
