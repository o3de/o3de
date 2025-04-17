/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorPythonBindings/PythonUtility.h>
#include <Source/PythonMarshalComponent.h>
#include <Source/PythonMarshalTuple.h>
#include <Source/PythonProxyObject.h>
#include <Source/PythonTypeCasters.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/std/allocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <EditorPythonBindings/PythonCommon.h>
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>

namespace EditorPythonBindings
{
    bool IsPointerType(PythonMarshalTypeRequests::BehaviorTraits traits)
    {
        return (((traits & AZ::BehaviorParameter::TR_POINTER) == AZ::BehaviorParameter::TR_POINTER) ||
                ((traits & AZ::BehaviorParameter::TR_REFERENCE) == AZ::BehaviorParameter::TR_REFERENCE));
    }

    template <typename TInput, typename TPythonType>
    pybind11::object MarshalBehaviorValueParameter(AZ::BehaviorArgument& result)
    {
        if (result.ConvertTo<TInput>())
        {
            TInput inputValue = static_cast<TInput>(*result.GetAsUnsafe<TInput>());
            return TPythonType(inputValue);
        }
        return pybind11::cast<pybind11::none>(Py_None);
    }

    void ReportMissingTypeId(AZ::TypeId typeId)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(typeId);
        if (behaviorClass)
        {
            AZ_Warning("python", false, "Missing BehaviorClass for UUID:%s Name:%s", typeId.ToString<AZStd::string>().c_str(), behaviorClass->m_name.c_str());
            return;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Error("python", serializeContext, "SerializeContext is missing");
        if (!serializeContext)
        {
            AZ_Warning("python", false, "Missing Serialize class for UUID:%s", typeId.ToString<AZStd::string>().c_str());
            return;
        }

        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId);
        if(!classData)
        {
            AZ_Warning("python", false, "Missing Serialize class for UUID:%s", typeId.ToString<AZStd::string>().c_str());
        }
        else if (classData->m_container)
        {
            AZ_Warning("python", false, "Missing Serialize class container for UUID:%s Name:%s", typeId.ToString<AZStd::string>().c_str(), classData->m_name);
        }
        else
        {
            AZ_Warning("python", false, "Missing Serialize class for UUID:%s Name:%s", typeId.ToString<AZStd::string>().c_str(), classData->m_name);
        }
    }

    class TypeConverterAny
        : public PythonMarshalComponent::TypeConverter
    {
    public:
        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
        {
            if (!CanConvertPythonToBehaviorValue(traits, pyObj))
            {
                AZ_Warning("python", false, "AZStd::any<> handles Behavior Class types only.");
                return AZStd::nullopt;
            }

            if ((traits & AZ::BehaviorParameter::TR_POINTER) == AZ::BehaviorParameter::TR_POINTER)
            {
                AZ_Warning("python", false, "AZStd::any* pointer argument types are not supported; try 'AZStd::any' value or 'const AZStd::any&' instead");
                return AZStd::nullopt;
            }

            if (pybind11::isinstance<EditorPythonBindings::PythonProxyObject>(pyObj))
            {
                EditorPythonBindings::PythonProxyObject* proxyObj = pybind11::cast<EditorPythonBindings::PythonProxyObject*>(pyObj);
                if (proxyObj)
                {
                    return PythonToParameterWithProxy(proxyObj, pyObj, outValue);
                }
                AZ_Warning("python", false, "Passed in PythonProxyObject is empty.");
                return AZStd::nullopt;
            }
            else if (pyObj.is_none())
            {
                AZStd::any* anyValue = aznew AZStd::any();
                outValue.Set<AZStd::any>(anyValue);

                auto deleteAny = [anyValue]()
                {
                    delete anyValue;
                };
                return AZStd::make_optional(PythonMarshalTypeRequests::BehaviorValueResult{ true, deleteAny });
            }
            else if (PyList_Check(pyObj.ptr()))
            {
                return ReturnVectorFromList(traits, pybind11::cast<pybind11::list>(pyObj), outValue);
            }
            else if (PyBool_Check(pyObj.ptr()))
            {
                return ReturnSimpleType<bool>(Py_True == pyObj.ptr(), outValue);
            }
            else if (PyFloat_Check(pyObj.ptr()))
            {
                return ReturnSimpleType<double>(PyFloat_AsDouble(pyObj.ptr()), outValue);
            }
            else if (PyLong_Check(pyObj.ptr()))
            {
                return ReturnSimpleType<AZ::s64>(PyLong_AsLongLong(pyObj.ptr()), outValue);
            }
            else if (PyUnicode_Check(pyObj.ptr()))
            {
                // in the case of an error, NULL is returned with an exception set and no size is stored
                Py_ssize_t size = -1;
                const char* value = PyUnicode_AsUTF8AndSize(pyObj.ptr(), &size);
                if (value && size != -1)
                {
                    return ReturnSimpleType<AZStd::string_view>(AZStd::string_view(value, size), outValue);
                }
            }
            return AZStd::nullopt;
        }

        AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
        {
            if ((behaviorValue.m_traits & AZ::BehaviorParameter::TR_POINTER) == AZ::BehaviorParameter::TR_POINTER)
            {
                AZ_Warning("python", false, "Return value 'AZStd::any*' pointer argument types are not supported; try returning 'const AZStd::any&' instead");
                return AZStd::nullopt;
            }

            if (!behaviorValue.ConvertTo<AZStd::any>())
            {
                AZ_Warning("python", false, "Cannot convert the return value to a AZStd::any value.");
                return AZStd::nullopt;
            }
            AZStd::any* anyValue = static_cast<AZStd::any*>(behaviorValue.GetAsUnsafe<AZStd::any>());
            const AZ::TypeId anyValueTypId { anyValue->get_type_info().m_id };

            // is a registered convertible type?
            if (PythonMarshalTypeRequestBus::GetNumOfEventHandlers(anyValueTypId))
            {
                AZ::BehaviorArgument tempBehaviorValue;
                tempBehaviorValue.m_typeId = anyValueTypId;
                tempBehaviorValue.m_value = AZStd::any_cast<void>(anyValue);

                AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> result;
                PythonMarshalTypeRequestBus::EventResult(result, anyValueTypId, &PythonMarshalTypeRequestBus::Events::BehaviorValueParameterToPython, tempBehaviorValue);
                return result;
            }
            else
            {
                AZ::BehaviorContext* behaviorContext {nullptr};
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("python", false, "A behavior context is required!");
                    return AZStd::nullopt;
                }

                AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(behaviorContext, anyValueTypId);
                if (behaviorClass)
                {
                    PythonMarshalTypeRequests::PythonValueResult result;
                    result.first = PythonProxyObjectManagement::CreatePythonProxyObject(anyValueTypId, AZStd::any_cast<void>(anyValue));
                    return result;
                }
            }
            return AZStd::nullopt;
        }

        bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
        {
            // supports Python native types None, Float, Long, Bool, List, or String
            if (pyObj.is_none()            ||
                PyFloat_Check(pyObj.ptr()) ||
                PyLong_Check(pyObj.ptr())  ||
                PyBool_Check(pyObj.ptr())  ||
                PyList_Check(pyObj.ptr())  ||
                PyUnicode_Check(pyObj.ptr()))
            {
                return true;
            }
            return pybind11::isinstance<EditorPythonBindings::PythonProxyObject>(pyObj);
        }

    protected:
        template <typename T>
        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> ReturnSimpleType(T value, AZ::BehaviorArgument& outValue)
        {
            AZStd::any* anyValue = aznew AZStd::any(value);
            outValue.Set<AZStd::any>(anyValue);

            auto deleteAny = [anyValue]()
            {
                delete anyValue;
            };
            return AZStd::make_optional(PythonMarshalTypeRequests::BehaviorValueResult{ true, deleteAny });
        }

        AZStd::any* CreateAnyValue(AZ::TypeId typeId, void* address) const
        {
            const AZ::BehaviorClass* sourceClass = AZ::BehaviorContextHelper::GetClass(typeId);
            if (!sourceClass)
            {
                ReportMissingTypeId(typeId);
                return nullptr;
            }

            if (!sourceClass->m_allocate || !sourceClass->m_cloner || !sourceClass->m_mover || !sourceClass->m_destructor || !sourceClass->m_deallocate)
            {
                AZ_Warning("python", false, "BehaviorClass:%s must handle allocation", sourceClass->m_name.c_str());
                return nullptr;
            }

            AZStd::any::type_info valueInfo;
            valueInfo.m_id = typeId;
            valueInfo.m_isPointer = false;
            valueInfo.m_useHeap = true;
            valueInfo.m_handler = [sourceClass](AZStd::any::Action action, AZStd::any* dest, const AZStd::any* source)
            {
                if (action == AZStd::any::Action::Reserve)
                {
                    *reinterpret_cast<void**>(dest) = sourceClass->Allocate();
                }
                else if (action == AZStd::any::Action::Copy)
                {
                    sourceClass->m_cloner(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(source), sourceClass->m_userData);
                }
                else if (action == AZStd::any::Action::Move)
                {
                    sourceClass->m_mover(AZStd::any_cast<void>(dest), AZStd::any_cast<void>(const_cast<AZStd::any*>(source)), sourceClass->m_userData);
                }
                else if (action == AZStd::any::Action::Destroy)
                {
                    sourceClass->Destroy(AZ::BehaviorObject(AZStd::any_cast<void>(dest), sourceClass->m_typeId));
                }
            };

            return aznew AZStd::any(address, valueInfo);
        }

        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToParameterWithProxy(EditorPythonBindings::PythonProxyObject* proxyObj, pybind11::object pyObj, AZ::BehaviorArgument& outValue)
        {
            auto behaviorObject = proxyObj->GetBehaviorObject();
            if (!behaviorObject)
            {
                AZ_Warning("python", false, "Empty behavior object sent in.");
                return AZStd::nullopt;
            }

            AZStd::any* anyValue = CreateAnyValue(behaviorObject.value()->m_typeId, behaviorObject.value()->m_address);
            if (!anyValue)
            {
                return AZStd::nullopt;
            }

            auto deleteAny = [anyValue]()
            {
                delete anyValue;
            };
            outValue.Set<AZStd::any>(anyValue);
            return AZStd::make_optional(PythonMarshalTypeRequests::BehaviorValueResult{ true, deleteAny });
        }

        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> ReturnVectorFromList(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::list pyList, AZ::BehaviorArgument& outValue)
        {
            // empty lists are okay, sending as an empty AZStd::any()
            if (pyList.size() == 0)
            {
                AZStd::any* anyValue = aznew AZStd::any();
                auto deleteAny = [anyValue]()
                {
                    delete anyValue;
                };
                outValue.Set<AZStd::any>(anyValue);
                return AZStd::make_optional(PythonMarshalTypeRequests::BehaviorValueResult{ true, deleteAny });
            }

            // determine the type from the Python type
            pybind11::object pyListElement = pyList[0];

            AZ::TypeId vectorType;
            if (pybind11::isinstance<EditorPythonBindings::PythonProxyObject>(pyListElement))
            {
                // making a AZ::TypeId for a 'AZStd::vector<Element Type Id, AZStd::allocator>'
                // the vector TypeId equals "underlying element type" + "allocator type" + "vector type"
                auto* proxy = pybind11::cast<PythonProxyObject*>(pyListElement);
                if (!proxy || !proxy->GetWrappedType())
                {
                    return AZStd::nullopt;
                }
                constexpr const char AZStdVectorTypeId[] = "{A60E3E61-1FF6-4982-B6B8-9E4350C4C679}";
                vectorType = proxy->GetWrappedType().value();
                vectorType += azrtti_typeid<AZStd::allocator>();
                vectorType += AZ::TypeId(AZStdVectorTypeId);
            }
            else if (PyBool_Check(pyListElement.ptr()))
            {
                vectorType = azrtti_typeid<AZStd::vector<bool>>();
            }
            else if (PyFloat_Check(pyListElement.ptr()))
            {
                vectorType = azrtti_typeid<AZStd::vector<double>>();
            }
            else if (PyNumber_Check(pyListElement.ptr()))
            {
                vectorType = azrtti_typeid<AZStd::vector<AZ::s64>>();
            }
            else if (PyUnicode_Check(pyListElement.ptr()))
            {
                vectorType = azrtti_typeid<AZStd::vector<AZStd::string>>();
            }

            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> vectorResult;
            PythonMarshalTypeRequestBus::EventResult(vectorResult, vectorType, &PythonMarshalTypeRequestBus::Events::PythonToBehaviorValueParameter, traits, pyList, outValue);
            if (vectorResult && vectorResult.value().first)
            {
                AZStd::any* anyValue = CreateAnyValue(vectorType, outValue.m_value);
                if (!anyValue)
                {
                    return AZStd::nullopt;
                }
                outValue.Set<AZStd::any>(anyValue);
                auto deleteAny = [anyValue]()
                {
                    delete anyValue;
                };
                return AZStd::make_optional(PythonMarshalTypeRequests::BehaviorValueResult{ true, deleteAny });
            }
            return AZStd::nullopt;
        }
    };

    class TypeConverterBool
        : public PythonMarshalComponent::TypeConverter
    {
    public:
        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
        {
            if (CanConvertPythonToBehaviorValue(traits,pyObj))
            {
                outValue.StoreInTempData(pybind11::cast<bool>(pyObj));
                return { { true, nullptr } };
            }
            return AZStd::nullopt;
        }

        AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
        {
            PythonMarshalTypeRequests::PythonValueResult result;
            result.first = MarshalBehaviorValueParameter<bool, pybind11::bool_>(behaviorValue);
            return result;
        }

        bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
        {
            return (PyBool_Check(pyObj.ptr()) != false);
        }
    };

    template <typename T>
    class TypeConverterInteger
        : public PythonMarshalComponent::TypeConverter
    {
    public:
        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
        {
            if (CanConvertPythonToBehaviorValue(traits, pyObj))
            {
                outValue.StoreInTempData(pybind11::cast<T>(pyObj));
                return { { true, nullptr } };
            }
            return AZStd::nullopt;
        }

        AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
        {
            PythonMarshalTypeRequests::PythonValueResult result;
            result.first = MarshalBehaviorValueParameter<T, pybind11::int_>(behaviorValue);
            return result;
        }

        bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
        {
            return PyLong_Check(pyObj.ptr()) != false;
        }
    };

    template <typename BehaviorType, typename NativeType, typename PythonType>
    class TypeConverterReal
        : public PythonMarshalComponent::TypeConverter
    {
    public:
        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
        {
            if (CanConvertPythonToBehaviorValue(traits, pyObj))
            {
                NativeType nativeType = pybind11::cast<NativeType>(pyObj);
                outValue.StoreInTempData(BehaviorType{ nativeType });
                return { { true, nullptr } };
            }
            return AZStd::nullopt;
        }

        AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
        {
            PythonMarshalTypeRequests::PythonValueResult result;
            result.first = MarshalBehaviorValueParameter<BehaviorType, pybind11::float_>(behaviorValue);
            return result;
        }

        bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
        {
            return (PyFloat_Check(pyObj.ptr()) != false);
        }
    };

    template <typename T>
    class TypeConverterString
        : public PythonMarshalComponent::TypeConverter
    {
    public:
        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
        {
            if (!CanConvertPythonToBehaviorValue(traits, pyObj))
            {
                return AZStd::nullopt;
            }
            else if (AZ::AzTypeInfo<T>::Uuid() == AZ::AzTypeInfo<AZStd::string_view>::Uuid())
            {
                // in the case of an error, NULL is returned with an exception set and no size is stored
                Py_ssize_t size = -1;
                const char* value = PyUnicode_AsUTF8AndSize(pyObj.ptr(), &size);
                if (value || size != -1)
                {
                    AZStd::string_view stringView(value, size);
                    outValue.StoreInTempData<AZStd::string_view>(AZStd::move(stringView));
                    return { { true, nullptr } };
                }
            }
            else if (AZ::AzTypeInfo<T>::Uuid() == AZ::AzTypeInfo<AZStd::string>::Uuid())
            {
                AZStd::string* stringValue = new AZStd::string(pybind11::cast<AZStd::string>(pyObj));
                outValue.Set<AZStd::string>(stringValue);
                return { {true, [stringValue]() { delete stringValue; }} };
            }
            else if (AZ::AzTypeInfo<T>::Uuid() == AZ::AzTypeInfo<AZ::IO::FixedMaxPathString>::Uuid())
            {
                auto* stringValue = new AZ::IO::FixedMaxPathString(pybind11::cast<AZ::IO::FixedMaxPathString>(pyObj));
                outValue.Set<AZ::IO::FixedMaxPathString>(stringValue);
                return { {true, [stringValue]() { delete stringValue; }} };
            }
            return AZStd::nullopt;
        }

        AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
        {
            PythonMarshalTypeRequests::PythonValueResult result;
            result.first = pybind11::cast(behaviorValue.GetAsUnsafe<T>());
            return result;
        }

        bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
        {
            return(PyUnicode_Check(pyObj.ptr()) != false );
        }
    };

    // The 'char' type can come in with a variety of type traits:
    //
    class TypeConverterChar
        : public PythonMarshalComponent::TypeConverter
    {
    public:
        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
        {
            if (!CanConvertPythonToBehaviorValue(traits, pyObj))
            {
                return AZStd::nullopt;
            }
            // in the case of an error, NULL is returned with an exception set and no size is stored
            Py_ssize_t size = -1;
            const char* value = PyUnicode_AsUTF8AndSize(pyObj.ptr(), &size);
            if (!value || size == -1)
            {
                return AZStd::nullopt;
            }

            if (IsPointerType(traits))
            {
                outValue.StoreInTempData(value);
            }
            else
            {
                outValue.StoreInTempData(value[0]);
            }
            return { { true, nullptr } };
        }

        AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
        {
            if (IsPointerType(static_cast<PythonMarshalTypeRequests::BehaviorTraits>(behaviorValue.m_traits)))
            {
                if (behaviorValue.ConvertTo<const char*>())
                {
                    PythonMarshalTypeRequests::PythonValueResult resultString;
                    resultString.first = pybind11::str(*behaviorValue.GetAsUnsafe<const char*>());
                    return resultString;
                }
            }
            else
            {
                if (behaviorValue.ConvertTo<char>())
                {
                    char characters[2];
                    characters[0] = *behaviorValue.GetAsUnsafe<char>();
                    characters[1] = 0;

                    PythonMarshalTypeRequests::PythonValueResult resultCharNumber;
                    resultCharNumber.first = pybind11::str(characters, 1);
                    return resultCharNumber;
                }
            }
            return AZStd::nullopt;
        }

        bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
        {
            return (PyUnicode_Check(pyObj.ptr()) != false);
        }
    };

    namespace Container
    {
        class TypeConverterByteStream
            : public PythonMarshalComponent::TypeConverter
        {
        public:
            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
            {
                if (!CanConvertPythonToBehaviorValue(traits, pyObj))
                {
                    AZ_Warning("python", false, "Expected a Python List as input");
                    return AZStd::nullopt;
                }

                AZStd::vector<AZ::u8>* newByteStream = new AZStd::vector<AZ::u8>();

                pybind11::list pyList(pyObj);
                for (auto pyItem = pyList.begin(); pyItem != pyList.end(); ++pyItem)
                {
                    AZ::u8 byte = (*pyItem).cast<AZ::u8>();
                    newByteStream->push_back(byte);
                }

                outValue.m_name = "AZStd::vector<AZ::u8>";
                outValue.m_value = newByteStream;
                outValue.m_typeId = AZ::AzTypeInfo<AZStd::vector<AZ::u8>>::Uuid();
                outValue.m_traits = traits;

                auto deleteVector = [newByteStream]()
                {
                    delete newByteStream;
                };
                return { { true, deleteVector } };
            }

            AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
            {
                if (behaviorValue.ConvertTo(AZ::AzTypeInfo<AZStd::vector<AZ::u8>>::Uuid()))
                {
                    pybind11::list pythonList;
                    AZStd::vector<AZ::u8>* byteStream = behaviorValue.GetAsUnsafe<AZStd::vector<AZ::u8>>();
                    for (AZ::u8 byte : *byteStream)
                    {
                        pythonList.append(pybind11::cast(byte));
                    }

                    PythonMarshalTypeRequests::PythonValueResult result;
                    result.first = pythonList;
                    return result;
                }
                return AZStd::nullopt;
            }

            bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
            {
                return (PyList_Check(pyObj.ptr()) != false);
            }
        };

        AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> ProcessBehaviorObject(AZ::BehaviorObject& behaviorObject)
        {
            AZ::BehaviorArgument source;
            source.m_value = behaviorObject.m_address;
            source.m_typeId = behaviorObject.m_typeId;

            AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> result;
            PythonMarshalTypeRequestBus::EventResult(result, source.m_typeId, &PythonMarshalTypeRequestBus::Events::BehaviorValueParameterToPython, source);
            if (result.has_value())
            {
                return result;
            }

            // return an opaque Behavior Objects to the caller if not a 'simple' type
            pybind11::object objectValue = PythonProxyObjectManagement::CreatePythonProxyObject(behaviorObject.m_typeId, behaviorObject.m_address);
            if (!objectValue.is_none())
            {
                return PythonMarshalTypeRequests::PythonValueResult( objectValue, {} );
            }
            return AZStd::nullopt;
        }

        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> ProcessPythonObject(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pythonObj, const AZ::TypeId& elementTypeId, AZ::BehaviorArgument& outValue)
        {
            // first try to convert using the element's type ID
            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> result;
            PythonMarshalTypeRequestBus::EventResult(result, elementTypeId, &PythonMarshalTypeRequestBus::Events::PythonToBehaviorValueParameter, traits, pythonObj, outValue);
            if (result)
            {
                return result;
            }
            else if (pybind11::isinstance<EditorPythonBindings::PythonProxyObject>(pythonObj))
            {
                AZ::BehaviorArgument behaviorArg;
                behaviorArg.m_traits = traits;
                behaviorArg.m_typeId = elementTypeId;

                if (Convert::PythonProxyObjectToBehaviorValueParameter(behaviorArg, pythonObj, outValue))
                {
                    return PythonMarshalTypeRequests::BehaviorValueResult( { true, nullptr } );
                }
            }
            return AZStd::nullopt;
        }

        bool LoadPythonToPairElement(PyObject* pyItem, PythonMarshalTypeRequests::BehaviorTraits traits, const AZ::SerializeContext::ClassElement* itemElement,
            AZ::SerializeContext::IDataContainer* pairContainer, size_t index, AZ::SerializeContext* serializeContext, void* newPair)
        {
            pybind11::object pyObj{ pybind11::reinterpret_borrow<pybind11::object>(pyItem) };
            AZ::BehaviorArgument behaviorItem;
            auto behaviorResult = ProcessPythonObject(traits, pyObj, itemElement->m_typeId, behaviorItem);
            if (behaviorResult && behaviorResult.value().first)
            {
                void* itemAddress = pairContainer->GetElementByIndex(newPair, itemElement, index);
                AZ_Assert(itemAddress, "Element reserved for associative container's pair, but unable to retrieve address of the item:%d", index);
                serializeContext->CloneObjectInplace(itemAddress, behaviorItem.m_value, itemElement->m_typeId);
            }
            else
            {
                AZ_Warning("python", false, "Could not convert to pair element type %s for the pair<>; failed to marshal Python input %s", itemElement->m_name, Convert::GetPythonTypeName(pyObj).c_str());
                return false;
            }
            return true;
        }

        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> ConvertPythonElement(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pythonElement, const AZ::TypeId& elementTypeId, AZ::BehaviorArgument& outValue)
        {
            // first try to convert using the element's type ID
            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> result;
            PythonMarshalTypeRequestBus::EventResult(result, elementTypeId, &PythonMarshalTypeRequestBus::Events::PythonToBehaviorValueParameter, traits, pythonElement, outValue);
            if (result)
            {
                return result;
            }
            else if (pybind11::isinstance<EditorPythonBindings::PythonProxyObject>(pythonElement))
            {
                AZ::BehaviorArgument behaviorArg;
                behaviorArg.m_traits = traits;
                behaviorArg.m_typeId = elementTypeId;

                if (Convert::PythonProxyObjectToBehaviorValueParameter(behaviorArg, pythonElement, outValue))
                {
                    return { { true, nullptr } };
                }
            }
            return AZStd::nullopt;
        }

        class TypeConverterDictionary final
            : public PythonMarshalComponent::TypeConverter
        {
            const AZ::SerializeContext::ClassData* m_classData = nullptr;
            const AZ::TypeId m_typeId = {};

        public:
            TypeConverterDictionary([[maybe_unused]] AZ::GenericClassInfo* genericClassInfo, const AZ::SerializeContext::ClassData* classData, const AZ::TypeId& typeId)
                : m_classData(classData)
                , m_typeId(typeId)
            {
            }

            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
            {
                if (!CanConvertPythonToBehaviorValue(traits, pyObj))
                {
                    AZ_Warning("python", false, "The dictionary container type for %s", m_classData->m_name);
                    return AZStd::nullopt;
                }

                const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_typeId);
                if (!behaviorClass)
                {
                    AZ_Warning("python", false, "Missing dictionary behavior class for %s", m_typeId.ToString<AZStd::string>().c_str());
                    return AZStd::nullopt;
                }

                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
                if (!serializeContext)
                {
                    return AZStd::nullopt;
                }

                // prepare the AZStd::unordered_map<> container
                AZ::BehaviorObject mapInstance = behaviorClass->Create();
                AZ::SerializeContext::IDataContainer* mapDataContainer = m_classData->m_container;
                const AZ::SerializeContext::ClassElement* pairElement = m_classData->m_container->GetElement(m_classData->m_container->GetDefaultElementNameCrc());
                const AZ::SerializeContext::ClassData* pairClass = serializeContext->FindClassData(pairElement->m_typeId);
                AZ_Assert(pairClass, "Associative container was registered but not the pair that's used for storage.");
                AZ::SerializeContext::IDataContainer* pairContainer = pairClass->m_container;
                AZ_Assert(pairContainer, "Associative container is missing the interface to the storage container.");

                // get the key/value element types
                const AZ::SerializeContext::ClassElement* keyElement = nullptr;
                const AZ::SerializeContext::ClassElement* valueElement = nullptr;
                auto keyValueTypeEnumCallback = [&keyElement, &valueElement](const AZ::Uuid&, const AZ::SerializeContext::ClassElement* genericClassElement)
                {
                    if (genericClassElement->m_flags & AZ::SerializeContext::ClassElement::Flags::FLG_POINTER)
                    {
                        AZ_Error("python", false, "Python marshalling does not handle naked pointers; not converting dict's pair");
                        return false;
                    }
                    else if (!keyElement)
                    {
                        keyElement = genericClassElement;
                    }
                    else if (!valueElement)
                    {
                        valueElement = genericClassElement;
                    }
                    else
                    {
                        AZ_Error("python", !valueElement, "The pair element in a container can't have more than 2 elements.");
                        return false;
                    }
                    return true;
                };
                pairContainer->EnumTypes(keyValueTypeEnumCallback);
                if (!keyElement || !valueElement)
                {
                    return AZStd::nullopt;
                }

                PyObject* key = nullptr;
                PyObject* value = nullptr;
                Py_ssize_t pos = 0;
                while (PyDict_Next(pyObj.ptr(), &pos, &key, &value))
                {
                    void* newPair = mapDataContainer->ReserveElement(mapInstance.m_address, pairElement);
                    AZ_Assert(newPair, "Could not allocate pair entry for map via ReserveElement()");
                    if (newPair)
                    {
                        const bool didKey = LoadPythonToPairElement(key, traits, keyElement, pairContainer, 0, serializeContext, newPair);
                        const bool didValue = LoadPythonToPairElement(value, traits, valueElement, pairContainer, 1, serializeContext, newPair);
                        if (didKey && didValue)
                        {
                            // store the pair in the map
                            mapDataContainer->StoreElement(mapInstance.m_address, newPair);
                        }
                        else
                        {
                            // release element, due to a failed pair conversion
                            mapDataContainer->FreeReservedElement(mapInstance.m_address, newPair, serializeContext);
                        }
                    }
                }

                AZ_Warning("python", static_cast<size_t>(PyDict_Size(pyObj.ptr())) == mapDataContainer->Size(mapInstance.m_address), "Python Dict size:%d does not match the size of the unordered_map:%d", pos, mapDataContainer->Size(mapInstance.m_address));
                outValue.m_value = mapInstance.m_address;
                outValue.m_typeId = mapInstance.m_typeId;
                outValue.m_traits = traits;

                auto deleteMapInstance = [behaviorClass, mapInstance]()
                {
                    behaviorClass->Destroy(mapInstance);
                };
                return PythonMarshalTypeRequests::BehaviorValueResult{ true, deleteMapInstance };
            }

            AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
            {
                // the class data must have a container interface
                auto* containerInterface = m_classData->m_container;
                if (!containerInterface)
                {
                    return AZStd::nullopt;
                }

                if (behaviorValue.ConvertTo(m_typeId))
                {
                    auto cleanUpList = AZStd::make_shared<AZStd::vector<PythonMarshalTypeRequests::DeallocateFunction>>();
                    pybind11::dict pythonDictionary;

                    // visit each unordered_map<K,V> entry
                    auto elementCallback = [pythonDictionary, cleanUpList](void* instancePointer, [[maybe_unused]] const auto& elementClassId, const auto* elementGenericClassData, [[maybe_unused]] const auto* genericClassElement)
                    {
                        pybind11::object pythonKey { pybind11::none() };
                        pybind11::object pythonItem { pybind11::none() };

                        // visit the AZStd::pair<K,V> elements
                        auto pairCallback = [cleanUpList, &pythonKey, &pythonItem](void* instancePair, const auto& elementClassId, [[maybe_unused]] const auto* elementGenericClassData, [[maybe_unused]] const auto* genericClassElement)
                        {
                            AZ::BehaviorObject behaviorObjectValue(instancePair, elementClassId);
                            auto result = ProcessBehaviorObject(behaviorObjectValue);
                            if (result)
                            {
                                PythonMarshalTypeRequests::DeallocateFunction deallocateFunction = result.value().second;
                                if (result.value().second)
                                {
                                    cleanUpList->emplace_back(AZStd::move(result.value().second));
                                }

                                pybind11::object pythonResult = result.value().first;
                                if (pythonKey.is_none())
                                {
                                    pythonKey = pythonResult;
                                }
                                else if (pythonItem.is_none())
                                {
                                    pythonItem = pythonResult;
                                }
                            }
                            return true;
                        };
                        elementGenericClassData->m_container->EnumElements(instancePointer, pairCallback);

                        // have a valid key and value?
                        if (!pythonKey.is_none() && !pythonItem.is_none())
                        {
                            // assign the key's value in the dictionary?
                            if (PyDict_SetItem(pythonDictionary.ptr(), pythonKey.ptr(), pythonItem.ptr()) < 0)
                            {
                                AZStd::string pythonKeyString = pybind11::cast<AZStd::string>(pythonKey);
                                AZStd::string pythonItemString = pybind11::cast<AZStd::string>(pythonItem);
                                AZ_Warning("python", false, "Could not add key:%s with item value:%s", pythonKeyString.c_str(), pythonKeyString.c_str());
                            }
                        }
                        return true;
                    };
                    containerInterface->EnumElements(behaviorValue.m_value, elementCallback);

                    PythonMarshalTypeRequests::PythonValueResult result;
                    result.first = pythonDictionary;

                    if (!cleanUpList->empty())
                    {
                        AZStd::weak_ptr<AZStd::vector<PythonMarshalTypeRequests::DeallocateFunction>> cleanUp(cleanUpList);
                        result.second = [cleanUp]()
                        {
                            auto cleanupList = cleanUp.lock();
                            if (cleanupList)
                            {
                                AZStd::for_each(cleanupList->begin(), cleanupList->end(), [](auto& deleteMe) { deleteMe(); });
                            }
                        };
                    }

                    return result;
                }
                return AZStd::nullopt;
            }

            bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
            {
                // the underlying types must have exactly two elements
                AZStd::vector<AZ::Uuid> typeList = AZ::Utils::GetContainedTypes(m_typeId);
                if (typeList.size() != 2)
                {
                    return false;
                }

                return (PyDict_Check(pyObj.ptr()) != false);
            }
        };

        class TypeConverterVector
            : public PythonMarshalComponent::TypeConverter
        {
        public:
            AZ::GenericClassInfo* m_genericClassInfo = nullptr;
            const AZ::SerializeContext::ClassData* m_classData = nullptr;
            const AZ::TypeId m_typeId = {};

            TypeConverterVector(AZ::GenericClassInfo* genericClassInfo, const AZ::SerializeContext::ClassData* classData, const AZ::TypeId& typeId)
                : m_genericClassInfo(genericClassInfo)
                , m_classData(classData)
                , m_typeId(typeId)
            {
            }

            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> HandlePythonElement(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pythonElement, const AZ::TypeId& elementTypeId, AZ::BehaviorArgument& outValue)
            {
                // first try to convert using the element's type ID
                AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> result;
                PythonMarshalTypeRequestBus::EventResult(result, elementTypeId, &PythonMarshalTypeRequestBus::Events::PythonToBehaviorValueParameter, traits, pythonElement, outValue);
                if (result)
                {
                    return result;
                }
                else if (pybind11::isinstance<EditorPythonBindings::PythonProxyObject>(pythonElement))
                {
                    AZ::BehaviorArgument behaviorArg;
                    behaviorArg.m_traits = traits;
                    behaviorArg.m_typeId = elementTypeId;

                    if (Convert::PythonProxyObjectToBehaviorValueParameter(behaviorArg, pythonElement, outValue))
                    {
                        return { { true, nullptr } };
                    }
                }
                return AZStd::nullopt;
            }

            // handle a vector of Behavior Class values
            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorObjectList(const AZ::TypeId& elementType, const AZ::BehaviorClass* behaviorClass, PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue)
            {
                auto iteratorToPushBackMethod = behaviorClass->m_methods.find("push_back");
                if (iteratorToPushBackMethod == behaviorClass->m_methods.end())
                {
                    AZ_Warning("python", false, "BehaviorClass container missing push_back method");
                    return AZStd::nullopt;
                }

                // prepare the AZStd::vector container
                AZ::BehaviorObject instance = behaviorClass->Create();
                AZ::BehaviorMethod* pushBackMethod = iteratorToPushBackMethod->second;

                [[maybe_unused]] size_t vectorCount = 0;
                pybind11::list pyList(pyObj);
                for (auto pyItem = pyList.begin(); pyItem != pyList.end(); ++pyItem)
                {
                    auto pyObjItem = pybind11::cast<pybind11::object>(*pyItem);
                    AZ::BehaviorArgument elementValue;
                    auto result = HandlePythonElement(traits, pyObjItem, elementType, elementValue);
                    if (result && result.value().first)
                    {
                        AZ::BehaviorArgument parameters[2];
                        parameters[0].Set(&instance);
                        parameters[1].Set(elementValue);
                        pushBackMethod->Call(parameters, 2);
                        ++vectorCount;
                    }
                    else
                    {
                        AZ_Warning("python", false, "Could not convert to behavior element type %s for the vector<>; failed to marshal Python input %s",
                            elementType.ToString<AZStd::string>().c_str(), Convert::GetPythonTypeName(pyObjItem).c_str());
                        return AZStd::nullopt;
                    }
                }

                AZ_Warning("python", vectorCount == pyList.size(), "Python list size:%d does not match the size of the vector:%d", pyList.size(), vectorCount);

                outValue.m_value = instance.m_address;
                outValue.m_typeId = instance.m_typeId;
                outValue.m_traits = traits;

                auto deleteVector = [behaviorClass, instance]()
                {
                    behaviorClass->Destroy(instance);
                };
                return { { true, deleteVector } };
            }

            // handle a vector of a data type not registered with the Behavior Context
            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorSerializedList(const AZ::TypeId& elementType, PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue)
            {
                // fetch the container parts
                const AZ::SerializeContext::ClassData* classData = m_genericClassInfo->GetClassData();
                const AZ::SerializeContext::ClassElement* classElement = classData->m_container->GetElement(classData->m_container->GetDefaultElementNameCrc());

                // prepare the AZStd::vector container
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
                AZStd::any* newVector = new AZStd::any(serializeContext->CreateAny(m_typeId));
                void* instance = AZStd::any_cast<void>(newVector);

                [[maybe_unused]] size_t vectorCount = 0;
                pybind11::list pyList(pyObj);
                for (auto pyItem = pyList.begin(); pyItem != pyList.end(); ++pyItem)
                {
                    auto pyObjItem = pybind11::cast<pybind11::object>(*pyItem);
                    AZ::BehaviorArgument elementValue;
                    auto elementResult = HandlePythonElement(traits, pyObjItem, elementType, elementValue);
                    if (elementResult && elementResult.value().first)
                    {
                        void* destination = classData->m_container->ReserveElement(instance, classElement);
                        AZ_Error("python", destination, "Could not allocate via ReserveElement()");
                        if (destination)
                        {
                            serializeContext->CloneObjectInplace(destination, elementValue.m_value, elementType);
                            ++vectorCount;
                        }
                    }
                    else
                    {
                        AZ_Warning("python", false, "Could not convert to serialized element type %s for the vector<>; failed to marshal Python input %s",
                            elementType.ToString<AZStd::string>().c_str(), Convert::GetPythonTypeName(pyObjItem).c_str());
                        return AZStd::nullopt;
                    }
                }

                AZ_Warning("python", vectorCount == pyList.size(), "Python list size:%d does not match the size of the vector:%d", pyList.size(), vectorCount);

                outValue.m_name = classData->m_name;
                outValue.m_value = instance;
                outValue.m_typeId = m_typeId;
                outValue.m_traits = traits;

                auto deleteVector = [newVector]()
                {
                    delete newVector;
                };
                return { { true, deleteVector } };
            }

            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
            {
                AZStd::vector<AZ::Uuid> typeList = AZ::Utils::GetContainedTypes(m_typeId);
                if (typeList.empty())
                {
                    AZ_Warning("python", false, "The list container type for %s had no types; expected one type", m_classData->m_name);
                    return AZStd::nullopt;
                }
                else if (PyList_Check(pyObj.ptr()) == false)
                {
                    AZ_Warning("python", false, "Expected a Python List as input");
                    return AZStd::nullopt;
                }

                const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_typeId);
                if (behaviorClass)
                {
                    return PythonToBehaviorObjectList(typeList[0], behaviorClass, traits, pyObj, outValue);
                }
                return PythonToBehaviorSerializedList(typeList[0], traits, pyObj, outValue);
            }

            using HandleResult = AZStd::optional<PythonMarshalTypeRequests::DeallocateFunction>;

            HandleResult HandleElement(AZ::BehaviorObject& behaviorObject, pybind11::list pythonList)
            {
                AZ::BehaviorArgument source;
                source.m_value = behaviorObject.m_address;
                source.m_typeId = behaviorObject.m_typeId;

                AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> result;
                PythonMarshalTypeRequestBus::EventResult(result, source.m_typeId, &PythonMarshalTypeRequestBus::Events::BehaviorValueParameterToPython, source);
                if (result.has_value())
                {
                    pythonList.append(result.value().first);
                    return AZStd::move(result.value().second);
                }

                // return back a 'list of opaque Behavior Objects' back to the caller if not a 'simple' type
                pybind11::object value = PythonProxyObjectManagement::CreatePythonProxyObject(behaviorObject.m_typeId, behaviorObject.m_address);
                if (!value.is_none())
                {
                    pythonList.append(value);
                }
                return AZStd::nullopt;
            }

            AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
            {
                auto* container = m_classData->m_container;
                if (behaviorValue.ConvertTo(m_typeId) && container)
                {
                    auto deleterList = AZStd::make_shared<AZStd::vector<PythonMarshalTypeRequests::DeallocateFunction>>();
                    pybind11::list pythonList;

                    auto elementCallback = [this, pythonList, deleterList](void* instancePointer, const auto& elementClassId, [[maybe_unused]] const auto* elementGenericClassData, [[maybe_unused]] const auto* genericClassElement)
                    {
                        AZ::BehaviorObject behaviorObject(instancePointer, elementClassId);
                        auto result = this->HandleElement(behaviorObject, pythonList);
                        if (result)
                        {
                            if (result.value())
                            {
                                deleterList->emplace_back(AZStd::move(result.value()));
                            }
                        }
                        return true;
                    };
                    container->EnumElements(behaviorValue.m_value, elementCallback);

                    PythonMarshalTypeRequests::PythonValueResult result;
                    result.first = pythonList;

                    if (!deleterList->empty())
                    {
                        AZStd::weak_ptr<AZStd::vector<PythonMarshalTypeRequests::DeallocateFunction>> cleanUp(deleterList);
                        result.second = [cleanUp]()
                        {
                            auto cleanupList = cleanUp.lock();
                            if (cleanupList)
                            {
                                AZStd::for_each(cleanupList->begin(), cleanupList->end(), [](auto& deleteMe) { deleteMe(); });
                            }
                        };
                    }

                    return result;
                }
                return AZStd::nullopt;
            }

            bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
            {
                AZStd::vector<AZ::Uuid> typeList = AZ::Utils::GetContainedTypes(m_typeId);
                if (typeList.empty())
                {
                    return false;
                }
                return (PyList_Check(pyObj.ptr()) != false);
            }
        };

        class TypeConverterSet
            : public PythonMarshalComponent::TypeConverter
        {
        public:
            AZ::GenericClassInfo* m_genericClassInfo = nullptr;
            const AZ::SerializeContext::ClassData* m_classData = nullptr;
            const AZ::TypeId m_typeId = {};

            TypeConverterSet(AZ::GenericClassInfo* genericClassInfo, const AZ::SerializeContext::ClassData* classData, const AZ::TypeId& typeId)
                : m_genericClassInfo(genericClassInfo)
                , m_classData(classData)
                , m_typeId(typeId)
            {
            }

            // handle a vector of Behavior Class values
            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorObjectSet(const AZ::TypeId& elementType, const AZ::BehaviorClass* behaviorClass, PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue)
            {
                auto iteratorToInsertMethod = behaviorClass->m_methods.find("Insert");
                if (iteratorToInsertMethod == behaviorClass->m_methods.end())
                {
                    AZ_Error("python", false, "The AZStd::unordered_set BehaviorClass reflection is missing the Insert method!");
                    return AZStd::nullopt;
                }

                // prepare the AZStd::unordered_set container
                AZ::BehaviorObject instance = behaviorClass->Create();
                AZ::BehaviorMethod* insertMethod = iteratorToInsertMethod->second;

                [[maybe_unused]] size_t itemCount = 0;
                pybind11::set pySet(pyObj);
                for (auto pyItem = pySet.begin(); pyItem != pySet.end(); ++pyItem)
                {
                    auto pyObjItem = pybind11::cast<pybind11::object>(*pyItem);
                    AZ::BehaviorArgument elementValue;
                    auto result = ConvertPythonElement(traits, pyObjItem, elementType, elementValue);
                    if (result && result.value().first)
                    {
                        AZ::BehaviorArgument parameters[2];

                        // set the 'this' pointer
                        parameters[0].m_value = instance.m_address;
                        parameters[0].m_typeId = instance.m_typeId;

                        // set the value element
                        parameters[1].Set(elementValue);

                        insertMethod->Call(parameters, 2);
                        ++itemCount;
                    }
                    else
                    {
                        AZ_Warning("python", false, "Convert to behavior element type %s for the unordered_set<> failed to marshal Python input %s",
                            elementType.ToString<AZStd::string>().c_str(), Convert::GetPythonTypeName(pyObjItem).c_str());
                        return AZStd::nullopt;
                    }
                }

                AZ_Warning("python", itemCount == pySet.size(), "Python Set size:%d does not match the size of the unordered_set:%d", pySet.size(), itemCount);

                outValue.m_value = instance.m_address;
                outValue.m_typeId = instance.m_typeId;
                outValue.m_traits = traits;

                auto deleteVector = [behaviorClass, instance]()
                {
                    behaviorClass->Destroy(instance);
                };
                return { { true, deleteVector } };
            }

            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorSerializedSet(const AZ::TypeId& elementType, PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue)
            {
                // fetch the container parts
                const AZ::SerializeContext::ClassData* classData = m_genericClassInfo->GetClassData();
                const AZ::SerializeContext::ClassElement* classElement = classData->m_container->GetElement(classData->m_container->GetDefaultElementNameCrc());

                // prepare the AZStd::unordered_set container
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
                AZStd::any* newVector = new AZStd::any(serializeContext->CreateAny(m_typeId));
                void* instance = AZStd::any_cast<void>(newVector);

                [[maybe_unused]] size_t itemCount = 0;
                pybind11::set pySet(pyObj);
                for (auto pyItem = pySet.begin(); pyItem != pySet.end(); ++pyItem)
                {
                    auto pyObjItem = pybind11::cast<pybind11::object>(*pyItem);
                    AZ::BehaviorArgument elementValue;
                    auto elementResult = ConvertPythonElement(traits, pyObjItem, elementType, elementValue);
                    if (elementResult && elementResult.value().first)
                    {
                        void* destination = classData->m_container->ReserveElement(instance, classElement);
                        AZ_Error("python", destination, "Could not allocate via ReserveElement()");
                        if (destination)
                        {
                            serializeContext->CloneObjectInplace(destination, elementValue.m_value, elementType);
                            ++itemCount;
                        }
                    }
                    else
                    {
                        AZ_Warning("python", false, "Convert to serialized element type %s for the unordered_set<> failed to marshal Python input %s",
                            elementType.ToString<AZStd::string>().c_str(), Convert::GetPythonTypeName(pyObjItem).c_str());
                        return AZStd::nullopt;
                    }
                }

                AZ_Warning("python", itemCount == pySet.size(), "Python list size:%d does not match the size of the unordered_set:%d", pySet.size(), itemCount);

                outValue.m_name = classData->m_name;
                outValue.m_value = instance;
                outValue.m_typeId = m_typeId;
                outValue.m_traits = traits;

                auto deleteVector = [newVector]()
                {
                    delete newVector;
                };
                return { { true, deleteVector } };
            }

            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
            {
                AZStd::vector<AZ::Uuid> typeList = AZ::Utils::GetContainedTypes(m_typeId);
                if (typeList.empty())
                {
                    AZ_Warning("python", false, "The unordered_set container type for %s had no types; expected one type", m_classData->m_name);
                    return AZStd::nullopt;
                }
                else if (PySet_Check(pyObj.ptr()) == false)
                {
                    AZ_Warning("python", false, "Expected a Python Set as input");
                    return AZStd::nullopt;
                }

                const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_typeId);
                if (behaviorClass)
                {
                    return PythonToBehaviorObjectSet(typeList[0], behaviorClass, traits, pyObj, outValue);
                }
                return PythonToBehaviorSerializedSet(typeList[0], traits, pyObj, outValue);
            }

            AZStd::optional<PythonMarshalTypeRequests::DeallocateFunction> HandleSetElement(AZ::BehaviorObject& behaviorObject, pybind11::set pythonSet)
            {
                AZ::BehaviorArgument source;
                source.m_value = behaviorObject.m_address;
                source.m_typeId = behaviorObject.m_typeId;

                AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> result;
                PythonMarshalTypeRequestBus::EventResult(result, source.m_typeId, &PythonMarshalTypeRequestBus::Events::BehaviorValueParameterToPython, source);
                if (result.has_value())
                {
                    pythonSet.add(result.value().first);
                    return AZStd::move(result.value().second);
                }

                // return back a 'list of opaque Behavior Objects' back to the caller if not a 'simple' type
                pybind11::object value = PythonProxyObjectManagement::CreatePythonProxyObject(behaviorObject.m_typeId, behaviorObject.m_address);
                if (!value.is_none())
                {
                    pythonSet.add(value);
                }
                return AZStd::nullopt;
            }

            AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
            {
                auto* container = m_classData->m_container;
                AZ_Error("python", container, "Set container class data is missing");
                if (container == nullptr)
                {
                    return AZStd::nullopt;
                }

                if (behaviorValue.ConvertTo(m_typeId))
                {
                    auto deleterList = AZStd::make_shared<AZStd::vector<PythonMarshalTypeRequests::DeallocateFunction>>();
                    pybind11::set pythonSet;

                    auto elementCallback = [this, pythonSet, deleterList](void* instancePointer, const auto& elementClassId, const auto*, const auto*)
                    {
                        AZ::BehaviorObject behaviorObject(instancePointer, elementClassId);
                        auto result = this->HandleSetElement(behaviorObject, pythonSet);
                        if (result)
                        {
                            if (result.value())
                            {
                                deleterList->emplace_back(AZStd::move(result.value()));
                            }
                        }
                        return true;
                    };
                    container->EnumElements(behaviorValue.m_value, elementCallback);

                    PythonMarshalTypeRequests::PythonValueResult result;
                    result.first = pythonSet;

                    if (!deleterList->empty())
                    {
                        AZStd::weak_ptr<AZStd::vector<PythonMarshalTypeRequests::DeallocateFunction>> cleanUp(deleterList);
                        result.second = [cleanUp]()
                        {
                            auto cleanupList = cleanUp.lock();
                            if (cleanupList)
                            {
                                AZStd::for_each(cleanupList->begin(), cleanupList->end(), [](auto& deleteMe) { deleteMe(); });
                            }
                        };
                    }

                    return result;
                }
                return AZStd::nullopt;
            }

            bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
            {
                AZStd::vector<AZ::Uuid> typeList = AZ::Utils::GetContainedTypes(m_typeId);
                if (typeList.empty())
                {
                    return false;
                }
                return (PySet_Check(pyObj.ptr()) != false);
            }
        };

        class TypeConverterPair final
            : public PythonMarshalComponent::TypeConverter
        {
            const AZ::SerializeContext::ClassData* m_classData = nullptr;
            const AZ::TypeId m_typeId = {};

            bool IsValidList(pybind11::object pyObj) const
            {
                return PyList_Check(pyObj.ptr()) != false && PyList_Size(pyObj.ptr()) == 2;
            }

            bool IsValidTuple(pybind11::object pyObj) const
            {
                return PyTuple_Check(pyObj.ptr()) != false && PyTuple_Size(pyObj.ptr()) == 2;
            }

            bool IsCompatibleProxy(pybind11::object pyObj) const
            {
                if (pybind11::isinstance<EditorPythonBindings::PythonProxyObject>(pyObj))
                {
                    auto behaviorObject = pybind11::cast<EditorPythonBindings::PythonProxyObject*>(pyObj)->GetBehaviorObject();
                    AZ::Uuid typeId = behaviorObject.value()->m_typeId;
                    return AZ::Utils::IsPairContainerType(typeId);
                }

                return false;
            }

        public:
            TypeConverterPair([[maybe_unused]] AZ::GenericClassInfo* genericClassInfo, const AZ::SerializeContext::ClassData* classData, const AZ::TypeId& typeId)
                : m_classData(classData)
                , m_typeId(typeId)
            {
            }

            AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override
            {
                if (!CanConvertPythonToBehaviorValue(traits, pyObj))
                {
                    AZ_Warning("python", false, "Cannot convert pair container for %s", m_classData->m_name);
                    return AZStd::nullopt;
                }

                const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_typeId);
                if (!behaviorClass)
                {
                    AZ_Warning("python", false, "Missing pair behavior class for %s", m_typeId.ToString<AZStd::string>().c_str());
                    return AZStd::nullopt;
                }

                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
                if (!serializeContext)
                {
                    return AZStd::nullopt;
                }

                // prepare the AZStd::pair<> container
                AZ::BehaviorObject pairInstance = behaviorClass->Create();
                AZ::SerializeContext::IDataContainer* pairDataContainer = m_classData->m_container;

                // get the element types
                const AZ::SerializeContext::ClassElement* element0 = nullptr;
                const AZ::SerializeContext::ClassElement* element1 = nullptr;

                auto elementTypeEnumCallback = [&element0, &element1](const AZ::Uuid&, const AZ::SerializeContext::ClassElement* genericClassElement)
                {
                    if (genericClassElement->m_flags & AZ::SerializeContext::ClassElement::Flags::FLG_POINTER)
                    {
                        AZ_Error("python", false, "Python marshalling does not handle naked pointers; not converting the pair");
                        return false;
                    }
                    else if (!element0)
                    {
                        element0 = genericClassElement;
                    }
                    else if (!element1)
                    {
                        element1 = genericClassElement;
                    }
                    else
                    {
                        AZ_Error("python", false, "The pair container can't have more than 2 elements.");
                        return false;
                    }

                    return true;
                };

                pairDataContainer->EnumTypes(elementTypeEnumCallback);
                if (!element0 || !element1)
                {
                    AZ_Error("python", false, "Could not retrieve pair elements.");
                    return AZStd::nullopt;
                }

                // load python items into pair elements
                PyObject* item0 = nullptr;
                PyObject* item1 = nullptr;
                if (IsValidList(pyObj))
                {
                    pybind11::list pyList(pyObj);

                    item0 = pyList[0].ptr();
                    item1 = pyList[1].ptr();
                }
                else if (IsValidTuple(pyObj))
                {
                    pybind11::tuple pyTuple(pyObj);

                    item0 = pyTuple[0].ptr();
                    item1 = pyTuple[1].ptr();
                }
                else if (IsCompatibleProxy(pyObj))
                {
                    // OnDemandReflection<AZStd::pair<T1, T2>> exposes "first" and "second" in the proxy object
                    EditorPythonBindings::PythonProxyObject* proxy = pybind11::cast<EditorPythonBindings::PythonProxyObject*>(pyObj);
                    item0 = proxy->GetPropertyValue("first").ptr();
                    item1 = proxy->GetPropertyValue("second").ptr();
                }

                void* reserved0 = pairDataContainer->ReserveElement(pairInstance.m_address, element0);
                AZ_Assert(reserved0, "Could not allocate pair's first item via ReserveElement()");
                if (item0 && item1 && !LoadPythonToPairElement(item0, traits, element0, pairDataContainer, 0, serializeContext, pairInstance.m_address))
                {
                    pairDataContainer->FreeReservedElement(pairInstance.m_address, reserved0, serializeContext);
                    return AZStd::nullopt;
                }

                void* reserved1 = pairDataContainer->ReserveElement(pairInstance.m_address, element1);
                AZ_Assert(reserved1, "Could not allocate pair's second item via ReserveElement()");
                if (item1 && !LoadPythonToPairElement(item1, traits, element1, pairDataContainer, 1, serializeContext, pairInstance.m_address))
                {
                    pairDataContainer->FreeReservedElement(pairInstance.m_address, reserved0, serializeContext);
                    pairDataContainer->FreeReservedElement(pairInstance.m_address, reserved1, serializeContext);
                    return AZStd::nullopt;
                }

                outValue.m_value = pairInstance.m_address;
                outValue.m_typeId = pairInstance.m_typeId;
                outValue.m_traits = traits;

                auto pairInstanceDeleter = [behaviorClass, pairInstance]()
                {
                    behaviorClass->Destroy(pairInstance);
                };

                return PythonMarshalTypeRequests::BehaviorValueResult{ true, pairInstanceDeleter };
            }

            AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue) override
            {
                // the class data must have a container interface
                AZ::SerializeContext::IDataContainer* containerInterface = m_classData->m_container;

                if (!containerInterface)
                {
                    AZ_Warning("python", false, "Container interface is missing from class %s.", m_classData->m_name);
                    return AZStd::nullopt;
                }

                if (!behaviorValue.ConvertTo(m_typeId))
                {
                    AZ_Warning("python", false, "Cannot convert behavior value %s.", behaviorValue.m_name);
                    return AZStd::nullopt;
                }

                auto cleanUpList = AZStd::make_shared<AZStd::vector<PythonMarshalTypeRequests::DeallocateFunction>>();

                // return pair as list, if conversion failed for an item it will remain as 'none'
                pybind11::list pythonList;
                pybind11::object pythonItem0{ pybind11::none() };
                pybind11::object pythonItem1{ pybind11::none() };
                size_t itemCount = 0;

                auto pairElementCallback = [cleanUpList, &pythonItem0, &pythonItem1, &itemCount](void* instancePair, const AZ::Uuid& elementClassId, [[maybe_unused]] const AZ::SerializeContext::ClassData* elementGenericClassData, [[maybe_unused]] const AZ::SerializeContext::ClassElement* genericClassElement)
                {
                    AZ::BehaviorObject behaviorObjectValue(instancePair, elementClassId);
                    auto result = ProcessBehaviorObject(behaviorObjectValue);

                    if (result.has_value())
                    {
                        PythonMarshalTypeRequests::DeallocateFunction deallocateFunction = result.value().second;
                        if (result.value().second)
                        {
                            cleanUpList->emplace_back(AZStd::move(result.value().second));
                        }

                        pybind11::object pythonResult = result.value().first;
                        if (itemCount == 0)
                        {
                            pythonItem0 = pythonResult;
                        }
                        else
                        {
                            pythonItem1 = pythonResult;
                        }

                        itemCount++;
                    }
                    else
                    {
                        AZ_Warning("python", false, "BehaviorObject was not processed, python item will remain 'none'.");
                    }

                    return true;
                };

                containerInterface->EnumElements(behaviorValue.m_value, pairElementCallback);
                pythonList.append(pythonItem0);
                pythonList.append(pythonItem1);

                PythonMarshalTypeRequests::PythonValueResult result;
                result.first = pythonList;

                if (!cleanUpList->empty())
                {
                    AZStd::weak_ptr<AZStd::vector<PythonMarshalTypeRequests::DeallocateFunction>> cleanUp(cleanUpList);
                    result.second = [cleanUp]()
                    {
                        auto cleanupList = cleanUp.lock();
                        if (cleanupList)
                        {
                            AZStd::for_each(cleanupList->begin(), cleanupList->end(), [](auto& deleteMe) { deleteMe(); });
                        }
                    };
                }

                return result;
            }

            bool CanConvertPythonToBehaviorValue([[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override
            {
                AZStd::vector<AZ::Uuid> typeList = AZ::Utils::GetContainedTypes(m_typeId);
                bool isList = IsValidList(pyObj);
                bool isTuple = IsValidTuple(pyObj);
                bool isCompatibleProxy = IsCompatibleProxy(pyObj);

                if (typeList.empty() || typeList.size() != 2)
                {
                    return false;
                }

                return isList || isTuple || isCompatibleProxy;
            }
        };

        using TypeConverterRegistrant = AZStd::function<void(const AZ::TypeId& typeId, PythonMarshalComponent::TypeConverterPointer typeConverterPointer)>;

        void RegisterContainerTypes(TypeConverterRegistrant registrant)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            if (!serializeContext)
            {
                return;
            }

            // handle the generic container types and create type converters for each found
            auto handleTypeInfo = [registrant, serializeContext](const AZ::SerializeContext::ClassData* classData, const AZ::TypeId& typeId)
            {
                if (typeId == AZ::AzTypeInfo<AZStd::vector<AZ::u8>>::Uuid())
                {
                    // AZStd::vector<AZ::u8> is registered in the Serialization Context as a ByteStream, so it fails on IsVectorContainerType()
                    registrant(typeId, AZStd::make_unique<TypeConverterByteStream>());
                }
                else if (AZ::Utils::IsVectorContainerType(typeId))
                {
                    registrant(typeId, AZStd::make_unique<TypeConverterVector>(serializeContext->FindGenericClassInfo(typeId), classData, typeId));
                }
                else if (AZ::Utils::IsMapContainerType(typeId))
                {
                    registrant(typeId, AZStd::make_unique<TypeConverterDictionary>(serializeContext->FindGenericClassInfo(typeId), classData, typeId));
                }
                else if (AZ::Utils::IsPairContainerType(typeId))
                {
                    registrant(typeId, AZStd::make_unique<TypeConverterPair>(serializeContext->FindGenericClassInfo(typeId), classData, typeId));
                }
                else if (AZ::Utils::IsTupleContainerType(typeId))
                {
                    registrant(
                        typeId, AZStd::make_unique<TypeConverterTuple>(serializeContext->FindGenericClassInfo(typeId), classData, typeId));
                }
                else if (AZ::Utils::IsSetContainerType(typeId))
                {
                    registrant(typeId, AZStd::make_unique<TypeConverterSet>(serializeContext->FindGenericClassInfo(typeId), classData, typeId));
                }
                return true;
            };

            const bool includeGenerics = true;
            serializeContext->EnumerateAll(handleTypeInfo, includeGenerics);
        }
    }

    AZStd::optional<PythonMarshalComponent::BehaviorValueResult> PythonMarshalComponent::PythonToBehaviorValueParameter(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue)
    {
        const auto* typeId = PythonMarshalTypeRequestBus::GetCurrentBusId();
        AZ_Error("python", typeId, "Requires a valid non-null AZ::TypeId pointer");
        if (!typeId)
        {
            return AZStd::nullopt;
        }
        auto converterEntry = m_typeConverterMap.find(*typeId);
        if (m_typeConverterMap.end() == converterEntry)
        {
            return AZStd::nullopt;
        }
        return converterEntry->second->PythonToBehaviorValueParameter(traits, pyObj, outValue);
    }

    AZStd::optional<PythonMarshalComponent::PythonValueResult> PythonMarshalComponent::BehaviorValueParameterToPython(AZ::BehaviorArgument& behaviorValue)
    {
        const auto* typeId = PythonMarshalTypeRequestBus::GetCurrentBusId();
        AZ_Error("python", typeId, "Requires a valid non-null AZ::TypeId pointer");
        if (!typeId)
        {
            return AZStd::nullopt;
        }
        auto converterEntry = m_typeConverterMap.find(*typeId);
        if (m_typeConverterMap.end() == converterEntry)
        {
            return AZStd::nullopt;
        }
        return converterEntry->second->BehaviorValueParameterToPython(behaviorValue);
    }

    //////////////////////////////////////////////////////////////////////////
    // PythonMarshalComponent
    void PythonMarshalComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(PythonMarshalingService);
    }

    void PythonMarshalComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(PythonMarshalingService);
    }

    void PythonMarshalComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(PythonEmbeddedService);
    }

    bool PythonMarshalComponent::CanConvertPythonToBehaviorValue(BehaviorTraits traits, pybind11::object pyObj) const
    {
        const auto* typeId = PythonMarshalTypeRequestBus::GetCurrentBusId();
        AZ_Error("python", typeId, "Requires a valid non-null AZ::TypeId pointer");
        if (!typeId)
        {
            return false;
        }
        auto converterEntry = m_typeConverterMap.find(*typeId);
        if (converterEntry == m_typeConverterMap.end())
        {
            return false;
        }
        return converterEntry->second->CanConvertPythonToBehaviorValue(traits, pyObj);
    }

    void PythonMarshalComponent::RegisterTypeConverter(const AZ::TypeId& typeId, TypeConverterPointer typeConverterPointer)
    {
        PythonMarshalTypeRequestBus::MultiHandler::BusConnect(typeId);
        m_typeConverterMap[typeId] = AZStd::move(typeConverterPointer);
    }

    void PythonMarshalComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto&& serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PythonMarshalComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>{AZ_CRC_CE("AssetBuilder")})
                ;
        }
    }

    void PythonMarshalComponent::Activate()
    {
        RegisterTypeConverter(AZ::AzTypeInfo<bool>::Uuid(), AZStd::make_unique<TypeConverterBool>());
        RegisterTypeConverter(AZ::AzTypeInfo<char>::Uuid(), AZStd::make_unique<TypeConverterChar>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZ::s8>::Uuid(), AZStd::make_unique<TypeConverterInteger<AZ::s8>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZ::u8>::Uuid(), AZStd::make_unique<TypeConverterInteger<AZ::u8>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZ::s16>::Uuid(), AZStd::make_unique<TypeConverterInteger<AZ::s16>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZ::u16>::Uuid(), AZStd::make_unique<TypeConverterInteger<AZ::u16>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZ::s32>::Uuid(), AZStd::make_unique<TypeConverterInteger<AZ::s32>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZ::u32>::Uuid(), AZStd::make_unique<TypeConverterInteger<AZ::u32>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZ::s64>::Uuid(), AZStd::make_unique<TypeConverterInteger<AZ::s64>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZ::u64>::Uuid(), AZStd::make_unique<TypeConverterInteger<AZ::u64>>());
        RegisterTypeConverter(AZ::AzTypeInfo<long>::Uuid(), AZStd::make_unique<TypeConverterInteger<long>>());
        RegisterTypeConverter(AZ::AzTypeInfo<unsigned long>::Uuid(), AZStd::make_unique<TypeConverterInteger<unsigned long>>());
        RegisterTypeConverter(AZ::AzTypeInfo<float>::Uuid(), AZStd::make_unique<TypeConverterReal<float, float, float>>());
        RegisterTypeConverter(AZ::AzTypeInfo<double>::Uuid(), AZStd::make_unique<TypeConverterReal<double, double, double>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZStd::string>::Uuid(), AZStd::make_unique<TypeConverterString<AZStd::string>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZ::IO::FixedMaxPathString>::Uuid(), AZStd::make_unique<TypeConverterString<AZ::IO::FixedMaxPathString>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZStd::string_view>::Uuid(), AZStd::make_unique<TypeConverterString<AZStd::string_view>>());
        RegisterTypeConverter(AZ::AzTypeInfo<AZStd::any>::Uuid(), AZStd::make_unique<TypeConverterAny>());

        Container::RegisterContainerTypes([this](const AZ::TypeId& typeId, auto containerConverter)
        {
            this->RegisterTypeConverter(typeId, AZStd::move(containerConverter));
        });
    }

    void PythonMarshalComponent::Deactivate()
    {
        PythonMarshalTypeRequestBus::MultiHandler::BusDisconnect();
        m_typeConverterMap.clear();
    }
}
