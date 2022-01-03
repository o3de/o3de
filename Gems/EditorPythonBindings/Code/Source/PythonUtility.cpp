/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/PythonUtility.h>
#include <Source/PythonProxyObject.h>
#include <Source/PythonTypeCasters.h>
#include <Source/PythonMarshalComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
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
                const AZ::TypeId& underlyingTypeId = AZ::Internal::GetUnderlyingTypeId(*behaviorParameter.m_azRtti);
                if (underlyingTypeId != behaviorParameter.m_typeId)
                {
                    return AZStd::make_optional(underlyingTypeId);
                }
            }
            return AZStd::nullopt;
        }

        template <typename T>
        bool ConvertPythonFromEnumClass(const AZ::TypeId& underlyingTypeId, AZ::BehaviorValueParameter& behaviorValue, AZ::s64& outboundPythonValue)
        {
            if (underlyingTypeId == AZ::AzTypeInfo<T>::Uuid())
            {
                outboundPythonValue = aznumeric_cast<AZ::s64>(*behaviorValue.GetAsUnsafe<T>());
                return true;
            }
            return false;
        }

        AZStd::optional<pybind11::object> ConvertFromEnumClass(AZ::BehaviorValueParameter& behaviorValue)
        {
            if (!behaviorValue.m_azRtti)
            {
                return AZStd::nullopt;
            }
            const AZ::TypeId& underlyingTypeId = AZ::Internal::GetUnderlyingTypeId(*behaviorValue.m_azRtti);
            if (underlyingTypeId != behaviorValue.m_typeId)
            {
                AZ::s64 outboundPythonValue = 0;

                bool converted =
                    ConvertPythonFromEnumClass<AZ::u8>(underlyingTypeId, behaviorValue, outboundPythonValue)  ||
                    ConvertPythonFromEnumClass<AZ::u16>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::u32>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::u64>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::s8>(underlyingTypeId, behaviorValue, outboundPythonValue)  ||
                    ConvertPythonFromEnumClass<AZ::s16>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::s32>(underlyingTypeId, behaviorValue, outboundPythonValue) ||
                    ConvertPythonFromEnumClass<AZ::s64>(underlyingTypeId, behaviorValue, outboundPythonValue);

                AZ_Error("python", converted, "Enumeration backed by a non-numeric integer type.");
                return converted ? AZStd::make_optional(pybind11::cast<AZ::s64>(outboundPythonValue)) : AZStd::nullopt;
            }
            return AZStd::nullopt;
        }

        template <typename T>
        bool ConvertBehaviorParameterEnum(pybind11::object obj, const AZ::TypeId& underlyingTypeId, AZ::BehaviorValueParameter& parameter)
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

        bool ConvertEnumClassFromPython(pybind11::object obj, const AZ::BehaviorParameter& behaviorArgument, AZ::BehaviorValueParameter& parameter)
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
                        ConvertBehaviorParameterEnum<AZ::u8>(obj, underlyingTypeId, parameter)  ||
                        ConvertBehaviorParameterEnum<AZ::u16>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::u32>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::u64>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::s8>(obj, underlyingTypeId, parameter)  ||
                        ConvertBehaviorParameterEnum<AZ::s16>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::s32>(obj, underlyingTypeId, parameter) ||
                        ConvertBehaviorParameterEnum<AZ::s64>(obj, underlyingTypeId, parameter) ;

                    AZ_Error("python", handled, "Enumeration backed by a non-numeric integer type.");
                    return handled;
                }
            }
            return false;
        }

        // type checks
        bool IsPrimitiveType(const AZ::TypeId& typeId)
        {
            return (typeId == AZ::AzTypeInfo<bool>::Uuid()              ||
                    typeId == AZ::AzTypeInfo<char>::Uuid()              ||
                    typeId == AZ::AzTypeInfo<float>::Uuid()             ||
                    typeId == AZ::AzTypeInfo<double>::Uuid()            ||
                    typeId == AZ::AzTypeInfo<AZ::s8>::Uuid()            ||
                    typeId == AZ::AzTypeInfo<AZ::u8>::Uuid()            ||
                    typeId == AZ::AzTypeInfo<AZ::s16>::Uuid()           ||
                    typeId == AZ::AzTypeInfo<AZ::u16>::Uuid()           ||
                    typeId == AZ::AzTypeInfo<AZ::s32>::Uuid()           ||
                    typeId == AZ::AzTypeInfo<AZ::u32>::Uuid()           ||
                    typeId == AZ::AzTypeInfo<AZ::s64>::Uuid()           ||
                    typeId == AZ::AzTypeInfo<AZ::u64>::Uuid() );
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

        bool AllocateBehaviorValueParameter(const AZ::BehaviorMethod* behaviorMethod, AZ::BehaviorValueParameter& result, Convert::StackVariableAllocator& stackVariableAllocator)
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
                    AZ_Error("python", behaviorClass, "A behavior class is missing for %s!",
                        resultType->m_typeId.ToString<AZStd::string>().c_str());
                }
            }
            return false;
        }

        void DeallocateBehaviorValueParameter(AZ::BehaviorValueParameter& valueParameter)
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

        // from Python to BehaviorValueParameter

        bool PythonProxyObjectToBehaviorValueParameter(const AZ::BehaviorParameter& behaviorArgument, pybind11::object pyObj, AZ::BehaviorValueParameter& parameter)
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
            AZ::BehaviorValueParameter& outBehavior,
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

        bool PythonToBehaviorValueParameter(const AZ::BehaviorParameter& behaviorArgument, pybind11::object pyObj, AZ::BehaviorValueParameter& parameter, Convert::StackVariableAllocator& stackVariableAllocator)
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

        // from BehaviorValueParameter to Python

        AZStd::optional<pybind11::object> CustomBehaviorToPython(AZ::BehaviorValueParameter& behaviorValue, Convert::StackVariableAllocator& stackVariableAllocator)
        {
            AZStd::optional<CustomTypeBindingNotifications::ValueHandle> handle;
            PyObject* outPyObj = nullptr;
            CustomTypeBindingNotificationBus::EventResult(
                handle,
                behaviorValue.m_typeId,
                &CustomTypeBindingNotificationBus::Events::BehaviorToPython,
                behaviorValue,
                outPyObj);

            if (outPyObj != nullptr && handle)
            {
                Internal::StoreVariableCustomTypeDeleter(handle.value(), behaviorValue.m_typeId, stackVariableAllocator);
                return { pybind11::reinterpret_borrow<pybind11::object>(outPyObj) };
            }

            return AZStd::nullopt;
        }

        pybind11::object BehaviorValueParameterToPython(AZ::BehaviorValueParameter& behaviorValue, Convert::StackVariableAllocator& stackVariableAllocator)
        {
            auto pyValue = Internal::ConvertFromEnumClass(behaviorValue);
            if (pyValue.has_value())
            {
                return pyValue.value();
            }

            AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> result;
            PythonMarshalTypeRequestBus::EventResult(result, behaviorValue.m_typeId, &PythonMarshalTypeRequestBus::Events::BehaviorValueParameterToPython, behaviorValue);
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
        using BehaviorMethodArgumentArray = AZStd::array<AZ::BehaviorValueParameter, MaxBehaviorMethodArguments>;

        pybind11::object InvokeBehaviorMethodWithResult(AZ::BehaviorMethod* behaviorMethod, pybind11::args pythonInputArgs, AZ::BehaviorObject self, AZ::BehaviorValueParameter& result)
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
                AZ::BehaviorValueParameter theThisPointer;
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
                            behaviorMethod->m_name.c_str(), parameterCount,
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
            AZ::BehaviorValueParameter result;
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
    }
}
