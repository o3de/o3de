/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/string/string_view.h>
#include <ScriptCanvas/Core/Datum.h>
#include <AzCore/std/function/invoke.h>

namespace ScriptCanvas
{
    namespace Data
    {
        using GetterFunction = AZStd::function<AZ::Outcome<Datum, AZStd::string>(const Datum&)>;
        struct GetterWrapper
        {
            AZ_CLASS_ALLOCATOR(GetterWrapper, AZ::SystemAllocator, 0);
            GetterFunction m_getterFunction;
            Data::Type m_propertyType;
            AZStd::string m_propertyName;
        };
        using GetterContainer = AZStd::unordered_map<AZStd::string, GetterWrapper>;

        using SetterFunction = AZStd::function<AZ::Outcome<void, AZStd::string>(Datum&, const Datum&)>;
        struct SetterWrapper
        {
            AZ_CLASS_ALLOCATOR(SetterWrapper, AZ::SystemAllocator, 0);
            SetterFunction m_setterFunction;
            Data::Type m_propertyType;
            AZStd::string m_propertyName;
        };
        using SetterContainer = AZStd::unordered_map<AZStd::string, SetterWrapper>;

        struct PropertyMetadata
        {
            AZ_TYPE_INFO(PropertyMetadata, "{A4910EF1-0139-4A7A-878C-E60E18F3993A}");
            AZ_CLASS_ALLOCATOR(PropertyMetadata, AZ::SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* reflectContext);

            SlotId m_propertySlotId;
            Data::Type m_propertyType;
            AZStd::string m_propertyName;
            Data::GetterFunction m_getterFunction;
        };


        AZStd::vector<AZ::BehaviorProperty*> GetBehaviorProperties(const Data::Type& type);

        GetterContainer ExplodeToGetters(const Data::Type& type);
        SetterContainer ExplodeToSetters(const Data::Type& type);

        AZ_INLINE AZStd::vector<AZ::BehaviorProperty*> GetBehaviorProperties(const Data::Type& type)
        {
            AZ_Error("Script Canvas", type.GetType() == Data::eType::BehaviorContextObject, "GetBehaviorProperties cannot return properties for non-BehaviorContextObjexts");

            AZStd::vector<AZ::BehaviorProperty*> propertiesByType;

            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(type.GetAZType());
            if (behaviorClass)
            {
                for (auto&& namePropertyPair : behaviorClass->m_properties)
                {
                    propertiesByType.push_back(namePropertyPair.second);
                }
            }

            return propertiesByType;
        }

        namespace Internal
        {
            template<typename FunctionType, typename GetterType>
            struct WrapGetterHelper
            {
                static_assert(!AZStd::is_void<GetterType>::value, "Getter function must be either a member pointer that accepts 0 arguments or an invokable object that accepts 1 argument");

                using PropertyType = AZStd::decay_t<AZStd::invoke_result_t<FunctionType, GetterType>>;
                static_assert(!AZStd::is_void<PropertyType>::value, "Getter function must return a non-void type");

                static GetterWrapper Callback(AZStd::string_view propertyName, const FunctionType& propertyGetter)
                {
                    GetterFunction getterWrapper = [propertyGetter](const Datum& thisDatum) -> AZ::Outcome<Datum, AZStd::string>
                    {
                        auto thisObject = thisDatum.GetAs<GetterType>();
                        if (!thisObject)
                        {
                            return AZ::Failure(AZStd::string::format("Unable to invoke getter. The getter parameter is nullptr"));
                        }

                        return AZ::Success(Datum(AZStd::invoke(propertyGetter, thisObject)));
                    };

                    return { getterWrapper, Data::FromAZType<PropertyType>(), propertyName };
                }
            };

            template<typename FunctionType, typename SetterType, typename PropertyType>
            struct WrapSetterHelper
            {
                static_assert(!AZStd::is_void<SetterType>::value, "Setter function must be either a member function pointer that accepts 1 arguments or an invokable object that accepts 2 argument");
                static_assert(!AZStd::is_void<PropertyType>::value, "Property being set must be a non-void type");

