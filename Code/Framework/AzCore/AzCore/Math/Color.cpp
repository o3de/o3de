/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Color.h"
#include <AzCore/Math/MathScriptHelpers.h>
#include <AzCore/Serialization/Locale.h>

namespace AZ
{
    namespace Internal
    {
        AZ::Color ConstructColor(int32_t red, int32_t blue, int32_t green, int32_t alpha)
        {
            return AZ::Color((red / 255.0f), (blue / 255.0f), (green / 255.0f), (alpha / 255.0f));
        }


        void ColorDefaultConstructor(AZ::Color* thisPtr)
        {
            new (thisPtr) AZ::Color(AZ::Color::CreateZero());
        }


        void ColorScriptConstructor(Color* thisPtr, ScriptDataContext& dc)
        {
            int32_t numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 0:
            {
                *thisPtr = Color(1.0f, 1.0f, 1.0f, 1.0f);
            }
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number = 0;
                    dc.ReadArg(0, number);
                    *thisPtr = Color(number);
                }
                else if (!(ConstructOnTypedArgument<Color>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Vector4>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Vector3>(*thisPtr, dc, 0)
                    || ConstructOnTypedArgument<Vector2>(*thisPtr, dc, 0)))
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When only providing 1 argument to Color(), it must be a number, Color, Vector4, Vector3, or Vector2");
                }
            } break;
            case 3:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2))
                {
                    float r = 0;
                    float g = 0;
                    float b = 0;
                    dc.ReadArg(0, r);
                    dc.ReadArg(1, g);
                    dc.ReadArg(2, b);
                    *thisPtr = Color(r, g, b, 1.0);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 3 arguments to Color(), all must be numbers!");
                }
            } break;
            case 4:
            {
                if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
                {
                    float r = 0;
                    float g = 0;
                    float b = 0;
                    float a = 0;
                    dc.ReadArg(0, r);
                    dc.ReadArg(1, g);
                    dc.ReadArg(2, b);
                    dc.ReadArg(3, a);
                    *thisPtr = Color(r, g, b, a);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 4 arguments to Color(), all must be numbers!");
                }
            } break;
            default:
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Color() accepts only 1, 3, or 4 arguments, not %d!", numArgs);
            } break;
            }
        }


        void ColorMultiplyGeneric(const Color* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Color result = *thisPtr * scalar;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Color>(0))
                {
                    Color color = Color::CreateZero();
                    dc.ReadArg(0, color);
                    Color result = *thisPtr * color;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Color multiply should have only 1 argument - Color or a Float/Number!");
                dc.PushResult(Color::CreateZero());
            }
        }


        void ColorDivideGeneric(const Color* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 1)
            {
                if (dc.IsNumber(0))
                {
                    float scalar = 0;
                    dc.ReadArg(0, scalar);
                    Color result = *thisPtr / scalar;
                    dc.PushResult(result);
                }
                else if (dc.IsClass<Color>(0))
                {
                    Color color = Color::CreateZero();
                    dc.ReadArg(0, color);
                    Color result = *thisPtr / color;
                    dc.PushResult(result);
                }
            }

            if (!dc.GetNumResults())
            {
                AZ_Error("Script", false, "Color divide should have only 1 argument - Color or a Float/Number!");
                dc.PushResult(Color::CreateZero());
            }
        }


        void ColorSetGeneric(Color* thisPtr, ScriptDataContext& dc)
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
                else if (dc.IsClass<Color>(0))
                {
                    Color setValue = Color::CreateZero();
                    dc.ReadArg(0, setValue);
                    *thisPtr = setValue;
                    valueWasSet = true;
                }
                else if (dc.IsClass<Vector3>(0))
                {
                    Vector3 setValue = Vector3::CreateZero();
                    dc.ReadArg(0, setValue);
                    thisPtr->Set(setValue);
                    valueWasSet = true;
                }
                else if (dc.IsClass<Vector4>(0))
                {
                    Vector4 setValue = Vector4::CreateZero();
                    dc.ReadArg(0, setValue);
                    thisPtr->Set(setValue.GetX(), setValue.GetY(), setValue.GetZ(), setValue.GetW());
                    valueWasSet = true;
                }
            }
            else if (dc.GetNumArguments() == 2 && dc.IsClass<Vector3>(0) && dc.IsNumber(1))
            {
                Vector3 vector3 = Vector3::CreateZero();
                float a = 0;
                dc.ReadArg(0, vector3);
                dc.ReadArg(1, a);
                thisPtr->Set(vector3, a);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 4 && dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
            {
                float r = 0;
                float g = 0;
                float b = 0;
                float a = 0;
                dc.ReadArg(0, r);
                dc.ReadArg(1, g);
                dc.ReadArg(2, b);
                dc.ReadArg(3, a);
                thisPtr->Set(r, g, b, a);
                valueWasSet = true;
            }

            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Color.Set only supports Set(number,number,number,number), Set(number), Set(Vector4), Set(Vector3,float)");
            }
        }


        AZStd::string ColorToString(const Color& c)
        {
            AZ::Locale::ScopedSerializationLocale locale; // colors<--->string should be intepreted in the "C" Locale.
            return AZStd::string::format("(r=%.7f,g=%.7f,b=%.7f,a=%.7f)", static_cast<float>(c.GetR()), static_cast<float>(c.GetG()), static_cast<float>(c.GetB()), static_cast<float>(c.GetA()));
        }
    }


    void Color::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Color>()->
                Serializer<FloatBasedContainerSerializer<Color, &Color::CreateFromFloat4, &Color::StoreToFloat4, &GetTransformEpsilon, 4> >();
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Color>()->
                Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)->
                Attribute(AZ::Script::Attributes::Module, "math")->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)->
                Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
                Constructor<float>()->
                Constructor<float, float, float, float>()->
                    Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::ColorScriptConstructor)->
                    Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::ColorDefaultConstructor)->
                Property("r", &Color::GetR, &Color::SetR)->
                Property("g", &Color::GetG, &Color::SetG)->
                Property("b", &Color::GetB, &Color::SetB)->
                Property("a", &Color::GetA, &Color::SetA)->
                Method<Color(Color::*)() const>("Unary", &Color::operator-)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Unary)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("ToString", &Internal::ColorToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
                Method<Color(Color::*)(float) const>("MultiplyFloat", &Color::operator*)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::ColorMultiplyGeneric)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
                Method<Color(Color::*)(const Color&) const>("MultiplyColor", &Color::operator*)->
                    Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic multiply above
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<Color(Color::*)(float) const>("DivideFloat", &Color::operator/)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::ColorDivideGeneric)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Div)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<Color(Color::*)(const Color&) const>("DivideColor", &Color::operator/)->
                    Attribute(AZ::Script::Attributes::Ignore, 0)-> // ignore for script since we already got the generic divide above
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Clone", [](const Color& rhs) -> Color { return rhs; })->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Equal", &Color::operator==)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("LessThan", &Color::IsLessThan)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("LessEqualThan", &Color::IsLessEqualThan)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessEqualThan)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Add", &Color::operator+)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Add)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<Color(Color::*)(const Color&) const>("Subtract", &Color::operator-)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Sub)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<void (Color::*)(float, float, float, float)>("Set", &Color::Set)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::ColorSetGeneric)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetElement", &Color::GetElement)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("SetElement", &Color::SetElement)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Dot", &Color::Dot)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("Dot3", &Color::Dot3)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("IsClose", &Color::IsClose, behaviorContext->MakeDefaultValues(Constants::Tolerance))->
                Method("IsZero", &Color::IsZero, behaviorContext->MakeDefaultValues(Constants::FloatEpsilon))->
                Method("IsLessThan", &Color::IsLessThan)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("IsLessEqualThan", &Color::IsLessEqualThan)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("IsGreaterThan", &Color::IsGreaterThan)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("IsGreaterEqualThan", &Color::IsGreaterEqualThan)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromVector3", &Color::CreateFromVector3)->
                Method("CreateFromVector3AndFloat", &Color::CreateFromVector3AndFloat)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("CreateOne", &Color::CreateOne)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("CreateZero", &Color::CreateZero)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("ToU32", &Color::ToU32)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("ToU32LinearToGamma", &Color::ToU32LinearToGamma)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("FromU32", &Color::FromU32)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("FromU32GammaToLinear", &Color::FromU32GammaToLinear)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("LinearToGamma", &Color::LinearToGamma)->
                Method("GammaToLinear", &Color::GammaToLinear)->
                Method("ConstructFromValues", &Internal::ConstructColor)
                ;
        }
    }
}
