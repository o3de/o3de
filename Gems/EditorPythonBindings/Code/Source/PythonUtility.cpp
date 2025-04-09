/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzCore/Script/ScriptContextAttributes.h"
#include <EditorPythonBindings/PythonUtility.h>
#include <Source/PythonMarshalComponent.h>
#include <Source/PythonProxyObject.h>
#include <Source/PythonTypeCasters.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/sort.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EditorPythonBindings/CustomTypeBindingBus.h>
#include <pybind11/embed.h>

namespace EditorPythonBindings
{
    namespace Module
    {
        pybind11::module DeterminePackageModule(PackageMapType& modulePackageMap, AZStd::string_view moduleName, pybind11::module parentModule, pybind11::module fallbackModule, [[maybe_unused]] bool alertUsingFallback)
        {
            if (moduleName.empty() || !moduleName[0])
            {
                AZ_Warning("python", !alertUsingFallback, "Could not determine missing or empty module; using fallback module");
                return fallbackModule;
            }
            else if (parentModule.is_none())
            {
                AZ_Warning("python", !alertUsingFallback, "Could not determine using None parent module; using fallback module");
                return fallbackModule;
            }

            AZStd::string parentModuleName(PyModule_GetName(parentModule.ptr()));
            modulePackageMap[parentModuleName] = parentModule;
            pybind11::module currentModule = parentModule;

            AZStd::string fullModuleName(parentModuleName);
            fullModuleName.append(".");
            fullModuleName.append(moduleName);

            AZStd::vector<AZStd::string> moduleParts;
            AzFramework::StringFunc::Tokenize(fullModuleName.c_str(), moduleParts, ".", false, false);
            for (int modulePartsIndex = 0; modulePartsIndex < moduleParts.size(); ++modulePartsIndex)
            {
                AZStd::string currentModulePath;
                AzFramework::StringFunc::Join(currentModulePath, moduleParts.begin(), moduleParts.begin() + modulePartsIndex + 1, ".");

                auto itPackageEntry = modulePackageMap.find(currentModulePath.c_str());
                if (itPackageEntry != modulePackageMap.end())
                {
                    currentModule = itPackageEntry->second;
                }
                else
                {
                    PyObject* newModule = PyImport_AddModule(currentModulePath.c_str());
                    if (!newModule)
                    {
                        AZ_Warning("python", false, "Could not add module named %s; using fallback module", currentModulePath.c_str());
                        return fallbackModule;
                    }
                    else
                    {
                        auto newSubModule = pybind11::reinterpret_borrow<pybind11::module>(newModule);
                        modulePackageMap[currentModulePath] = newSubModule;
                        const char* subModuleName = moduleParts[modulePartsIndex].c_str();
                        currentModule.attr(subModuleName) = newSubModule;
                        currentModule = newSubModule;
                    }
                }
            }
            return currentModule;
        }
    }

    namespace Internal
    {
        void LogSerializeTypeInfo(const AZ::TypeId& typeId)
        {
            AZStd::string info = AZStd::string::format("Serialize class info for typeId %s (", typeId.ToString<AZStd::string>().c_str());

            AZ::SerializeContext* serializeContext{ nullptr };
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            if (serializeContext)
            {
                auto&& classInfo = serializeContext->FindClassData(typeId);
                if (classInfo)
                {
                    info = AZStd::string::format("name:%s version:%d isContainer:%s",
                        classInfo->m_name, classInfo->m_version, classInfo->m_container ? "true" : "false");
                }

                auto&& genericClassInfo = serializeContext->FindGenericClassInfo(typeId);
                if (genericClassInfo)
                {
                    info += " generic:true";
                    info += AZStd::string::format(" specialized typeId: %s",
                        genericClassInfo->GetSpecializedTypeId().ToString<AZStd::string>().c_str());
                    info += AZStd::string::format(" generic typeId: %s",
                        genericClassInfo->GetGenericTypeId().ToString<AZStd::string>().c_str());
                    size_t numTemplatedArguments = genericClassInfo->GetNumTemplatedArguments();
                    info += AZStd::string::format(" template arguments %zu", genericClassInfo->GetNumTemplatedArguments());
                    for (size_t index = 0; index < numTemplatedArguments; ++index)
                    {
                        info += AZStd::string::format(" [%zu] template type: %s",
                            index,
                            genericClassInfo->GetTemplatedTypeId(index).ToString<AZStd::string>().c_str());
                    }
                }
            }
            info += ")";
            AZ_Warning("python", false, "Serialize generic class info %s", info.c_str());
        }

        AZStd::optional<AZ::TypeId> IsEnumClass(const AZ::BehaviorParameter& behaviorParameter)
        {
            if (behaviorParameter.m_azRtti)
            {
                // If the underlying type of the supplied type is different, then T is an enum
                AZ::TypeId underlyingTypeId = AZ::Internal::GetUnderlyingTypeId(*behaviorParameter.m_azRtti);
                if (underlyingTypeId != behaviorParameter.m_typeId)
                {
                    return AZStd::make_optional(underlyingTypeId);
                }
            }
            return AZStd::nullopt;
        }

        template <typename T>
        bool ConvertPythonFromEnumClass(const AZ::TypeId& underlyingTypeId, AZ::BehaviorArgument& behaviorValue, AZ::s64& outboundPythonValue)
        {
            if (underlyingTypeId == AZ::AzTypeInfo<T>::Uuid())
            {
                outboundPythonValue = aznumeric_cast<AZ::s64>(*behaviorValue.GetAsUnsafe<T>());
                return true;
            }
            return false;
        }

        AZStd::optional<pybind11::object> ConvertFromEnumClass(AZ::BehaviorArgument& behaviorValue)
        {
            if (!behaviorValue.m_azRtti)
            {
                return AZStd::nullopt;
            }
            AZ::TypeId underlyingTypeId = AZ::Internal::GetUnderlyingTypeId(*behaviorValue.m_azRtti);
            if (underlyingTypeId != behaviorValue.m_typeId)
            {
                AZ::s64 outboundPythonValue = 0;

                bool converted = 
                    ConvertPythonFromEnumClass<long>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<unsigned long>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::u8>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::u16>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::u32>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::u64>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::s8>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::s16>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::s32>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::s64>(underlyingTypeId, behaviorValue, outboundPythonValue);

                AZ_Error("python", converted, "Enumeration backed by a non-numeric integer type.");
                return converted ? AZStd::make_optional(pybind11::cast<AZ::s64>(AZStd::move(outboundPythonValue))) : AZStd::nullopt;
            }
            return AZStd::nullopt;
        }