                static SetterWrapper Callback(AZStd::string_view propertyName, const FunctionType& propertySetter)
                {
                    SetterFunction setterWrapper = [propertySetter](Datum& thisDatum, const Datum& propertyDatum) -> AZ::Outcome<void, AZStd::string>
                    {

                        auto thisObject = thisDatum.ModAs<SetterType>();
                        auto propertyObject = propertyDatum.GetAs<AZStd::decay_t<PropertyType>>();
                        if (!thisObject)
                        {
                            return AZ::Failure(AZStd::string::format("Unable to invoke setter. The setter parameter is nullptr"));
                        }

                        if (!propertyObject)
                        {
                            return AZ::Failure(AZStd::string::format("Unable to invoke setter. The property value parameter is nullptr"));
                        }

                        AZStd::invoke(propertySetter, thisObject, *propertyObject);
                        return AZ::Success();
                    };

                    return { setterWrapper, Data::FromAZType<PropertyType>(), propertyName };
                }
            };
        }

        template<typename FunctionType, typename = void>
        struct WrapGetter;

        template<typename FunctionType>
        struct WrapGetter<FunctionType, AZStd::enable_if_t<AZStd::is_member_pointer<FunctionType>::value>>
            : Internal::WrapGetterHelper<FunctionType, typename AZStd::function_traits<FunctionType>::class_type>
        {};

        template<typename FunctionType>
        struct WrapGetter<FunctionType, AZStd::enable_if_t<!AZStd::is_member_pointer<FunctionType>::value && AZStd::function_traits<FunctionType>::arity == 1>>
            : Internal::WrapGetterHelper<FunctionType, AZStd::decay_t<AZStd::function_traits_get_arg_t<FunctionType, 0>>>
        {};

        template<typename FunctionType, typename = void>
        struct WrapSetter;

        template<typename FunctionType>
        struct WrapSetter<FunctionType, AZStd::enable_if_t<AZStd::is_member_function_pointer<FunctionType>::value && AZStd::function_traits<FunctionType>::arity == 1>>
            : Internal::WrapSetterHelper<FunctionType, typename AZStd::function_traits<FunctionType>::class_type, AZStd::function_traits_get_arg_t<FunctionType, 0>>
        {};

        template<typename FunctionType>
        struct WrapSetter<FunctionType, AZStd::enable_if_t<!AZStd::is_member_function_pointer<FunctionType>::value && AZStd::function_traits<FunctionType>::arity == 2>>
            : Internal::WrapSetterHelper<FunctionType, AZStd::function_traits_get_arg_t<FunctionType, 0>, AZStd::function_traits_get_arg_t<FunctionType, 1>>
        {};

        template<eType>
        struct PropertyTraits
        {
            static GetterContainer GetGetterWrappers([[maybe_unused]] const Data::Type& type)
            {
                return GetterContainer{};
            }
            static SetterContainer GetSetterWrappers([[maybe_unused]] const Data::Type& type)
            {
                return SetterContainer{};
            }
        };

