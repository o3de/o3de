/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathReflection.h>

#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/UuidSerializer.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/VectorN.h>
#include <AzCore/Math/MathMatrixSerializer.h>
#include <AzCore/Math/MathVectorSerializer.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/ColorSerializer.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/TransformSerializer.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MatrixMxN.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/InterpolationSample.h>
#include <AzCore/Math/MathScriptHelpers.h>

#include <AzCore/Casting/lossy_cast.h>

namespace AZ
{
    void MathReflect(SerializeContext& context)
    {
        // aggregates
        context.Class<Uuid>()->
            Serializer<UuidSerializer>();

        Crc32::Reflect(context);
    }


    namespace Internal
    {
        /**
         * Script Wrapper for UUID
         */

         //////////////////////////////////////////////////////////////////////////
        void ScriptUuidConstructor(Uuid* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsString(0))
                {
                    const char* value = nullptr;
                    if (dc.ReadArg(0, value))
                    {
                        *thisPtr = Uuid(value);
                    }
                }
                else
                {
                    AZ_Warning("Script", false, "You should provide only 1 string containing the Uuid in allowed formats!");
                }
            } break;
            default:
                *thisPtr = Uuid::CreateNull();
                break;
            }
        }

        void UuidDefaultConstructor(AZ::Uuid* thisPtr)
        {
            new (thisPtr) AZ::Uuid(AZ::Uuid::CreateNull());
        }

        //////////////////////////////////////////////////////////////////////////
        void UuidCreateStringGeneric(ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsString(0))
                {
                    const char* value = nullptr;
                    if (dc.ReadArg(0, value))
                    {
                        dc.PushResult(Uuid(value));
                    }
                }
                else
                {
                    AZ_Warning("Script", false, "CreateString can have one parameter (null terminated string) or two parameters string and stringLength CreateString(string 'uuid', number stringLen = 0)!");
                }
            } break;
            case 2:
            {
                if (dc.IsString(0) && dc.IsNumber(1))
                {
                    const char* uuidString = nullptr;
                    unsigned int uuidStringLength = 0;
                    if (dc.ReadArg(0, uuidString) && dc.ReadArg(1, uuidStringLength))
                    {
                        dc.PushResult(Uuid(uuidString, uuidStringLength));
                    }
                }
                else
                {
                    AZ_Warning("Script", false, "CreateString can have one parameter (null terminated string) or two parameters string and stringLength CreateString(string 'uuid', number stringLen = 0)!");
                }
            }
            default:
                break;
            }
        }

        /**
         * Script Wrapper for Crc32
         */

         //////////////////////////////////////////////////////////////////////////
        void ScriptCrc32Constructor(Crc32* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() > 0)
            {
                if (dc.IsString(0))
                {
                    const char* str = nullptr;
                    if (dc.ReadArg(0, str))
                    {
                        *thisPtr = Crc32(str);
                    }
                }
                else if (dc.IsClass<Crc32>(0))
                {
                    Crc32 crc32;
                    dc.ReadArg(0, crc32);
                    *thisPtr = crc32;
                }
                else if (dc.IsNumber(0))
                {
                    u32 value = 0;
                    dc.ReadArg(0, value);
                    *thisPtr = value;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void Crc32AddGeneric(Crc32* thisPtr, ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() > 0)
            {
                if (dc.IsString(0))
                {
                    const char* str = nullptr;
                    if (dc.ReadArg(0, str))
                    {
                        thisPtr->Add(str);
                    }
                }
                //else if ()
            }
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string Crc32ToString(const Crc32& crc32)
        {
            return AZStd::string::format("0x%08x", static_cast<u32>(crc32));
        }

        /**
        * Script Wrapper for Random
        */

        //////////////////////////////////////////////////////////////////////////
        void ScriptRandomConstructor(SimpleLcgRandom* thisPtr, ScriptDataContext& dc)
        {
            int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
            {
                if (dc.IsNumber(0))
                {
                    float number;
                    if (dc.ReadArg(0, number))
                    {
                        AZ::u64 seed = static_cast<AZ::u64>(number);
                        *thisPtr = SimpleLcgRandom(seed);
                    }
                }
                else
                {
                    AZ_Warning("Script", false, "You should provde only 1 number containing the random seed!");
                }
            } break;
            default:
                *thisPtr = SimpleLcgRandom();
                break;
            }
        }

        void GetSinCosMultipleReturn(const float* thisPtr, ScriptDataContext& dc)
        {
            float sin, cos;
            AZ::SinCos(*thisPtr, sin, cos);
            dc.PushResult(sin);
            dc.PushResult(cos);
        }
    } // namespace Internal

    class MathGlobals
    {
    public:
        AZ_TYPE_INFO(MathGlobals, "{35D44724-7470-42F2-A0E3-4E4349793B98}");
        AZ_CLASS_ALLOCATOR(MathGlobals, SystemAllocator);

        MathGlobals() = default;
        ~MathGlobals() = default;
    };

    void MathReflect(BehaviorContext& context)
    {
        context.Constant("FloatEpsilon", BehaviorConstant(Constants::FloatEpsilon));

        context.Class<MathGlobals>("Math")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, "math")
            ->Method("DegToRad", &DegToRad, { {{"Degrees", ""}} })
            ->Method("RadToDeg", &RadToDeg, {{{"Radians", ""}}})
            ->Method("Sin", &sinf)
            ->Method("Cos", &cosf)
            ->Method("Tan", &tanf)
            ->Method("ArcSin", &asinf)
            ->Method("ArcCos", &acosf)
            ->Method("ArcTan", &atanf)
            ->Method("ArcTan2", &atan2f)
            ->Method("Ceil", &ceilf)
            ->Method("Floor", &floorf)
            ->Method("Round", &roundf)
            ->Method("Sqrt", &sqrtf)
            ->Method("Pow", &powf)
            ->Method("Sign", &GetSign)
            ->Method("Min", &GetMin<float>)
            ->Method("Max", &GetMax<float>)
            ->Method<float(float, float, float)>("Lerp", &AZ::Lerp,               { { { "a", "" },{ "b", "" },{ "t", "" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns a linear interpolation between two values 'a' and 'b'")
            ->Method<float(float, float, float)>("LerpInverse", &AZ::LerpInverse, { { { "a", "" },{ "b", "" },{ "value", "" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns a value t where Lerp(a,b,t)==value (or 0 if a==b)")
            ->Method("Clamp", &GetClamp<float>)
            ->Method("IsEven", &IsEven<int>)
            ->Method<bool(int)>("IsOdd", &IsOdd<int>, {{{"Value",""}}})
            ->Method<float(float,float)>("Mod", &GetMod)
            ->Method<void(float, float&, float&)>("GetSinCos", &AZ::SinCos)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::MethodOverride, &Internal::GetSinCosMultipleReturn)
            ->Method<bool(double, double, double)>("IsClose", &AZ::IsClose, context.MakeDefaultValues(static_cast<double>(Constants::FloatEpsilon)))
            ->Method<float(float)>("Abs", &GetAbs)
            ->Method("Divide By Number", [](float lhs, float rhs) { return lhs / rhs; }, { "Source", "The source value gets divided." }, { { { "Divisor", "The value that divides Source." } } })
            ->Attribute(AZ::ScriptCanvasAttributes::ExplicitOverloadCrc, ExplicitOverloadInfo("Divide By Number (/)", "Math"))
            ->Attribute(AZ::ScriptCanvasAttributes::OperatorOverride, AZ::Script::Attributes::OperatorType::Div)
            ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "DivideGroup", "" }, { "DivideGroup" }))
             ;

        // Uuid
        context.Class<Uuid>("Uuid")->
            Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)->
            Attribute(AZ::Script::Attributes::Module, "math")->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::ScriptUuidConstructor)->
            Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::UuidDefaultConstructor)->
            Method("ToString", [](const Uuid* self) { return self->ToString<AZStd::string>(); })->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method("LessThan", &Uuid::operator<)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<bool (Uuid::*)(const Uuid&) const>("Equal", &Uuid::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
            Method("Clone", [](const Uuid& rhs) -> Uuid { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("IsNull", &Uuid::IsNull)->
            Method("Create", &Uuid::Create)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateNull", &Uuid::CreateNull)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateString", [](AZStd::string_view uuidString) { return Uuid::CreateString(uuidString); })->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::UuidCreateStringGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateName", [](AZStd::string_view nameString) { return Uuid::CreateName(nameString); })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("CreateRandom", &Uuid::CreateRandom);

        // Random
        context.Class<SimpleLcgRandom>("Random")->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::ScriptRandomConstructor)->
            Method("SetSeed", &SimpleLcgRandom::SetSeed, {{{ "Seed", "" }}})->
            Method("GetRandom", &SimpleLcgRandom::GetRandom)->
            Method("GetRandomFloat", &SimpleLcgRandom::GetRandomFloat);

        // Crc
        context.Class<Crc32>()->
            Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)->
            Attribute(AZ::Script::Attributes::Module, "math")->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
            Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::ScriptCrc32Constructor)->
            Property("value", &Crc32::operator u32, nullptr)->
            Method("ToString", &Internal::Crc32ToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method("Equal", &Crc32::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method("Clone", [](const Crc32& rhs) -> Crc32 { return rhs; })->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Method<void(Crc32::*)(AZStd::string_view)>("Add", &Crc32::Add)->
                Attribute(AZ::Script::Attributes::MethodOverride, &Internal::Crc32AddGeneric)->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Property("stringValue", nullptr, [](Crc32* thisPtr, AZStd::string_view value) { *thisPtr  = Crc32(value); })->
            Method("CreateCrc32", [](AZStd::string_view value) -> Crc32 { return Crc32(value); }, { { { "value", "String to compute to Crc32" } } })->
            Constructor<AZStd::string_view>()
            ;

        // Interpolation
        context.Class<InterpolationMode>()->
            Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
            Enum<(int)AZ::InterpolationMode::NoInterpolation>("NoInterpolation")->
            Enum<(int)AZ::InterpolationMode::LinearInterpolation>("LinearInterpolation");
    }

    void MathReflect(JsonRegistrationContext& context)
    {
        context.Serializer<JsonColorSerializer>()->HandlesType<Color>();
        context.Serializer<JsonUuidSerializer>()->HandlesType<Uuid>();
        context.Serializer<JsonMatrix3x3Serializer>()->HandlesType<Matrix3x3>();
        context.Serializer<JsonMatrix3x4Serializer>()->HandlesType<Matrix3x4>();
        context.Serializer<JsonMatrix4x4Serializer>()->HandlesType<Matrix4x4>();
        context.Serializer<JsonVector2Serializer>()->HandlesType<Vector2>();
        context.Serializer<JsonVector3Serializer>()->HandlesType<Vector3>();
        context.Serializer<JsonVector4Serializer>()->HandlesType<Vector4>();
        context.Serializer<JsonQuaternionSerializer>()->HandlesType<Quaternion>();
        context.Serializer<JsonTransformSerializer>()->HandlesType<Transform>();
    }

    void MathReflect(ReflectContext* context)
    {
        if (context)
        {
            Aabb::Reflect(context);
            Obb::Reflect(context);
            Color::Reflect(context);
            Vector2::Reflect(context);
            Vector3::Reflect(context);
            Vector4::Reflect(context);
            VectorN::Reflect(context);
            Quaternion::Reflect(context);
            Matrix3x3::Reflect(context);
            Matrix3x4::Reflect(context);
            Matrix4x4::Reflect(context);
            MatrixMxN::Reflect(context);
            Frustum::Reflect(context);
            Plane::Reflect(context);
            Transform::Reflect(context);

            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                MathReflect(*serializeContext);
            }

            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                MathReflect(*behaviorContext);
            }

            JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context);
            if (jsonContext)
            {
                MathReflect(*jsonContext);
            }
        }
    }
}
