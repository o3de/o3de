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


    namespace Colors
    {
        // Basic Colors (CSS 1 standard)
        const Color White                     { 1.000f, 1.000f, 1.000f, 1.000f }; // RGB: (255, 255, 255)
        const Color Silver                    { 0.753f, 0.753f, 0.753f, 1.000f }; // RGB: (192, 192, 192)
        const Color Gray                      { 0.500f, 0.500f, 0.500f, 1.000f }; // RGB: (128, 128, 128)
        const Color Black                     { 0.000f, 0.000f, 0.000f, 1.000f }; // RGB: (0, 0, 0)
        const Color Red                       { 1.000f, 0.000f, 0.000f, 1.000f }; // RGB: (255, 0, 0)
        const Color Maroon                    { 0.500f, 0.000f, 0.000f, 1.000f }; // RGB: (128, 0, 0)
        const Color Lime                      { 0.000f, 1.000f, 0.000f, 1.000f }; // RGB: (0, 255, 0)
        const Color Green                     { 0.000f, 0.500f, 0.000f, 1.000f }; // RGB: (0, 128, 0)
        const Color Blue                      { 0.000f, 0.000f, 1.000f, 1.000f }; // RGB: (0, 0, 255)
        const Color Navy                      { 0.000f, 0.000f, 0.500f, 1.000f }; // RGB: (0, 0, 128)
        const Color Yellow                    { 1.000f, 1.000f, 0.000f, 1.000f }; // RGB: (255, 255, 0)
        const Color Orange                    { 1.000f, 0.647f, 0.000f, 1.000f }; // RGB: (255, 165, 0)
        const Color Olive                     { 0.500f, 0.500f, 0.000f, 1.000f }; // RGB: (128, 128, 0)
        const Color Purple                    { 0.500f, 0.000f, 0.500f, 1.000f }; // RGB: (128, 0, 128)
        const Color Fuchsia                   { 1.000f, 0.000f, 1.000f, 1.000f }; // RGB: (255, 0, 255)
        const Color Teal                      { 0.000f, 0.500f, 0.500f, 1.000f }; // RGB: (0, 128, 128)
        const Color Aqua                      { 0.000f, 1.000f, 1.000f, 1.000f }; // RGB: (0, 255, 255)
        // CSS3 colors
        // Reds
        const Color IndianRed                 { 0.804f, 0.361f, 0.361f, 1.000f }; // RGB: (205, 92, 92)
        const Color LightCoral                { 0.941f, 0.502f, 0.502f, 1.000f }; // RGB: (240, 128, 128)
        const Color Salmon                    { 0.980f, 0.502f, 0.447f, 1.000f }; // RGB: (250, 128, 114)
        const Color DarkSalmon                { 0.914f, 0.588f, 0.478f, 1.000f }; // RGB: (233, 150, 122)
        const Color LightSalmon               { 1.000f, 0.627f, 0.478f, 1.000f }; // RGB: (255, 160, 122)
        const Color Crimson                   { 0.863f, 0.078f, 0.235f, 1.000f }; // RGB: (220, 20, 60)
        const Color FireBrick                 { 0.698f, 0.133f, 0.133f, 1.000f }; // RGB: (178, 34, 34)
        const Color DarkRed                   { 0.545f, 0.000f, 0.000f, 1.000f }; // RGB: (139, 0, 0)
        // Pinks                                                              
        const Color Pink                      { 1.000f, 0.753f, 0.796f, 1.000f }; // RGB: (255, 192, 203)
        const Color LightPink                 { 1.000f, 0.714f, 0.757f, 1.000f }; // RGB: (255, 182, 193)
        const Color HotPink                   { 1.000f, 0.412f, 0.706f, 1.000f }; // RGB: (255, 105, 180)
        const Color DeepPink                  { 1.000f, 0.078f, 0.576f, 1.000f }; // RGB: (255, 20, 147)
        const Color MediumVioletRed           { 0.780f, 0.082f, 0.522f, 1.000f }; // RGB: (199, 21, 133)
        const Color PaleVioletRed             { 0.859f, 0.439f, 0.576f, 1.000f }; // RGB: (219, 112, 147)
        // Oranges
        const Color Coral                     { 1.000f, 0.498f, 0.314f, 1.000f }; // RGB: (255, 127, 80)
        const Color Tomato                    { 1.000f, 0.388f, 0.278f, 1.000f }; // RGB: (255, 99, 71)
        const Color OrangeRed                 { 1.000f, 0.271f, 0.000f, 1.000f }; // RGB: (255, 69, 0)
        const Color DarkOrange                { 1.000f, 0.549f, 0.000f, 1.000f }; // RGB: (255, 140, 0)
        // Yellows
        const Color Gold                      { 1.000f, 0.843f, 0.000f, 1.000f }; // RGB: (255, 215, 0)
        const Color LightYellow               { 1.000f, 1.000f, 0.878f, 1.000f }; // RGB: (255, 255, 224)
        const Color LemonChiffon              { 1.000f, 0.980f, 0.804f, 1.000f }; // RGB: (255, 250, 205)
        const Color LightGoldenrodYellow      { 0.980f, 0.980f, 0.824f, 1.000f }; // RGB: (250, 250, 210)
        const Color PapayaWhip                { 1.000f, 0.937f, 0.835f, 1.000f }; // RGB: (255, 239, 213)
        const Color Moccasin                  { 1.000f, 0.894f, 0.710f, 1.000f }; // RGB: (255, 228, 181)
        const Color PeachPuff                 { 1.000f, 0.855f, 0.725f, 1.000f }; // RGB: (255, 218, 185)
        const Color PaleGoldenrod             { 0.933f, 0.910f, 0.667f, 1.000f }; // RGB: (238, 232, 170)
        const Color Khaki                     { 0.941f, 0.902f, 0.549f, 1.000f }; // RGB: (240, 230, 140)
        const Color DarkKhaki                 { 0.741f, 0.718f, 0.420f, 1.000f }; // RGB: (189, 183, 107)
        // Purples                                                                    
        const Color Lavender                  { 0.902f, 0.902f, 0.980f, 1.000f }; // RGB: (230, 230, 250)
        const Color Thistle                   { 0.847f, 0.749f, 0.847f, 1.000f }; // RGB: (216, 191, 216)
        const Color Plum                      { 0.867f, 0.627f, 0.867f, 1.000f }; // RGB: (221, 160, 221)
        const Color Violet                    { 0.933f, 0.510f, 0.933f, 1.000f }; // RGB: (238, 130, 238)
        const Color Orchid                    { 0.855f, 0.439f, 0.839f, 1.000f }; // RGB: (218, 112, 214)
        const Color Magenta                   { 1.000f, 0.000f, 1.000f, 1.000f }; // RGB: (255, 0, 255)
        const Color MediumOrchid              { 0.729f, 0.333f, 0.827f, 1.000f }; // RGB: (186, 85, 211)
        const Color MediumPurple              { 0.576f, 0.439f, 0.859f, 1.000f }; // RGB: (147, 112, 219)
        const Color BlueViolet                { 0.541f, 0.169f, 0.886f, 1.000f }; // RGB: (138, 43, 226)
        const Color DarkViolet                { 0.580f, 0.000f, 0.827f, 1.000f }; // RGB: (148, 0, 211)
        const Color DarkOrchid                { 0.600f, 0.196f, 0.800f, 1.000f }; // RGB: (153, 50, 204)
        const Color DarkMagenta               { 0.545f, 0.000f, 0.545f, 1.000f }; // RGB: (139, 0, 139)
        const Color RebeccaPurple             { 0.400f, 0.200f, 0.600f, 1.000f }; // RGB: (102, 51, 153)
        const Color Indigo                    { 0.294f, 0.000f, 0.510f, 1.000f }; // RGB: (75, 0, 130)
        const Color MediumSlateBlue           { 0.482f, 0.408f, 0.933f, 1.000f }; // RGB: (123, 104, 238)
        const Color SlateBlue                 { 0.416f, 0.353f, 0.804f, 1.000f }; // RGB: (106, 90, 205)
        const Color DarkSlateBlue             { 0.282f, 0.239f, 0.545f, 1.000f }; // RGB: (72, 61, 139)
        // Greens
        const Color GreenYellow               { 0.678f, 1.000f, 0.184f, 1.000f }; // RGB: (173, 255, 47)
        const Color Chartreuse                { 0.498f, 1.000f, 0.000f, 1.000f }; // RGB: (127, 255, 0)
        const Color LawnGreen                 { 0.486f, 0.988f, 0.000f, 1.000f }; // RGB: (124, 252, 0)
        const Color LimeGreen                 { 0.196f, 0.804f, 0.196f, 1.000f }; // RGB: (50, 205, 50)
        const Color PaleGreen                 { 0.596f, 0.984f, 0.596f, 1.000f }; // RGB: (152, 251, 152)
        const Color LightGreen                { 0.565f, 0.933f, 0.565f, 1.000f }; // RGB: (144, 238, 144)
        const Color MediumSpringGreen         { 0.000f, 0.980f, 0.604f, 1.000f }; // RGB: (0, 250, 154)
        const Color SpringGreen               { 0.000f, 1.000f, 0.498f, 1.000f }; // RGB: (0, 255, 127)
        const Color MediumSeaGreen            { 0.235f, 0.702f, 0.443f, 1.000f }; // RGB: (60, 179, 113)
        const Color SeaGreen                  { 0.180f, 0.545f, 0.341f, 1.000f }; // RGB: (46, 139, 87)
        const Color ForestGreen               { 0.133f, 0.545f, 0.133f, 1.000f }; // RGB: (34, 139, 34)
        const Color DarkGreen                 { 0.000f, 0.392f, 0.000f, 1.000f }; // RGB: (0, 100, 0)
        const Color YellowGreen               { 0.604f, 0.804f, 0.196f, 1.000f }; // RGB: (154, 205, 50)
        const Color OliveDrab                 { 0.420f, 0.557f, 0.137f, 1.000f }; // RGB: (107, 142, 35)
        const Color DarkOliveGreen            { 0.333f, 0.420f, 0.184f, 1.000f }; // RGB: (85, 107, 47)
        const Color MediumAquamarine          { 0.400f, 0.804f, 0.667f, 1.000f }; // RGB: (102, 205, 170)
        const Color DarkSeaGreen              { 0.561f, 0.737f, 0.561f, 1.000f }; // RGB: (143, 188, 143)
        const Color LightSeaGreen             { 0.125f, 0.698f, 0.667f, 1.000f }; // RGB: (32, 178, 170)
        const Color DarkCyan                  { 0.000f, 0.545f, 0.545f, 1.000f }; // RGB: (0, 139, 139)
        // Blues
        const Color Cyan                      { 0.000f, 1.000f, 1.000f, 1.000f }; // RGB: (0, 255, 255)
        const Color LightCyan                 { 0.878f, 1.000f, 1.000f, 1.000f }; // RGB: (224, 255, 255)
        const Color PaleTurquoise             { 0.686f, 0.933f, 0.933f, 1.000f }; // RGB: (175, 238, 238)
        const Color Aquamarine                { 0.498f, 1.000f, 0.831f, 1.000f }; // RGB: (127, 255, 212)
        const Color Turquoise                 { 0.251f, 0.878f, 0.816f, 1.000f }; // RGB: (64, 224, 208)
        const Color MediumTurquoise           { 0.282f, 0.820f, 0.800f, 1.000f }; // RGB: (72, 209, 204)
        const Color DarkTurquoise             { 0.000f, 0.808f, 0.820f, 1.000f }; // RGB: (0, 206, 209)
        const Color CadetBlue                 { 0.373f, 0.620f, 0.627f, 1.000f }; // RGB: (95, 158, 160)
        const Color SteelBlue                 { 0.275f, 0.510f, 0.706f, 1.000f }; // RGB: (70, 130, 180)
        const Color LightSteelBlue            { 0.690f, 0.769f, 0.871f, 1.000f }; // RGB: (176, 196, 222)
        const Color PowderBlue                { 0.690f, 0.878f, 0.902f, 1.000f }; // RGB: (176, 224, 230)
        const Color LightBlue                 { 0.678f, 0.847f, 0.902f, 1.000f }; // RGB: (173, 216, 230)
        const Color SkyBlue                   { 0.529f, 0.808f, 0.922f, 1.000f }; // RGB: (135, 206, 235)
        const Color LightSkyBlue              { 0.529f, 0.808f, 0.980f, 1.000f }; // RGB: (135, 206, 250)
        const Color DeepSkyBlue               { 0.000f, 0.749f, 1.000f, 1.000f }; // RGB: (0, 191, 255)
        const Color DodgerBlue                { 0.118f, 0.565f, 1.000f, 1.000f }; // RGB: (30, 144, 255)
        const Color CornflowerBlue            { 0.392f, 0.584f, 0.929f, 1.000f }; // RGB: (100, 149, 237)
        const Color RoyalBlue                 { 0.255f, 0.412f, 0.882f, 1.000f }; // RGB: (65, 105, 225)
        const Color MediumBlue                { 0.000f, 0.000f, 0.804f, 1.000f }; // RGB: (0, 0, 205)
        const Color DarkBlue                  { 0.000f, 0.000f, 0.545f, 1.000f }; // RGB: (0, 0, 139)
        const Color MidnightBlue              { 0.098f, 0.098f, 0.439f, 1.000f }; // RGB: (25, 25, 112)
        // Browns
        const Color Cornsilk                  { 1.000f, 0.973f, 0.863f, 1.000f }; // RGB: (255, 248, 220)
        const Color BlanchedAlmond            { 1.000f, 0.922f, 0.804f, 1.000f }; // RGB: (255, 235, 205)
        const Color Bisque                    { 1.000f, 0.894f, 0.769f, 1.000f }; // RGB: (255, 228, 196)
        const Color NavajoWhite               { 1.000f, 0.871f, 0.678f, 1.000f }; // RGB: (255, 222, 173)
        const Color Wheat                     { 0.961f, 0.871f, 0.702f, 1.000f }; // RGB: (245, 222, 179)
        const Color BurlyWood                 { 0.871f, 0.722f, 0.529f, 1.000f }; // RGB: (222, 184, 135)
        const Color Tan                       { 0.824f, 0.706f, 0.549f, 1.000f }; // RGB: (210, 180, 140)
        const Color RosyBrown                 { 0.737f, 0.561f, 0.561f, 1.000f }; // RGB: (188, 143, 143)
        const Color SandyBrown                { 0.957f, 0.643f, 0.376f, 1.000f }; // RGB: (244, 164, 96)
        const Color Goldenrod                 { 0.855f, 0.647f, 0.125f, 1.000f }; // RGB: (218, 165, 32)
        const Color DarkGoldenrod             { 0.722f, 0.525f, 0.043f, 1.000f }; // RGB: (184, 134, 11)
        const Color Peru                      { 0.804f, 0.522f, 0.247f, 1.000f }; // RGB: (205, 133, 63)
        const Color Chocolate                 { 0.824f, 0.412f, 0.118f, 1.000f }; // RGB: (210, 105, 30)
        const Color SaddleBrown               { 0.545f, 0.271f, 0.075f, 1.000f }; // RGB: (139, 69, 19)
        const Color Sienna                    { 0.627f, 0.322f, 0.176f, 1.000f }; // RGB: (160, 82, 45)
        const Color Brown                     { 0.647f, 0.165f, 0.165f, 1.000f }; // RGB: (165, 42, 42)
        // Whites
        const Color Snow                      { 1.000f, 0.980f, 0.980f, 1.000f }; // RGB: (255, 250, 250)
        const Color Honeydew                  { 0.941f, 1.000f, 0.941f, 1.000f }; // RGB: (240, 255, 240)
        const Color MintCream                 { 0.961f, 1.000f, 0.980f, 1.000f }; // RGB: (245, 255, 250)
        const Color Azure                     { 0.941f, 1.000f, 1.000f, 1.000f }; // RGB: (240, 255, 255)
        const Color AliceBlue                 { 0.941f, 0.973f, 1.000f, 1.000f }; // RGB: (240, 248, 255)
        const Color GhostWhite                { 0.973f, 0.973f, 1.000f, 1.000f }; // RGB: (248, 248, 255)
        const Color WhiteSmoke                { 0.961f, 0.961f, 0.961f, 1.000f }; // RGB: (245, 245, 245)
        const Color Seashell                  { 1.000f, 0.961f, 0.933f, 1.000f }; // RGB: (255, 245, 238)
        const Color Beige                     { 0.961f, 0.961f, 0.863f, 1.000f }; // RGB: (245, 245, 220)
        const Color OldLace                   { 0.992f, 0.961f, 0.902f, 1.000f }; // RGB: (253, 245, 230)
        const Color FloralWhite               { 1.000f, 0.980f, 0.941f, 1.000f }; // RGB: (255, 250, 240)
        const Color Ivory                     { 1.000f, 1.000f, 0.941f, 1.000f }; // RGB: (255, 255, 240)
        const Color AntiqueWhite              { 0.980f, 0.922f, 0.843f, 1.000f }; // RGB: (250, 235, 215)
        const Color Linen                     { 0.980f, 0.941f, 0.902f, 1.000f }; // RGB: (250, 240, 230)
        const Color LavenderBlush             { 1.000f, 0.941f, 0.961f, 1.000f }; // RGB: (255, 240, 245)
        const Color MistyRose                 { 1.000f, 0.894f, 0.882f, 1.000f }; // RGB: (255, 228, 225)
        // Grays
        const Color Gainsboro                 { 0.863f, 0.863f, 0.863f, 1.000f }; // RGB: (220, 220, 220)
        const Color LightGray                 { 0.827f, 0.827f, 0.827f, 1.000f }; // RGB: (211, 211, 211)
        const Color LightGrey                 { 0.827f, 0.827f, 0.827f, 1.000f }; // RGB: (211, 211, 211)
        const Color DarkGray                  { 0.663f, 0.663f, 0.663f, 1.000f }; // RGB: (169, 169, 169)
        const Color DarkGrey                  { 0.663f, 0.663f, 0.663f, 1.000f }; // RGB: (169, 169, 169)
        const Color Grey                      { 0.502f, 0.502f, 0.502f, 1.000f }; // RGB: (128, 128, 128)
        const Color DimGray                   { 0.412f, 0.412f, 0.412f, 1.000f }; // RGB: (105, 105, 105)
        const Color DimGrey                   { 0.412f, 0.412f, 0.412f, 1.000f }; // RGB: (105, 105, 105)
        const Color LightSlateGray            { 0.467f, 0.533f, 0.600f, 1.000f }; // RGB: (119, 136, 153)
        const Color LightSlateGrey            { 0.467f, 0.533f, 0.600f, 1.000f }; // RGB: (119, 136, 153)
        const Color SlateGray                 { 0.439f, 0.502f, 0.565f, 1.000f }; // RGB: (112, 128, 144)
        const Color SlateGrey                 { 0.439f, 0.502f, 0.565f, 1.000f }; // RGB: (112, 128, 144)
        const Color DarkSlateGray             { 0.184f, 0.310f, 0.310f, 1.000f }; // RGB: (47, 79, 79)
        const Color DarkSlateGrey             { 0.184f, 0.310f, 0.310f, 1.000f }; // RGB: (47, 79, 79)
    }
}