        template<>
        struct PropertyTraits<Data::eType::Quaternion>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("X", WrapGetter<decltype(&Data::QuaternionType::GetX)>::Callback("x", &Data::QuaternionType::GetX));
                getterFunctions.emplace("Y", WrapGetter<decltype(&Data::QuaternionType::GetY)>::Callback("y", &Data::QuaternionType::GetY));
                getterFunctions.emplace("Z", WrapGetter<decltype(&Data::QuaternionType::GetZ)>::Callback("z", &Data::QuaternionType::GetZ));
                getterFunctions.emplace("W", WrapGetter<decltype(&Data::QuaternionType::GetW)>::Callback("w", &Data::QuaternionType::GetW));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("X", WrapSetter<decltype(&Data::QuaternionType::SetX)>::Callback("x", &Data::QuaternionType::SetX));
                setterFunctions.emplace("Y", WrapSetter<decltype(&Data::QuaternionType::SetY)>::Callback("y", &Data::QuaternionType::SetY));
                setterFunctions.emplace("Z", WrapSetter<decltype(&Data::QuaternionType::SetZ)>::Callback("z", &Data::QuaternionType::SetZ));
                setterFunctions.emplace("W", WrapSetter<decltype(&Data::QuaternionType::SetW)>::Callback("w", &Data::QuaternionType::SetW));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::Vector2>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("X", WrapGetter<decltype(&Data::Vector2Type::GetX)>::Callback("x", &Data::Vector2Type::GetX));
                getterFunctions.emplace("Y", WrapGetter<decltype(&Data::Vector2Type::GetY)>::Callback("y", &Data::Vector2Type::GetY));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("X", WrapSetter<decltype(&Data::Vector2Type::SetX)>::Callback("x", &Data::Vector2Type::SetX));
                setterFunctions.emplace("Y", WrapSetter<decltype(&Data::Vector2Type::SetY)>::Callback("y", &Data::Vector2Type::SetY));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::Vector3>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("X", WrapGetter<decltype(&Data::Vector3Type::GetX)>::Callback("x", &Data::Vector3Type::GetX));
                getterFunctions.emplace("Y", WrapGetter<decltype(&Data::Vector3Type::GetY)>::Callback("y", &Data::Vector3Type::GetY));
                getterFunctions.emplace("Z", WrapGetter<decltype(&Data::Vector3Type::GetZ)>::Callback("z", &Data::Vector3Type::GetZ));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("X", WrapSetter<decltype(&Data::Vector3Type::SetX)>::Callback("x", &Data::Vector3Type::SetX));
                setterFunctions.emplace("Y", WrapSetter<decltype(&Data::Vector3Type::SetY)>::Callback("y", &Data::Vector3Type::SetY));
                setterFunctions.emplace("Z", WrapSetter<decltype(&Data::Vector3Type::SetZ)>::Callback("z", &Data::Vector3Type::SetZ));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::Vector4>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("X", WrapGetter<decltype(&Data::Vector4Type::GetX)>::Callback("x", &Data::Vector4Type::GetX));
                getterFunctions.emplace("Y", WrapGetter<decltype(&Data::Vector4Type::GetY)>::Callback("y", &Data::Vector4Type::GetY));
                getterFunctions.emplace("Z", WrapGetter<decltype(&Data::Vector4Type::GetZ)>::Callback("z", &Data::Vector4Type::GetZ));
                getterFunctions.emplace("W", WrapGetter<decltype(&Data::Vector4Type::GetW)>::Callback("w", &Data::Vector4Type::GetW));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("X", WrapSetter<decltype(&Data::Vector4Type::SetX)>::Callback("x", &Data::Vector4Type::SetX));
                setterFunctions.emplace("Y", WrapSetter<decltype(&Data::Vector4Type::SetY)>::Callback("y", &Data::Vector4Type::SetY));
                setterFunctions.emplace("Z", WrapSetter<decltype(&Data::Vector4Type::SetZ)>::Callback("z", &Data::Vector4Type::SetZ));
                setterFunctions.emplace("W", WrapSetter<decltype(&Data::Vector4Type::SetW)>::Callback("w", &Data::Vector4Type::SetW));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::Color>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("Red", WrapGetter<decltype(&Data::ColorType::GetR)>::Callback("r", &Data::ColorType::GetR));
                getterFunctions.emplace("Green", WrapGetter<decltype(&Data::ColorType::GetG)>::Callback("g", &Data::ColorType::GetG));
                getterFunctions.emplace("Blue", WrapGetter<decltype(&Data::ColorType::GetB)>::Callback("b", &Data::ColorType::GetB));
                getterFunctions.emplace("Alpha", WrapGetter<decltype(&Data::ColorType::GetA)>::Callback("a", &Data::ColorType::GetA));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("Red", WrapSetter<decltype(&Data::ColorType::SetR)>::Callback("r", &Data::ColorType::SetR));
                setterFunctions.emplace("Green", WrapSetter<decltype(&Data::ColorType::SetG)>::Callback("g", &Data::ColorType::SetG));
                setterFunctions.emplace("Blue", WrapSetter<decltype(&Data::ColorType::SetB)>::Callback("b", &Data::ColorType::SetB));
                setterFunctions.emplace("Alpha", WrapSetter<decltype(&Data::ColorType::SetA)>::Callback("a", &Data::ColorType::SetA));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::Plane>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("Normal", WrapGetter<decltype(&Data::PlaneType::GetNormal)>::Callback("normal", &Data::PlaneType::GetNormal));
                getterFunctions.emplace("Distance", WrapGetter<decltype(&Data::PlaneType::GetDistance)>::Callback("distance", &Data::PlaneType::GetDistance));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("Normal", WrapSetter<decltype(&Data::PlaneType::SetNormal)>::Callback("normal", &Data::PlaneType::SetNormal));
                setterFunctions.emplace("Distance", WrapSetter<decltype(&Data::PlaneType::SetDistance)>::Callback("distance", &Data::PlaneType::SetDistance));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::Transform>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("X Axis", WrapGetter<decltype(&Data::TransformType::GetBasisX)>::Callback("basisX", &Data::TransformType::GetBasisX));
                getterFunctions.emplace("Y Axis", WrapGetter<decltype(&Data::TransformType::GetBasisY)>::Callback("basisY", &Data::TransformType::GetBasisY));
                getterFunctions.emplace("Z Axis", WrapGetter<decltype(&Data::TransformType::GetBasisZ)>::Callback("basisZ", &Data::TransformType::GetBasisZ));
                getterFunctions.emplace("Translation", WrapGetter<decltype(&Data::TransformType::GetTranslation)>::Callback("translation", &Data::TransformType::GetTranslation));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("translation", WrapSetter<void(AZ::Transform::*)(const AZ::Vector3&)>::Callback("translation", &Data::TransformType::SetTranslation));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::AABB>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("Min", WrapGetter<decltype(&Data::AABBType::GetMin)>::Callback("min", &Data::AABBType::GetMin));
                getterFunctions.emplace("Max", WrapGetter<decltype(&Data::AABBType::GetMax)>::Callback("max", &Data::AABBType::GetMax));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("Min", WrapSetter<decltype(&Data::AABBType::SetMin)>::Callback("min", &Data::AABBType::SetMin));
                setterFunctions.emplace("Max", WrapSetter<decltype(&Data::AABBType::SetMax)>::Callback("max", &Data::AABBType::SetMax));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::OBB>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("X Axis", WrapGetter<decltype(&Data::OBBType::GetAxisX)>::Callback("axisX", &Data::OBBType::GetAxisX));
                getterFunctions.emplace("Y Axis", WrapGetter<decltype(&Data::OBBType::GetAxisY)>::Callback("axisY", &Data::OBBType::GetAxisY));
                getterFunctions.emplace("Z Axis", WrapGetter<decltype(&Data::OBBType::GetAxisZ)>::Callback("axisZ", &Data::OBBType::GetAxisZ));
                getterFunctions.emplace("X Half Length", WrapGetter<decltype(&Data::OBBType::GetHalfLengthX)>::Callback("halfLengthX", &Data::OBBType::GetHalfLengthX));
                getterFunctions.emplace("Y Half Length", WrapGetter<decltype(&Data::OBBType::GetHalfLengthY)>::Callback("halfLengthY", &Data::OBBType::GetHalfLengthY));
                getterFunctions.emplace("Z Half Length", WrapGetter<decltype(&Data::OBBType::GetHalfLengthZ)>::Callback("halfLengthZ", &Data::OBBType::GetHalfLengthZ));
                getterFunctions.emplace("Position", WrapGetter<decltype(&Data::OBBType::GetPosition)>::Callback("position", &Data::OBBType::GetPosition));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("X Half Length", WrapSetter<decltype(&Data::OBBType::SetHalfLengthX)>::Callback("halfLengthX", &Data::OBBType::SetHalfLengthX));
                setterFunctions.emplace("Y Half Length", WrapSetter<decltype(&Data::OBBType::SetHalfLengthY)>::Callback("halfLengthY", &Data::OBBType::SetHalfLengthY));
                setterFunctions.emplace("Z Half Length", WrapSetter<decltype(&Data::OBBType::SetHalfLengthZ)>::Callback("halfLengthZ", &Data::OBBType::SetHalfLengthZ));
                setterFunctions.emplace("Position", WrapSetter<decltype(&Data::OBBType::SetPosition)>::Callback("position", &Data::OBBType::SetPosition));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::Matrix3x3>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("X Axis", WrapGetter<decltype(&Data::Matrix3x3Type::GetBasisX)>::Callback("basisX", &Data::Matrix3x3Type::GetBasisX));
                getterFunctions.emplace("Y Axis", WrapGetter<decltype(&Data::Matrix3x3Type::GetBasisY)>::Callback("basisY", &Data::Matrix3x3Type::GetBasisY));
                getterFunctions.emplace("Z Axis", WrapGetter<decltype(&Data::Matrix3x3Type::GetBasisZ)>::Callback("basisZ", &Data::Matrix3x3Type::GetBasisZ));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("X Axis", WrapSetter<void(AZ::Matrix3x3::*)(const AZ::Vector3&)>::Callback("basisX", &Data::Matrix3x3Type::SetBasisX));
                setterFunctions.emplace("Y Axis", WrapSetter<void(AZ::Matrix3x3::*)(const AZ::Vector3&)>::Callback("basisY", &Data::Matrix3x3Type::SetBasisY));
                setterFunctions.emplace("Z Axis", WrapSetter<void(AZ::Matrix3x3::*)(const AZ::Vector3&)>::Callback("basisZ", &Data::Matrix3x3Type::SetBasisZ));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::Matrix4x4>
        {
            static GetterContainer GetGetterWrappers(const Data::Type&)
            {
                GetterContainer getterFunctions;
                getterFunctions.emplace("X Axis", WrapGetter<decltype(&Data::Matrix4x4Type::GetBasisX)>::Callback("basisX", &Data::Matrix4x4Type::GetBasisX));
                getterFunctions.emplace("Y Axis", WrapGetter<decltype(&Data::Matrix4x4Type::GetBasisY)>::Callback("basisY", &Data::Matrix4x4Type::GetBasisY));
                getterFunctions.emplace("Z Axis", WrapGetter<decltype(&Data::Matrix4x4Type::GetBasisZ)>::Callback("basisZ", &Data::Matrix4x4Type::GetBasisZ));
                getterFunctions.emplace("Translation", WrapGetter<decltype(&Data::Matrix4x4Type::GetTranslation)>::Callback("translation", &Data::Matrix4x4Type::GetTranslation));
                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type&)
            {
                SetterContainer setterFunctions;
                setterFunctions.emplace("X Axis", WrapSetter<void(AZ::Matrix4x4::*)(const AZ::Vector4&)>::Callback("basisX", &Data::Matrix4x4Type::SetBasisX));
                setterFunctions.emplace("Y Axis", WrapSetter<void(AZ::Matrix4x4::*)(const AZ::Vector4&)>::Callback("basisY", &Data::Matrix4x4Type::SetBasisY));
                setterFunctions.emplace("Z Axis", WrapSetter<void(AZ::Matrix4x4::*)(const AZ::Vector4&)>::Callback("basisZ", &Data::Matrix4x4Type::SetBasisZ));
                setterFunctions.emplace("Translation", WrapSetter<void(AZ::Matrix4x4::*)(const AZ::Vector3&)>::Callback("translation", &Data::Matrix4x4Type::SetTranslation));
                return setterFunctions;
            }
        };