        template <typename T>
        bool ConvertBehaviorParameterEnum(pybind11::object obj, const AZ::TypeId& underlyingTypeId, AZ::BehaviorArgument& parameter)
        {
            if (underlyingTypeId == AZ::AzTypeInfo<T>::Uuid())
            {
                void* value = parameter.m_tempData.allocate(sizeof(T), AZStd::alignment_of<T>::value, 0);
                *reinterpret_cast<T*>(value) = pybind11::cast<T>(obj);

                if (parameter.m_traits & AZ::BehaviorParameter::TR_POINTER)
                {
                    *reinterpret_cast<void**>(parameter.m_value) = reinterpret_cast<T*>(&value);
                }
                else
                {
                    parameter.m_value = value;
                }
                return true;
            }
            return false;
        }

        bool ConvertEnumClassFromPython(
            pybind11::object obj, const AZ::BehaviorParameter& behaviorArgument, AZ::BehaviorArgument& parameter)
        {
            if (behaviorArgument.m_azRtti)
            {
                // If the underlying type of the supplied type is different, then T is an enum
                const AZ::TypeId underlyingTypeId = AZ::Internal::GetUnderlyingTypeId(*behaviorArgument.m_azRtti);
                if (underlyingTypeId != behaviorArgument.m_typeId)
                {
                    parameter.m_name = behaviorArgument.m_name;
                    parameter.m_azRtti = behaviorArgument.m_azRtti;
                    parameter.m_traits = behaviorArgument.m_traits;
                    parameter.m_typeId = behaviorArgument.m_typeId;

                    bool handled = 
                        ConvertBehaviorParameterEnum<long>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<unsigned long>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::u8>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::u16>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::u32>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::u64>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::s8>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::s16>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::s32>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::s64>(obj, underlyingTypeId, parameter);

                    AZ_Error("python", handled, "Enumeration backed by a non-numeric integer type.");
                    return handled;
                }
            }
            return false;
        }

        // type checks
        bool IsPrimitiveType(const AZ::TypeId& typeId)
        {
            return (
                typeId == AZ::AzTypeInfo<bool>::Uuid() ||
                typeId == AZ::AzTypeInfo<char>::Uuid() ||
                typeId == AZ::AzTypeInfo<float>::Uuid() ||
                typeId == AZ::AzTypeInfo<double>::Uuid() ||
                typeId == AZ::AzTypeInfo<long>::Uuid() ||
                typeId == AZ::AzTypeInfo<unsigned long>::Uuid() ||
                typeId == AZ::AzTypeInfo<AZ::s8>::Uuid() ||
                typeId == AZ::AzTypeInfo<AZ::u8>::Uuid() ||
                typeId == AZ::AzTypeInfo<AZ::s16>::Uuid() ||
                typeId == AZ::AzTypeInfo<AZ::u16>::Uuid() ||
                typeId == AZ::AzTypeInfo<AZ::s32>::Uuid() ||
                typeId == AZ::AzTypeInfo<AZ::u32>::Uuid() ||
                typeId == AZ::AzTypeInfo<AZ::s64>::Uuid() ||
                typeId == AZ::AzTypeInfo<AZ::u64>::Uuid());
        }

        bool IsPointerType(const AZ::u32 traits)
        {
            return (((traits & AZ::BehaviorParameter::TR_POINTER) == AZ::BehaviorParameter::TR_POINTER) ||
                ((traits & AZ::BehaviorParameter::TR_REFERENCE) == AZ::BehaviorParameter::TR_REFERENCE));
        }

        // allocation patterns

        void StoreVariableCustomTypeDeleter(
            CustomTypeBindingNotifications::ValueHandle handle,
            AZ::TypeId typeId,
            Convert::StackVariableAllocator& stackVariableAllocator)
        {
            auto deallocateValue = [typeId = std::move(typeId), handle]() mutable
            {
                CustomTypeBindingNotificationBus::Event(
                    typeId,
                    &CustomTypeBindingNotificationBus::Events::CleanUpValue,
                    handle);
            };
            stackVariableAllocator.StoreVariableDeleter(deallocateValue);
        }

        bool AllocateBehaviorObjectByClass(const AZ::BehaviorClass* behaviorClass, AZ::BehaviorObject& behaviorObject)
        {
            if (behaviorClass)
            {
                if (behaviorClass->m_defaultConstructor)
                {
                    AZ::BehaviorObject newBehaviorObject = behaviorClass->Create();
                    behaviorObject.m_typeId = newBehaviorObject.m_typeId;
                    behaviorObject.m_address = newBehaviorObject.m_address;
                    return true;
                }
                else
                {
                    AZ_Warning("python", behaviorClass->m_defaultConstructor, "Missing default constructor for AZ::BehaviorClass for typeId:%s", behaviorClass->m_name.c_str());
                }
            }
            return false;
        }

        bool AllocateBehaviorValueParameter(const AZ::BehaviorMethod* behaviorMethod, AZ::BehaviorArgument& result, Convert::StackVariableAllocator& stackVariableAllocator)
        {
            if (const AZ::BehaviorParameter* resultType = behaviorMethod->GetResult())
            {
                result.Set(*resultType);

                if (auto underlyingTypeId = Internal::IsEnumClass(result); underlyingTypeId)
                {
                    result.m_typeId = underlyingTypeId.value();
                }

                if (resultType->m_traits & AZ::BehaviorParameter::TR_POINTER)
                {
                    result.m_value = result.m_tempData.allocate(sizeof(intptr_t), alignof(intptr_t));
                    return true;
                }

                if (resultType->m_traits & AZ::BehaviorParameter::TR_REFERENCE)
                {
                    return true;
                }

                if (IsPrimitiveType(resultType->m_typeId))
                {
                    result.m_value = result.m_tempData.allocate(sizeof(intptr_t), alignof(intptr_t));
                    return true;
                }

                AZ::BehaviorContext* behaviorContext(nullptr);
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Assert(false, "A behavior context is required!");
                    return false;
                }

                const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(behaviorContext, resultType->m_typeId);
                if (behaviorClass)
                {
                    AZ::BehaviorObject behaviorObject;
                    if (AllocateBehaviorObjectByClass(behaviorClass, behaviorObject))
                    {
                        result.m_value = behaviorObject.m_address;
                        result.m_typeId = resultType->m_typeId;
                        return true;
                    }
                }
                else
                {
                    CustomTypeBindingNotifications::AllocationHandle allocationHandleResult;
                    CustomTypeBindingNotificationBus::EventResult(
                        allocationHandleResult,
                        result.m_typeId,
                        &CustomTypeBindingNotificationBus::Events::AllocateDefault);

                    if (allocationHandleResult)
                    {
                        CustomTypeBindingNotifications::ValueHandle handle = allocationHandleResult.value().first;
                        const AZ::BehaviorObject& behaviorObject = allocationHandleResult.value().second;
                        StoreVariableCustomTypeDeleter(handle, behaviorObject.m_typeId, stackVariableAllocator);
                        result.m_value = behaviorObject.m_address;
                        result.m_typeId = behaviorObject.m_typeId;
                        return true;
                    }

                    // So far no allocation scheme has been found for this typeId, but the SerializeContext might have more information
                    // so this code tries to pull out more type information about the typeId so that the user can get more human readable
                    // information than a UUID
                    LogSerializeTypeInfo(resultType->m_typeId);
                    AZ_Error("python", behaviorClass, "A behavior class for method %s is missing for type '%s' (%s)!",
                        behaviorMethod->m_name.c_str(),
                        resultType->m_name,
                        resultType->m_typeId.ToString<AZStd::string>().c_str());
                }
            }
            return false;
        }