        template<>
        struct PropertyTraits<Data::eType::BehaviorContextObject>
        {
            static GetterContainer GetGetterWrappers(const Data::Type& type)
            {
                GetterContainer getterFunctions;
                auto behaviorProperties = GetBehaviorProperties(type);
                for (auto&& behaviorProperty : behaviorProperties)
                {
                    AZ::BehaviorMethod* getterMethod = behaviorProperty->m_getter;
                    if (getterMethod && getterMethod->HasResult() && getterMethod->GetNumArguments() == 1)
                    {
                        GetterFunction getterFunction = [&behaviorProperty, getterMethod](const Datum& thisDatum) -> AZ::Outcome<Datum, AZStd::string>
                        {
                            const size_t maxGetterArguments = 1;
                            AZStd::array<AZ::BehaviorValueParameter, maxGetterArguments> getterParams;
                            const AZ::BehaviorParameter* thisParam = getterMethod->GetArgument(0);
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> thisObjectParam = thisDatum.ToBehaviorValueParameter(*thisParam);
                            if (!thisObjectParam.IsSuccess())
                            {
                                return AZ::Failure(AZStd::string::format("BehaviorContextObject %s couldn't be turned into a BehaviorValueParameter for getter: %s",
                                    Data::GetName(thisDatum.GetType()).data(),
                                    thisObjectParam.GetError().data()));
                            }
                            getterParams[0].Set(thisObjectParam.TakeValue());
                            auto behaviorMethodInvokeOutcome = Datum::CallBehaviorContextMethodResult(getterMethod, getterMethod->GetResult(), getterParams.begin(),
                                static_cast<AZ::u32>(getterMethod->GetNumArguments()), behaviorProperty->m_name);
                            if (!behaviorMethodInvokeOutcome)
                            {
                                return AZ::Failure(AZStd::string::format("Attempting to invoke getter method %s failed: %s", getterMethod->m_name.data(), behaviorMethodInvokeOutcome.GetError().data()));
                            }

                            return behaviorMethodInvokeOutcome;
                        };

                        const AZ::BehaviorParameter* resultParam = getterMethod->GetResult();

                        Data::Type resultType = Data::FromAZType(resultParam->m_typeId);

                        if (AZ::BehaviorContextHelper::IsStringParameter((*resultParam)))
                        {
                            resultType = Data::Type::String();
                        }

                        getterFunctions.insert({ behaviorProperty->m_name, { AZStd::move(getterFunction), resultType, behaviorProperty->m_name } });
                    }
                }

                return getterFunctions;
            }

            static SetterContainer GetSetterWrappers(const Data::Type& type)
            {
                enum : size_t { thisIndex = 0U };
                enum : size_t { propertyIndex = 1U };
                SetterContainer setterFunctions;
                auto behaviorProperties = GetBehaviorProperties(type);
                for (auto&& behaviorProperty : behaviorProperties)
                {
                    AZ::BehaviorMethod* setterMethod = behaviorProperty->m_setter;
                    if (setterMethod && setterMethod->GetNumArguments() == 2)
                    {
                        SetterFunction setterFunction = [setterMethod](Datum& thisDatum, const Datum& propertyDatum) -> AZ::Outcome<void, AZStd::string>
                        {
                            const size_t maxSetterArguments = 2;
                            AZStd::array<AZ::BehaviorValueParameter, maxSetterArguments> setterParams;
                            const AZ::BehaviorParameter* thisParam = setterMethod->GetArgument(thisIndex);
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> thisObjectParam = thisDatum.ToBehaviorValueParameter(*thisParam);
                            if (!thisObjectParam.IsSuccess())
                            {
                                return AZ::Failure(AZStd::string::format("BehaviorContextObject %s couldn't be turned into a BehaviorValueParameter for setter: %s",
                                    Data::GetName(thisDatum.GetType()).data(),
                                    thisObjectParam.GetError().data()));
                            }
                            setterParams[thisIndex].Set(thisObjectParam.TakeValue());

                            const AZ::BehaviorParameter* propertyParam = setterMethod->GetArgument(propertyIndex);
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> propertyObjectParam = propertyDatum.ToBehaviorValueParameter(*propertyParam);
                            if (!propertyObjectParam.IsSuccess())
                            {
                                return AZ::Failure(AZStd::string::format("Property type %s couldn't be turned into a BehaviorValueParameter for setter. BehaviorContextObject %s will not be set: %s.",
                                    Data::GetName(propertyDatum.GetType()).data(),
                                    Data::GetName(thisDatum.GetType()).data(),
                                    propertyObjectParam.GetError().data()));
                            }

                            setterParams[propertyIndex].Set(propertyObjectParam.GetValue());

                            if (!setterMethod->Call(setterParams.data(), static_cast<AZ::u32>(setterParams.size())))
                            {
                                return AZ::Failure(AZStd::string::format("Attempting to invoke setter method %s failed", setterMethod->m_name.data()));
                            }

                            return AZ::Success();
                        };

                        const AZ::BehaviorParameter* argumentParam = setterMethod->GetArgument(propertyIndex);

                        Data::Type argumentType = Data::FromAZType(argumentParam->m_typeId);

                        if (AZ::BehaviorContextHelper::IsStringParameter((*argumentParam)))
                        {
                            argumentType = Data::Type::String();
                        }

                        setterFunctions.insert({ behaviorProperty->m_name, { AZStd::move(setterFunction), argumentType, behaviorProperty->m_name } });
                    }
                }

                return setterFunctions;
            }
        };