        void DeallocateBehaviorValueParameter(AZ::BehaviorArgument& valueParameter)
        {
            if (IsPointerType(valueParameter.m_traits) || IsPrimitiveType(valueParameter.m_typeId))
            {
                // no constructor was used
                return;
            }

            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Assert(false, "A behavior context is required!");
            }

            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(behaviorContext, valueParameter.m_typeId);
            if (behaviorClass)
            {
                AZ::BehaviorObject behaviorObject;
                behaviorObject.m_address = valueParameter.m_value;
                behaviorObject.m_typeId = valueParameter.m_typeId;
                behaviorClass->Destroy(behaviorObject);
            }
        }
    }

    namespace Convert
    {
        // StackVariableAllocator

        StackVariableAllocator::~StackVariableAllocator()
        {
            for (auto& cleanUp : m_cleanUpItems)
            {
                cleanUp();
            }
        }

        void StackVariableAllocator::StoreVariableDeleter(VariableDeleter&& deleter)
        {
            m_cleanUpItems.emplace_back(deleter);
        }

        // from Python to BehaviorArgument

        bool PythonProxyObjectToBehaviorValueParameter(const AZ::BehaviorParameter& behaviorArgument, pybind11::object pyObj, AZ::BehaviorArgument& parameter)
        {
            auto behaviorObject = pybind11::cast<EditorPythonBindings::PythonProxyObject*>(pyObj)->GetBehaviorObject();
            if (behaviorObject)
            {
                const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(behaviorObject.value()->m_typeId);
                if (!behaviorClass)
                {
                    AZ_Warning("python", false, "Missing BehaviorClass for typeId %s", behaviorObject.value()->m_typeId.ToString<AZStd::string>().c_str());
                    return false;
                }

                if (behaviorClass->m_azRtti)
                {
                    // is exact type or can be down casted?
                    if (!behaviorClass->m_azRtti->IsTypeOf(behaviorArgument.m_typeId))
                    {
                        return false;
                    }
                }
                else if (behaviorObject.value()->m_typeId != behaviorArgument.m_typeId)
                {
                    // type mismatch detected
                    return false;
                }

                if ((behaviorArgument.m_traits & AZ::BehaviorParameter::TR_POINTER) == AZ::BehaviorParameter::TR_POINTER)
                {
                    parameter.m_value = &behaviorObject.value()->m_address;
                }
                else
                {
                    parameter.m_value = behaviorObject.value()->m_address;
                }
                parameter.m_typeId = behaviorClass->m_typeId;
                parameter.m_azRtti = behaviorClass->m_azRtti;
                parameter.m_traits = behaviorArgument.m_traits;
                parameter.m_name = behaviorArgument.m_name;
                return true;
            }
            return false;
        }

        bool CustomPythonToBehavior(
            const AZ::BehaviorParameter& behaviorArgument,
            pybind11::object pyObj,
            AZ::BehaviorArgument& outBehavior,
            StackVariableAllocator& stackVariableAllocator)
        {
            AZStd::optional<CustomTypeBindingNotifications::ValueHandle> handle;
            CustomTypeBindingNotificationBus::EventResult(
                handle,
                behaviorArgument.m_typeId,
                &CustomTypeBindingNotificationBus::Events::PythonToBehavior,
                pyObj.ptr(),
                static_cast<AZ::BehaviorParameter::Traits>(behaviorArgument.m_traits),
                outBehavior);

            if (handle)
            {
                Internal::StoreVariableCustomTypeDeleter(handle.value(), behaviorArgument.m_typeId, stackVariableAllocator);
                outBehavior.m_typeId = behaviorArgument.m_typeId;
                outBehavior.m_traits = behaviorArgument.m_traits;
                outBehavior.m_name = behaviorArgument.m_name;
                outBehavior.m_azRtti = behaviorArgument.m_azRtti;
                return true;
            }
            return false;
        }

        bool PythonToBehaviorValueParameter(const AZ::BehaviorParameter& behaviorArgument, pybind11::object pyObj, AZ::BehaviorArgument& parameter, Convert::StackVariableAllocator& stackVariableAllocator)
        {
            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> result;
            PythonMarshalTypeRequests::BehaviorTraits traits = static_cast<PythonMarshalTypeRequests::BehaviorTraits>(behaviorArgument.m_traits);
            PythonMarshalTypeRequestBus::EventResult(result, behaviorArgument.m_typeId, &PythonMarshalTypeRequestBus::Events::PythonToBehaviorValueParameter, traits, pyObj, parameter);
            if (result && result.value().first)
            {
                auto deleter = AZStd::move(result.value().second);
                if (deleter)
                {
                    stackVariableAllocator.StoreVariableDeleter(AZStd::move(deleter));
                }
                parameter.m_typeId = behaviorArgument.m_typeId;
                parameter.m_traits = behaviorArgument.m_traits;
                parameter.m_name = behaviorArgument.m_name;
                parameter.m_azRtti = behaviorArgument.m_azRtti;
                return true;
            }
            else if (auto underlyingTypeId = Internal::IsEnumClass(behaviorArgument); underlyingTypeId)
            {
                AZ::BehaviorParameter tempArg;
                tempArg.m_azRtti = behaviorArgument.m_azRtti;
                tempArg.m_traits = behaviorArgument.m_traits;
                tempArg.m_name = behaviorArgument.m_name;
                tempArg.m_typeId = underlyingTypeId.value();
                if (PythonToBehaviorValueParameter(tempArg, pyObj, parameter, stackVariableAllocator))
                {
                    parameter.m_typeId = behaviorArgument.m_typeId;
                    return true;
                }
            }
            else if (pybind11::isinstance<EditorPythonBindings::PythonProxyObject>(pyObj))
            {
                return PythonProxyObjectToBehaviorValueParameter(behaviorArgument, pyObj, parameter);
            }
            else if (CustomPythonToBehavior(behaviorArgument, pyObj, parameter, stackVariableAllocator))
            {
                return true;
            }
            return false;
        }

        // from BehaviorArgument to Python

        AZStd::optional<pybind11::object> CustomBehaviorToPython(AZ::BehaviorArgument& behaviorValue, Convert::StackVariableAllocator& stackVariableAllocator)
        {
            AZStd::optional<CustomTypeBindingNotifications::ValueHandle> handle;
            PyObject* outPyObj = nullptr;
            CustomTypeBindingNotificationBus::EventResult(
                handle,
                behaviorValue.m_typeId,
                &CustomTypeBindingNotificationBus::Events::BehaviorToPython, behaviorValue, outPyObj);

            if (outPyObj != nullptr && handle)
            {
                Internal::StoreVariableCustomTypeDeleter(handle.value(), behaviorValue.m_typeId, stackVariableAllocator);
                return { pybind11::reinterpret_borrow<pybind11::object>(outPyObj) };
            }

            return AZStd::nullopt;
        }

        pybind11::object BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue, Convert::StackVariableAllocator& stackVariableAllocator)
        {
            auto pyValue = Internal::ConvertFromEnumClass(behaviorValue);
            if (pyValue.has_value())
            {
                return pyValue.value();
            }

            AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> result;
            PythonMarshalTypeRequestBus::EventResult(
                result, behaviorValue.m_typeId, &PythonMarshalTypeRequestBus::Events::BehaviorValueParameterToPython, behaviorValue);
            if (result.has_value())
            {
                auto deleter = AZStd::move(result.value().second);
                if (deleter)
                {
                    stackVariableAllocator.StoreVariableDeleter(AZStd::move(deleter));
                }
                return result.value().first;
            }
            else if (auto customResult = CustomBehaviorToPython(behaviorValue, stackVariableAllocator); customResult)
            {
                return customResult.value();
            }
            else if (behaviorValue.m_typeId != AZ::Uuid::CreateNull() && behaviorValue.GetValueAddress())
            {
                return PythonProxyObjectManagement::CreatePythonProxyObject(behaviorValue.m_typeId, behaviorValue.GetValueAddress());
            }
            AZ_Warning("python", false, "Cannot convert type %s",
                behaviorValue.m_name ? behaviorValue.m_name : behaviorValue.m_typeId.ToString<AZStd::string>().c_str());
            return pybind11::cast<pybind11::none>(Py_None);
        }

        AZStd::string GetPythonTypeName(pybind11::object pyObj)
        {
            if (pybind11::isinstance<PythonProxyObject>(pyObj))
            {
                return pybind11::cast<PythonProxyObject*>(pyObj)->GetWrappedTypeName();
            }
            return pybind11::cast<AZStd::string>(pybind11::str(pyObj.get_type()));
        }
    }

    namespace Call
    {
        constexpr size_t MaxBehaviorMethodArguments = 32;
        using BehaviorMethodArgumentArray = AZStd::array<AZ::BehaviorArgument, MaxBehaviorMethodArguments>;

        pybind11::object InvokeBehaviorMethodWithResult(AZ::BehaviorMethod* behaviorMethod, pybind11::args pythonInputArgs, AZ::BehaviorObject self, AZ::BehaviorArgument& result)
        {
            if (behaviorMethod->GetNumArguments() > MaxBehaviorMethodArguments || pythonInputArgs.size() > MaxBehaviorMethodArguments)
            {
                AZ_Error("python", false, "Too many arguments for class method; set:%zu max:%zu", behaviorMethod->GetMinNumberOfArguments(), MaxBehaviorMethodArguments);
                return pybind11::cast<pybind11::none>(Py_None);
            }

            Convert::StackVariableAllocator stackVariableAllocator;
            BehaviorMethodArgumentArray parameters;
            int parameterCount = 0;
            if (self.IsValid())
            {
                // record the "this" pointer's metadata like its RTTI so that it can be
                // down casted to a parent class type if needed to invoke a parent method
                AZ::BehaviorArgument theThisPointer;
                if (const AZ::BehaviorParameter* thisInfo = behaviorMethod->GetArgument(0))
                {
                    // avoiding the "Special handling for the generic object holder." since it assumes
                    // the BehaviorObject.m_value is a pointer; the reference version is already dereferenced
                    if ((thisInfo->m_traits & AZ::BehaviorParameter::TR_POINTER) == AZ::BehaviorParameter::TR_POINTER)
                    {
                        theThisPointer.m_value = &self.m_address;
                    }
                    else
                    {
                        theThisPointer.m_value = self.m_address;
                    }
                    theThisPointer.Set(*thisInfo);
                    parameters[0].Set(theThisPointer);
                    ++parameterCount;
                }
                else
                {
                    AZ_Warning("python", false, "Missing self info index 0 in class method %s", behaviorMethod->m_name.c_str());
                    return pybind11::cast<pybind11::none>(Py_None);
                }
            }

            // prepare the parameters for the BehaviorMethod
            for (auto pythonArg : pythonInputArgs)
            {
                if (parameterCount < behaviorMethod->GetNumArguments())
                {
                    auto currentPythonArg = pybind11::cast<pybind11::object>(pythonArg);
                    const AZ::BehaviorParameter* behaviorArgument = behaviorMethod->GetArgument(parameterCount);
                    if (!behaviorArgument)
                    {
                        AZ_Warning("python", false, "Missing argument at index %d in class method %s", parameterCount, behaviorMethod->m_name.c_str());
                        return pybind11::cast<pybind11::none>(Py_None);
                    }
                    if (!Convert::PythonToBehaviorValueParameter(*behaviorArgument, currentPythonArg, parameters[parameterCount], stackVariableAllocator))
                    {
                        AZ_Warning("python", false, "BehaviorMethod %s: Parameter at [%d] index expects (%s:%s) for method but got type (%s)",
                            behaviorMethod->m_name.c_str(),
                            parameterCount,
                            behaviorArgument->m_name, behaviorArgument->m_typeId.ToString<AZStd::string>().c_str(),
                            Convert::GetPythonTypeName(currentPythonArg).c_str());
                        return pybind11::cast<pybind11::none>(Py_None);
                    }
                    ++parameterCount;
                }
            }

            // did the Python script send the right amount of arguments?
            const auto totalPythonArgs = pythonInputArgs.size() + (self.IsValid() ? 1 : 0); // +1 for the 'this' coming in from a marshaled Python/BehaviorObject
            if (totalPythonArgs < behaviorMethod->GetMinNumberOfArguments())
            {
                AZ_Warning("python", false, "Method %s requires at least %zu parameters got %zu", behaviorMethod->m_name.c_str(), behaviorMethod->GetMinNumberOfArguments(), totalPythonArgs);
                return pybind11::cast<pybind11::none>(Py_None);
            }
            else if (totalPythonArgs > behaviorMethod->GetNumArguments())
            {
                AZ_Warning("python", false, "Method %s requires %zu parameters but it got more (%zu) - excess parameters will not be used.", behaviorMethod->m_name.c_str(), behaviorMethod->GetMinNumberOfArguments(), totalPythonArgs);
            }

            if (behaviorMethod->HasResult())
            {
                if (Internal::AllocateBehaviorValueParameter(behaviorMethod, result, stackVariableAllocator))
                {
                    if (behaviorMethod->Call(parameters.begin(), static_cast<unsigned int>(totalPythonArgs), &result))
                    {
                        result.m_azRtti = behaviorMethod->GetResult()->m_azRtti;
                        result.m_typeId = behaviorMethod->GetResult()->m_typeId;
                        result.m_traits = behaviorMethod->GetResult()->m_traits;
                        return Convert::BehaviorValueParameterToPython(result, stackVariableAllocator);
                    }
                    else
                    {
                        AZ_Warning("python", false, "Failed to call class method %s", behaviorMethod->m_name.c_str());
                    }
                }
                else
                {
                    AZ_Warning("python", false, "Failed to allocate return value for method %s", behaviorMethod->m_name.c_str());
                }
            }
            else if (!behaviorMethod->Call(parameters.begin(), static_cast<unsigned int>(totalPythonArgs)))
            {
                AZ_Warning("python", false, "Failed to invoke class method %s", behaviorMethod->m_name.c_str());
            }
            return pybind11::cast<pybind11::none>(Py_None);
        }

        pybind11::object InvokeBehaviorMethod(AZ::BehaviorMethod* behaviorMethod, pybind11::args pythonInputArgs, AZ::BehaviorObject self)
        {
            AZ::BehaviorArgument result;
            result.m_value = nullptr;
            pybind11::object pythonOutput = InvokeBehaviorMethodWithResult(behaviorMethod, pythonInputArgs, self, result);
            if (result.m_value)
            {
                Internal::DeallocateBehaviorValueParameter(result);
            }

            return pythonOutput;
        }

        pybind11::object StaticMethod(AZ::BehaviorMethod* behaviorMethod, pybind11::args pythonInputArgs)
        {
            return InvokeBehaviorMethod(behaviorMethod, pythonInputArgs, {});
        }

        pybind11::object ClassMethod(AZ::BehaviorMethod* behaviorMethod, AZ::BehaviorObject self, pybind11::args pythonInputArgs)
        {
            if (behaviorMethod->GetNumArguments() == 0)
            {
                AZ_Error("python", false, "A member level function should require at least one argument");
            }
            else if (!self.IsValid())
            {
                AZ_Error("python", false, "Method %s requires at valid self object to invoke", behaviorMethod->m_name.c_str());
            }
            else
            {
                return InvokeBehaviorMethod(behaviorMethod, pythonInputArgs, self);
            }
            return pybind11::cast<pybind11::none>(Py_None);
        }
    } // namespace Call

    namespace Text
    {
        namespace Internal
        {
            AZStd::string ReadStringAttribute(const AZ::AttributeArray& attributes, const AZ::Crc32& attribute)
            {
                AZStd::string attributeValue = "";
                if (auto attributeItem = azrtti_cast<AZ::AttributeData<AZStd::string>*>(AZ::FindAttribute(attribute, attributes)))
                {
                    attributeValue = attributeItem->Get(nullptr);
                    return attributeValue;
                }

                if (auto attributeItem = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(attribute, attributes)))
                {
                    attributeValue = attributeItem->Get(nullptr);
                    return attributeValue;
                }
                return {};
            }

            AZStd::string TypeNameFallback(const AZ::TypeId& typeId)
            {
                // fall back to class data m_name
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

                if (serializeContext)
                {
                    const auto classData = serializeContext->FindClassData(typeId);
                    if (classData)
                    {
                        return classData->m_name;
                    }
                }

                return "";
            }

            void Indent(const int level, AZStd::string& buffer)
            {
                buffer.append(level * 4, ' ');
            }

            void AddCommentBlock(int level, const AZStd::string& comment, AZStd::string& buffer)
            {
                Indent(level, buffer);
                AzFramework::StringFunc::Append(buffer, "\"\"\"\n");
                Indent(level, buffer);
                AzFramework::StringFunc::Append(buffer, comment.c_str());
                Indent(level, buffer);
                AzFramework::StringFunc::Append(buffer, "\"\"\"\n");
            }
        } // namespace Internal

        AZStd::string PythonBehaviorDescription::FetchListType(const AZ::TypeId& typeId)
        {
            AZStd::string type = "list";

            AZStd::vector<AZ::Uuid> typeList = AZ::Utils::GetContainedTypes(typeId);
            if (!typeList.empty())
            {
                // trait info not available, so defaulting to TR_NONE
                AZStd::string_view itemType = FetchPythonTypeAndTraits(typeList[0], AZ::BehaviorParameter::TR_NONE);
                if (!itemType.empty())
                {
                    type = AZStd::string::format("List[" AZ_STRING_FORMAT "]", AZ_STRING_ARG(itemType));
                }
            }

            return type;
        }

        AZStd::string PythonBehaviorDescription::FetchMapType(const AZ::TypeId& typeId)
        {
            AZStd::string type = "dict";

            AZStd::vector<AZ::Uuid> typeList = AZ::Utils::GetContainedTypes(typeId);
            if (!typeList.empty())
            {
                // trait info not available, so defaulting to TR_NONE
                AZStd::string_view kType = FetchPythonTypeAndTraits(typeList[0], AZ::BehaviorParameter::TR_NONE);
                AZStd::string_view vType = FetchPythonTypeAndTraits(typeList[1], AZ::BehaviorParameter::TR_NONE);
                if (!kType.empty() && !vType.empty())
                {
                    type = AZStd::string::format(
                        "Dict[" AZ_STRING_FORMAT ", " AZ_STRING_FORMAT "]", AZ_STRING_ARG(kType), AZ_STRING_ARG(vType));
                }
            }

            return type;
        }

        AZStd::string_view PythonBehaviorDescription::FetchPythonTypeAndTraits(const AZ::TypeId& typeId, AZ::u32 traits)
        {
            if (m_typeCache.find(typeId) == m_typeCache.end())
            {
                AZStd::string type;
                if (AZ::AzTypeInfo<AZStd::string_view>::Uuid() == typeId || AZ::AzTypeInfo<AZStd::string>::Uuid() == typeId)
                {
                    type = "str";
                }
                else if (
                    AZ::AzTypeInfo<char>::Uuid() == typeId && traits & AZ::BehaviorParameter::TR_POINTER &&
                    traits & AZ::BehaviorParameter::TR_CONST)
                {
                    type = "str";
                }
                else if (AZ::AzTypeInfo<float>::Uuid() == typeId || AZ::AzTypeInfo<double>::Uuid() == typeId)
                {
                    type = "float";
                }
                else if (AZ::AzTypeInfo<bool>::Uuid() == typeId)
                {
                    type = "bool";
                }
                else if (
                    AZ::AzTypeInfo<long>::Uuid() == typeId || AZ::AzTypeInfo<unsigned long>::Uuid() == typeId ||
                    AZ::AzTypeInfo<AZ::s8>::Uuid() == typeId || AZ::AzTypeInfo<AZ::u8>::Uuid() == typeId ||
                    AZ::AzTypeInfo<AZ::s16>::Uuid() == typeId || AZ::AzTypeInfo<AZ::u16>::Uuid() == typeId ||
                    AZ::AzTypeInfo<AZ::s32>::Uuid() == typeId || AZ::AzTypeInfo<AZ::u32>::Uuid() == typeId ||
                    AZ::AzTypeInfo<AZ::s64>::Uuid() == typeId || AZ::AzTypeInfo<AZ::u64>::Uuid() == typeId)
                {
                    type = "int";
                }
                else if (AZ::AzTypeInfo<AZStd::vector<AZ::u8>>::Uuid() == typeId)
                {
                    type = "bytes";
                }
                else if (AZ::AzTypeInfo<AZStd::any>::Uuid() == typeId)
                {
                    type = "object";
                }
                else if (AZ::AzTypeInfo<void>::Uuid() == typeId)
                {
                    type = "None";
                }
                else if (AZ::Utils::IsVectorContainerType(typeId))
                {
                    type = FetchListType(typeId);
                }
                else if (AZ::Utils::IsMapContainerType(typeId))
                {
                    type = FetchMapType(typeId);
                }
                else if (AZ::Utils::IsOutcomeType(typeId))
                {
                    type = FetchOutcomeType(typeId);
                }
                else
                {
                    type = Internal::TypeNameFallback(typeId);
                }

                m_typeCache[typeId] = type;
            }

            return m_typeCache[typeId];
        }

        AZStd::string PythonBehaviorDescription::FetchPythonTypeName(const AZ::BehaviorParameter& param)
        {
            AZStd::string pythonType = FetchPythonTypeAndTraits(param.m_typeId, param.m_traits);

            if (pythonType.empty())
            {
                if (AZ::StringFunc::Equal(param.m_name, "void"))
                {
                    return "None";
                }

                return param.m_name;
            }
            return pythonType;
        }

        AZStd::string PythonBehaviorDescription::FetchOutcomeType(const AZ::TypeId& typeId)
        {
            AZStd::string type = "Outcome";
            AZStd::pair<AZ::Uuid, AZ::Uuid> outcomeTypes = AZ::Utils::GetOutcomeTypes(typeId);

            // trait info not available, so defaulting to TR_NONE
            AZStd::string_view valueT = FetchPythonTypeAndTraits(outcomeTypes.first, AZ::BehaviorParameter::TR_NONE);
            AZStd::string_view errorT = FetchPythonTypeAndTraits(outcomeTypes.second, AZ::BehaviorParameter::TR_NONE);
            if (!valueT.empty() && !errorT.empty())
            {
                type = AZStd::string::format(
                    "Outcome[" AZ_STRING_FORMAT ", " AZ_STRING_FORMAT "]", AZ_STRING_ARG(valueT), AZ_STRING_ARG(errorT));
            }

            return type;
        }

        //! Creates a string containing bus events and documentation.
        AZStd::string PythonBehaviorDescription::BusDefinition(const AZStd::string_view& busName, const AZ::BehaviorEBus* behaviorEBus)
        {
            AZStd::string buffer;
            if (!behaviorEBus || behaviorEBus->m_events.empty())
            {
                return buffer;
            }

            const auto& eventSenderEntry = behaviorEBus->m_events.begin();
            const AZ::BehaviorEBusEventSender& sender = eventSenderEntry->second;

            AzFramework::StringFunc::Append(buffer, "def ");
            AzFramework::StringFunc::Append(buffer, busName.data());
            bool isBroadcast = false;
            if (sender.m_event)
            {
                AZStd::string addressType = FetchPythonTypeName(behaviorEBus->m_idParam);
                if (addressType.empty())
                {
                    AzFramework::StringFunc::Append(buffer, "(busCallType: int, busEventName: str, address: Any, args: Tuple[Any])");
                }
                else
                {
                    AzFramework::StringFunc::Append(buffer, "(busCallType: int, busEventName: str, address: ");
                    AzFramework::StringFunc::Append(buffer, AZStd::string::format(AZ_STRING_FORMAT, AZ_STRING_ARG(addressType)).c_str());
                    AzFramework::StringFunc::Append(buffer, ", args: Tuple[Any])");
                }
            }
            else
            {
                AzFramework::StringFunc::Append(buffer, "(busCallType: int, busEventName: str, args: Tuple[Any])");
                isBroadcast = true;
            }
            AzFramework::StringFunc::Append(buffer, " -> Any:\n");

            auto eventInfoBuilder =
                [this](const AZ::BehaviorMethod* behaviorMethod, AZStd::string& inOutStrBuffer, [[maybe_unused]] TypeMap& typeCache)
            {
                AzFramework::StringFunc::Append(inOutStrBuffer, "(");

                size_t numArguments = behaviorMethod->GetNumArguments();

                const AZ::BehaviorParameter* busIdArg = behaviorMethod->GetBusIdArgument();

                for (size_t i = 0; i < numArguments; ++i)
                {
                    const AZ::BehaviorParameter* argParam = behaviorMethod->GetArgument(i);
                    if (argParam == busIdArg)
                    {
                        // address argument is part of the bus call, skip from event argument list
                        continue;
                    }

                    AZStd::string_view argType = FetchPythonTypeAndTraits(argParam->m_typeId, argParam->m_traits);
                    AzFramework::StringFunc::Append(inOutStrBuffer, argType.data());

                    if (i < (numArguments - 1))
                    {
                        AzFramework::StringFunc::Append(inOutStrBuffer, ", ");
                    }
                }

                const AZ::BehaviorParameter* resultParam = behaviorMethod->GetResult();
                AZStd::string returnType = FetchPythonTypeName(*resultParam);
                AZStd::string returnTypeStr = AZStd::string::format(") -> " AZ_STRING_FORMAT " \n", AZ_STRING_ARG(returnType));
                AzFramework::StringFunc::Append(inOutStrBuffer, returnTypeStr.c_str());
            };

            // record the event names the behavior can send, their parameters and return type
            AZStd::string comment = behaviorEBus->m_toolTip;
            if (comment.empty())
            {
                comment = Internal::ReadStringAttribute(behaviorEBus->m_attributes, AZ::Script::Attributes::ToolTip);
            }

            if (!behaviorEBus->m_events.empty())
            {
                AzFramework::StringFunc::Append(
                    comment, "The following bus Call types, Event names and Argument types are supported by this bus:\n");
                AZStd::vector<AZStd::string> events;
                for (const auto& eventSenderEntry2 : behaviorEBus->m_events)
                {
                    const AZStd::string& eventName = eventSenderEntry2.first;
                    AZStd::string eventNameStr = AZStd::string::format("'%s', ", eventName.c_str());

                    // prefer m_event info over m_broadcast
                    if (!isBroadcast && eventSenderEntry2.second.m_event != nullptr)
                    {
                        AZStd::string eventInfo;
                        AzFramework::StringFunc::Append(eventInfo, "bus.Event, ");
                        AzFramework::StringFunc::Append(eventInfo, eventNameStr.c_str());
                        eventInfoBuilder(eventSenderEntry2.second.m_event, eventInfo, m_typeCache);
                        events.push_back(eventInfo);
                    }
                    else if (isBroadcast && eventSenderEntry2.second.m_broadcast != nullptr)
                    {
                        AZStd::string eventInfo;
                        AzFramework::StringFunc::Append(eventInfo, "bus.Broadcast, ");
                        AzFramework::StringFunc::Append(eventInfo, eventNameStr.c_str());
                        eventInfoBuilder(eventSenderEntry2.second.m_broadcast, eventInfo, m_typeCache);
                        events.push_back(eventInfo);
                    }
                    else
                    {
                        AZ_Warning("python", false, "Event %s is expected to have valid event information.", eventName.c_str());
                    }
                }

                AZStd::sort(events.begin(), events.end());

                for (auto& eventInfo : events)
                {
                    Internal::Indent(1, comment);
                    AzFramework::StringFunc::Append(comment, eventInfo.c_str());
                }
            }

            Internal::AddCommentBlock(1, comment, buffer);

            Internal::Indent(1, buffer);
            AzFramework::StringFunc::Append(buffer, "pass\n\n");

            // can the EBus create & destroy a handler?
            if (behaviorEBus->m_createHandler && behaviorEBus->m_destroyHandler)
            {
                AzFramework::StringFunc::Append(buffer, "def ");
                AzFramework::StringFunc::Append(buffer, busName.data());
                AzFramework::StringFunc::Append(buffer, "Handler() -> None:\n");
                Internal::Indent(1, buffer);
                AzFramework::StringFunc::Append(buffer, "pass\n\n");
            }
            return buffer;
        }

        //! Creates a string with class or global method definition and documentation.
        AZStd::string PythonBehaviorDescription::MethodDefinition(
            const AZStd::string_view& methodName,
            const AZ::BehaviorMethod& behaviorMethod,
            const AZ::BehaviorClass* behaviorClass,
            bool defineTooltip,
            bool defineDebugDescription)
        {
            AZStd::string buffer;
            AZStd::vector<AZStd::string> pythonArgs;
            const bool isMemberLike =
                behaviorClass ? PythonProxyObjectManagement::IsMemberLike(behaviorMethod, behaviorClass->m_typeId) : false;

            int indentLevel = 0;
            if (isMemberLike)
            {
                indentLevel = 1;
                Internal::Indent(indentLevel, buffer);
                pythonArgs.emplace_back("self");
            }

            AzFramework::StringFunc::Append(buffer, "def ");
            if (isMemberLike || !behaviorClass)
            {
                AzFramework::StringFunc::Append(buffer, methodName.data());
            }
            else
            {
                AzFramework::StringFunc::Append(buffer, behaviorClass->m_name.c_str());
                AzFramework::StringFunc::Append(buffer, "_");
                AzFramework::StringFunc::Append(buffer, methodName.data());
            }
            AzFramework::StringFunc::Append(buffer, "(");

            AZStd::string bufferArg;
            for (size_t argIndex = 0; argIndex < behaviorMethod.GetNumArguments(); ++argIndex)
            {
                const AZStd::string* name = behaviorMethod.GetArgumentName(argIndex);
                if (!name || name->empty())
                {
                    bufferArg = AZStd::string::format(" arg%zu", argIndex);
                }
                else
                {
                    bufferArg = *name;
                }

                AZStd::string type = FetchPythonTypeName(*behaviorMethod.GetArgument(argIndex));
                if (!type.empty())
                {
                    AzFramework::StringFunc::Append(bufferArg, ": ");
                    AzFramework::StringFunc::Append(bufferArg, type.data());
                }

                pythonArgs.push_back(bufferArg);
                bufferArg.clear();
            }

            AZStd::string argsList;
            AzFramework::StringFunc::Join(buffer, pythonArgs.begin(), pythonArgs.end(), ",");
            AzFramework::StringFunc::Append(buffer, ") -> None:\n");

            AZStd::string methodTooltipAndDebugDescription = "";

            if (defineDebugDescription && behaviorMethod.m_debugDescription != nullptr)
            {
                AZStd::string debugDescription(behaviorMethod.m_debugDescription);
                if (!debugDescription.empty())
                {
                    methodTooltipAndDebugDescription += debugDescription;
                    methodTooltipAndDebugDescription += "\n";
                }
            }
            if (defineTooltip)
            {
                AZStd::string methodTooltip = Internal::ReadStringAttribute(behaviorMethod.m_attributes, AZ::Script::Attributes::ToolTip);
                if (!methodTooltip.empty())
                {
                    methodTooltipAndDebugDescription += methodTooltip;
                    methodTooltipAndDebugDescription += "\n";
                }
            }
            if (!methodTooltipAndDebugDescription.empty())
            {
                Internal::AddCommentBlock(indentLevel + 1, methodTooltipAndDebugDescription, buffer);
            }

            Internal::Indent(indentLevel + 1, buffer);
            AzFramework::StringFunc::Append(buffer, "pass\n\n");
            return buffer;
        }

        AZStd::string PythonBehaviorDescription::ClassDefinition(
            const AZ::BehaviorClass* behaviorClass,
            const AZStd::string_view& className,
            bool defineProperties,
            bool defineMethods,
            bool defineTooltip)
        {
            AZStd::string buffer;
            AzFramework::StringFunc::Append(buffer, "class ");
            AzFramework::StringFunc::Append(buffer, className.data());
            AzFramework::StringFunc::Append(buffer, ":\n");

            if (behaviorClass->m_methods.empty() && behaviorClass->m_properties.empty())
            {
                AZStd::string body;
                if (defineProperties && defineMethods)
                {
                    body = "    # behavior class type with no methods or properties \n";
                }
                else if (defineProperties)
                {
                    body = "    # behavior class type with no properties \n";
                }
                else if (defineMethods)
                {
                    body = "    # behavior class type with no methods \n";
                }
                else
                {
                    body = "";
                }
                if (defineTooltip)
                {
                    AZStd::string classTooltip =
                        Internal::ReadStringAttribute(behaviorClass->m_attributes, AZ::Script::Attributes::ToolTip);
                    if (!classTooltip.empty())
                    {
                        Internal::AddCommentBlock(1, classTooltip, body);
                    }
                }

                Internal::Indent(1, body);
                AzFramework::StringFunc::Append(body, "pass\n\n");
                AzFramework::StringFunc::Append(buffer, body.c_str());
            }
            else
            {
                if (defineProperties)
                {
                    for (const auto& propertyEntry : behaviorClass->m_properties)
                    {
                        AZ::BehaviorProperty* property = propertyEntry.second;
                        AZStd::string propertyName{ propertyEntry.first };
                        Scope::FetchScriptName(property->m_attributes, propertyName);
                        AZStd::string propertyDef = PropertyDefinition(propertyName, 1, *property, behaviorClass);
                        AzFramework::StringFunc::Append(buffer, propertyDef.c_str());
                    }
                }

                if (defineMethods)
                {
                    for (const auto& methodEntry : behaviorClass->m_methods)
                    {
                        AZ::BehaviorMethod* method = methodEntry.second;
                        if (method && PythonProxyObjectManagement::IsMemberLike(*method, behaviorClass->m_typeId))
                        {
                            AZStd::string baseMethodName{ methodEntry.first };
                            Scope::FetchScriptName(method->m_attributes, baseMethodName);
                            AZStd::string methodDef = MethodDefinition(baseMethodName, *method, behaviorClass, defineTooltip);
                            AzFramework::StringFunc::Append(buffer, methodDef.c_str());
                        }
                    }
                }
            }

            return buffer;
        }

        AZStd::string PythonBehaviorDescription::PropertyDefinition(
            AZStd::string_view propertyName, int level, const AZ::BehaviorProperty& property, [[maybe_unused]] const AZ::BehaviorClass* behaviorClass)
        {
            AZStd::string buffer;
            Internal::Indent(level, buffer);
            AzFramework::StringFunc::Append(buffer, "@property\n");

            Internal::Indent(level, buffer);
            AzFramework::StringFunc::Append(buffer, "def ");
            AzFramework::StringFunc::Append(buffer, propertyName.data());
            AzFramework::StringFunc::Append(buffer, "(self) -> ");

            AZStd::string_view type = FetchPythonTypeAndTraits(property.GetTypeId(), AZ::BehaviorParameter::TR_NONE);
            if (type.empty())
            {
                AzFramework::StringFunc::Append(buffer, "Any");
            }
            else
            {
                AzFramework::StringFunc::Append(buffer, type.data());
            }
            AzFramework::StringFunc::Append(buffer, ":\n");
            Internal::Indent(level + 1, buffer);
            AzFramework::StringFunc::Append(buffer, "pass\n\n");
            return buffer;
        }

        AZStd::string PythonBehaviorDescription::GlobalPropertyDefinition(
            [[maybe_unused]] const AZStd::string_view& moduleName,
            const AZStd::string_view& propertyName,
            const AZ::BehaviorProperty& behaviorProperty,
            bool needsHeader)
        {
            AZStd::string buffer;

            // add header
            if (needsHeader)
            {
                AzFramework::StringFunc::Append(buffer, "class property():\n");
            }

            Internal::Indent(1, buffer);
            AzFramework::StringFunc::Append(buffer, propertyName.data());
            AzFramework::StringFunc::Append(buffer, ": ClassVar[");

            const AZ::BehaviorParameter* resultParam = behaviorProperty.m_getter->GetResult();
            AZStd::string_view type = FetchPythonTypeAndTraits(resultParam->m_typeId, resultParam->m_traits);
            if (type.empty())
            {
                AzFramework::StringFunc::Append(buffer, "Any");
            }
            else
            {
                AzFramework::StringFunc::Append(buffer, type.data());
            }
            AzFramework::StringFunc::Append(buffer, "] = None");

            if (behaviorProperty.m_getter && !behaviorProperty.m_setter)
            {
                AzFramework::StringFunc::Append(buffer, " # read only");
            }
            AzFramework::StringFunc::Append(buffer, "\n");

            return buffer;
        }
    } // namespace Text
} // namespace EditorPythonBindings