        namespace Properties
        {
            struct TypeErasedPropertyTraits
            {
                AZ_CLASS_ALLOCATOR(TypeErasedPropertyTraits, AZ::SystemAllocator, 0);

                TypeErasedPropertyTraits() = default;

                template<typename Traits>
                explicit TypeErasedPropertyTraits(Traits)
                {
                    m_getterFunctionCB = &Traits::GetGetterWrappers;
                    m_setterFunctionCB = &Traits::GetSetterWrappers;
                }

                bool m_isTransient = false;

                GetterContainer GetGetterWrappers(const Data::Type& type) const { return m_getterFunctionCB ? m_getterFunctionCB(type) : GetterContainer{}; }
                SetterContainer GetSetterWrappers(const Data::Type& type) const { return m_setterFunctionCB ? m_setterFunctionCB(type) : SetterContainer{}; }

                using GetterContainerCreator = GetterContainer(*)(const Data::Type&);
                GetterContainerCreator m_getterFunctionCB = nullptr;
                using SetterContainerCreator = SetterContainer(*)(const Data::Type&);
                SetterContainerCreator m_setterFunctionCB = nullptr;
            };

            template<eType scTypeValue>
            static TypeErasedPropertyTraits MakeTypeErasedPropertyTraits()
            {
                return TypeErasedPropertyTraits(PropertyTraits<scTypeValue>{});
            }
        }
    } 
} 
