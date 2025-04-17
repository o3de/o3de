/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Containers/GraphObjectProxy.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

namespace AZ
{
    namespace Python
    {
        static const char* const None = "None";


        PythonBehaviorInfo::PythonBehaviorInfo(const AZ::BehaviorClass* behaviorClass)
            : m_behaviorClass(behaviorClass)
        {
            AZ_Assert(m_behaviorClass, "PythonBehaviorInfo requires a valid behaviorClass pointer");
            for (const auto& method : behaviorClass->m_methods)
            {
                PrepareMethod(method.first, *method.second);
            }
            for (const auto& property : behaviorClass->m_properties)
            {
                PrepareProperty(property.first, *property.second);
            }
        }

        void PythonBehaviorInfo::Reflect(AZ::ReflectContext* context)
        {
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<PythonBehaviorInfo>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                    ->Attribute(AZ::Script::Attributes::Module, "scene.graph")
                    ->Property("className", [](const PythonBehaviorInfo& self)
                        { return self.m_behaviorClass->m_name; }, nullptr)
                    ->Property("classUuid", [](const PythonBehaviorInfo& self)
                        { return self.m_behaviorClass->m_typeId.ToString<AZStd::string>(); }, nullptr)
                    ->Property("methodList", BehaviorValueGetter(&PythonBehaviorInfo::m_methodList), nullptr)
                    ->Property("propertyList", BehaviorValueGetter(&PythonBehaviorInfo::m_propertyList), nullptr)
                    ;
            }
        }

        bool PythonBehaviorInfo::IsMemberLike(const AZ::BehaviorMethod& method, const AZ::TypeId& typeId) const
        {
            return method.IsMember() || (method.GetNumArguments() > 0 && method.GetArgument(0)->m_typeId == typeId);
        }

        AZStd::string PythonBehaviorInfo::FetchPythonType(const AZ::BehaviorParameter& param) const
        {
            using namespace AzToolsFramework;
            EditorPythonConsoleInterface* editorPythonConsoleInterface = AZ::Interface<EditorPythonConsoleInterface>::Get();
            if (editorPythonConsoleInterface)
            {
                return editorPythonConsoleInterface->FetchPythonTypeName(param);
            }
            return None;
        }

        void PythonBehaviorInfo::PrepareMethod(AZStd::string_view methodName, const AZ::BehaviorMethod& behaviorMethod)
        {
            // if the method is a static method then it is not a part of the abstract class
            const bool isMemberLike = IsMemberLike(behaviorMethod, m_behaviorClass->m_typeId);
            if (isMemberLike == false)
            {
                return;
            }

            AZStd::string buffer;
            AZStd::vector<AZStd::string> pythonArgs;

            AzFramework::StringFunc::Append(buffer, "def ");
            AzFramework::StringFunc::Append(buffer, methodName.data());
            AzFramework::StringFunc::Append(buffer, "(");

            pythonArgs.emplace_back("self");

            AZStd::string bufferArg;
            for (size_t argIndex = 1; argIndex < behaviorMethod.GetNumArguments(); ++argIndex)
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

                AZStd::string type = FetchPythonType(*behaviorMethod.GetArgument(argIndex));
                if (!type.empty())
                {
                    AzFramework::StringFunc::Append(bufferArg, ": ");
                    AzFramework::StringFunc::Append(bufferArg, type.data());
                }

                pythonArgs.push_back(bufferArg);
                bufferArg.clear();
            }

            AZStd::string resultValue{ None };
            if (behaviorMethod.HasResult() && behaviorMethod.GetResult())
            {
                resultValue = FetchPythonType(*behaviorMethod.GetResult());
            }

            AZStd::string argsList;
            AzFramework::StringFunc::Join(buffer, pythonArgs.begin(), pythonArgs.end(), ",");
            AzFramework::StringFunc::Append(buffer, ") -> ");
            AzFramework::StringFunc::Append(buffer, resultValue.c_str());
            m_methodList.emplace_back(buffer);
        }

        void PythonBehaviorInfo::PrepareProperty(AZStd::string_view propertyName, const AZ::BehaviorProperty& behaviorProperty)
        {
            AZStd::string buffer;
            AZStd::vector<AZStd::string> pythonArgs;

            AzFramework::StringFunc::Append(buffer, propertyName.data());

            AzFramework::StringFunc::Append(buffer, "(");
            if (behaviorProperty.m_setter)
            {
                AZStd::string type = FetchPythonType(*behaviorProperty.m_setter->GetArgument(1));
                AzFramework::StringFunc::Append(buffer, type.data());
            }
            AzFramework::StringFunc::Append(buffer, ")");

            if (behaviorProperty.m_getter)
            {
                AZStd::string type = FetchPythonType(*behaviorProperty.m_getter->GetResult());
                AzFramework::StringFunc::Append(buffer, "->");
                AzFramework::StringFunc::Append(buffer, type.data());
            }

            m_propertyList.emplace_back(buffer);
        }
    }

    namespace SceneAPI
    {
        namespace Containers
        {
            void GraphObjectProxy::Reflect(AZ::ReflectContext* context)
            {
                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
                if (behaviorContext)
                {
                    Python::PythonBehaviorInfo::Reflect(context);

                    behaviorContext->Class<DataTypes::IGraphObject>();

                    behaviorContext->Class<GraphObjectProxy>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                        ->Attribute(AZ::Script::Attributes::Module, "scene.graph")
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ->Method("CastWithTypeName", &GraphObjectProxy::CastWithTypeName)
                        ->Method("Invoke", &GraphObjectProxy::Invoke)
                        ->Method("Fetch", &GraphObjectProxy::Fetch)
                        ->Method("GetClassInfo", [](GraphObjectProxy& self) -> Python::PythonBehaviorInfo*
                            {
                                if (self.m_pythonBehaviorInfo)
                                {
                                    return self.m_pythonBehaviorInfo.get();
                                }
                                else if (self.m_behaviorClass)
                                {
                                    self.m_pythonBehaviorInfo = AZStd::make_shared<Python::PythonBehaviorInfo>(self.m_behaviorClass);
                                    return self.m_pythonBehaviorInfo.get();
                                }
                                return nullptr;
                            })
                        ;
                }
            }

            GraphObjectProxy::GraphObjectProxy(AZStd::shared_ptr<const DataTypes::IGraphObject> graphObject)
            {
                m_graphObject = graphObject;
            }

            GraphObjectProxy::GraphObjectProxy(const GraphObjectProxy& other)
            {
                m_graphObject = other.m_graphObject;
                m_behaviorClass = other.m_behaviorClass;
                m_pythonBehaviorInfo = other.m_pythonBehaviorInfo;
            }

            GraphObjectProxy::~GraphObjectProxy()
            {
                m_pythonBehaviorInfo.reset();
                m_graphObject.reset();
                m_behaviorClass = nullptr;
            }

            bool GraphObjectProxy::CastWithTypeName(const AZStd::string& classTypeName)
            {
                const AZ::BehaviorClass* behaviorClass = BehaviorContextHelper::GetClass(classTypeName);
                if (behaviorClass)
                {
                    const void* baseClass = behaviorClass->m_azRtti->Cast(m_graphObject.get(), behaviorClass->m_azRtti->GetTypeId());
                    if (baseClass)
                    {
                        m_behaviorClass = behaviorClass;
                        return true;
                    }
                }
                return false;
            }

            AZStd::any GraphObjectProxy::Fetch(AZStd::string_view property)
            {
                if (!m_behaviorClass)
                {
                    AZ_Warning("SceneAPI", false, "Empty behavior class. Use the CastWithTypeName() to assign the concrete type of IGraphObject to Invoke().");
                    return AZStd::any(false);
                }

                auto entry = m_behaviorClass->m_properties.find(property);
                if (m_behaviorClass->m_properties.end() == entry)
                {
                    AZ_Warning("SceneAPI", false, "Missing property %.*s from class %s",
                        aznumeric_cast<int>(property.size()),
                        property.data(),
                        m_behaviorClass->m_name.c_str());

                    return AZStd::any(false);
                }

                if (!entry->second->m_getter)
                {
                    AZ_Warning("SceneAPI", false, "Property %.*s from class %s has a NULL getter",
                        aznumeric_cast<int>(property.size()),
                        property.data(),
                        m_behaviorClass->m_name.c_str());

                    return AZStd::any(false);
                }

                return InvokeBehaviorMethod(entry->second->m_getter, {});
            }

            AZStd::any GraphObjectProxy::InvokeBehaviorMethod(AZ::BehaviorMethod* behaviorMethod, AZStd::vector<AZStd::any> argList)
            {
                constexpr size_t behaviorParamListSize = 8;
                if (behaviorMethod->GetNumArguments() > behaviorParamListSize)
                {
                    AZ_Error("SceneAPI", false, "Unsupported behavior method; supports max %zu but %s has %zu argument slots",
                        behaviorParamListSize,
                        behaviorMethod->m_name.c_str(),
                        behaviorMethod->GetNumArguments());
                    return AZStd::any(false);
                }
                AZ::BehaviorArgument behaviorParamList[behaviorParamListSize];

                // detect if the method passes in a "this" pointer which can be true if the method is a member function
                // or if the first argument is the same as the behavior class such as from a lambda function
                bool hasSelfPointer = behaviorMethod->IsMember();
                if (!hasSelfPointer)
                {
                    hasSelfPointer = behaviorMethod->GetArgument(0)->m_typeId == m_behaviorClass->m_typeId;
                }

                // record the "this" pointer's meta data like its RTTI so that it can be
                // down casted to a parent class type if needed to invoke a parent method

                // When storing a parameter, if the behavior parameter indicates that it is a pointer, (TR_POINTER flag is set)
                // it is expected that what is stored in that parameter is a pointer to the value, more specifically
                // it will decode it on the other end by dereferencing it twice, like this:
                //    if (m_traits & BehaviorParameter::TR_POINTER)
                //    {
                //       valueAddress = *reinterpret_cast<void**>(valueAddress); // pointer to a pointer
                //    }
                // Notice the it expects it to be a void** (double reference) and dereferences it to get the address of the object.
                // 
                // That means that when TR_POINTER is set, not only does the object pointed to have to survive until used
                // but the memory storing that pointer also has to survive until used.
                // for example, this would be a scope use-after-free bug:
                // {
                //    AZ::Entity entity;
                //    AZ::BehaviorArgument arg;
                //    arg.m_traits = BehaviorParameter::TR_POINTER;
                // 
                //    {
                //       const void* objectPtr = &entity;   // this is a 64-bit object living in the stack, holding the addr of entity.
                //       arg.m_value = const_cast<void*>(&objectPtr); // notice, we are referencing objectPtr again with & since its TR_POINTER
                //    }
                //
                //    arg.GetValueAddress(); // double dereference causes memory error.
                //
                //  The above is a memory error because the objectPtr is a stack variable, and the ADDRESS of it is what's stored in m_value.

                // to avoid this, we cache the self pointer to the graph object ahead of time.
                const void* self = reinterpret_cast<const void*>(m_graphObject.get());

                if (const AZ::BehaviorParameter* thisInfo = behaviorMethod->GetArgument(0); hasSelfPointer)
                {
                    // avoiding the "Special handling for the generic object holder." since it assumes
                    // the BehaviorObject.m_value is a pointer; the reference version is already dereferenced
                    AZ::BehaviorArgument theThisPointer;
                    if ((thisInfo->m_traits & AZ::BehaviorParameter::TR_POINTER) == AZ::BehaviorParameter::TR_POINTER)
                    {
                        theThisPointer.m_value = &self;
                    }
                    else
                    {
                        theThisPointer.m_value = const_cast<void*>(self);
                    }
                    theThisPointer.Set(*thisInfo);
                    behaviorParamList[0].Set(theThisPointer);
                }

                int paramCount = 0;
                for (; paramCount < argList.size() && paramCount < behaviorMethod->GetNumArguments(); ++paramCount)
                {
                    size_t behaviorArgIndex = hasSelfPointer ? paramCount + 1 : paramCount;
                    const AZ::BehaviorParameter* argBehaviorInfo = behaviorMethod->GetArgument(behaviorArgIndex);
                    if (!Convert(argList[paramCount], argBehaviorInfo, behaviorParamList[behaviorArgIndex]))
                    {
                        AZ_Error("SceneAPI", false, "Could not convert from %s to %s at index %zu",
                            argBehaviorInfo->m_typeId.ToString<AZStd::string>().c_str(),
                            behaviorParamList[behaviorArgIndex].m_typeId.ToString<AZStd::string>().c_str(),
                            paramCount);
                        return AZStd::any(false);
                    }
                }

                if (hasSelfPointer)
                {
                    ++paramCount;
                }

                AZ::BehaviorArgument returnBehaviorValue;
                if (behaviorMethod->HasResult())
                {
                    returnBehaviorValue.Set(*behaviorMethod->GetResult());
                    const size_t typeSize = returnBehaviorValue.m_azRtti->GetTypeSize();

                    if (returnBehaviorValue.m_traits & BehaviorParameter::TR_POINTER)
                    {
                        // Used to allocate storage to store a copy of a pointer and the allocated memory address in one block.
                        constexpr size_t PointerAllocationStorage = 2 * sizeof(void*);
                        void* valueAddress = returnBehaviorValue.m_tempData.allocate(PointerAllocationStorage, 16, 0);
                        void* valueAddressPtr = reinterpret_cast<AZ::u8*>(valueAddress) + sizeof(void*);
                        ::memset(valueAddress, 0, sizeof(void*));
                        *reinterpret_cast<void**>(valueAddressPtr) = valueAddress;
                        returnBehaviorValue.m_value = valueAddressPtr;
                    }
                    else if (returnBehaviorValue.m_traits & BehaviorParameter::TR_REFERENCE)
                    {
                        // the reference value will just be assigned
                        returnBehaviorValue.m_value = nullptr;
                    }
                    else if (typeSize < returnBehaviorValue.m_tempData.max_size())
                    {
                        returnBehaviorValue.m_value = returnBehaviorValue.m_tempData.allocate(typeSize, 16);
                    }
                    else
                    {
                        AZ_Warning("SceneAPI", false, "Can't invoke method since the return value is too big; %d bytes", typeSize);
                        return AZStd::any(false);
                    }
                }

                if (!behaviorMethod->Call(behaviorParamList, paramCount, &returnBehaviorValue))
                {
                    return AZStd::any(false);
                }

                if (!behaviorMethod->HasResult())
                {
                    return AZStd::any(true);
                }

                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
                if (!serializeContext)
                {
                    AZ_Error("SceneAPI", false, "AZ::SerializeContext should be prepared.");
                    return AZStd::any(false);
                }

                // Create temporary any to get its type info to construct a new AZStd::any with new data
                AZStd::any tempAny = serializeContext->CreateAny(returnBehaviorValue.m_typeId);
                return AZStd::move(AZStd::any(returnBehaviorValue.m_value, tempAny.get_type_info()));
            }

            AZStd::any GraphObjectProxy::Invoke(AZStd::string_view method, AZStd::vector<AZStd::any> argList)
            {
                if (!m_behaviorClass)
                {
                    AZ_Warning("SceneAPI", false, "Use the CastWithTypeName() to assign the concrete type of IGraphObject to Invoke().");
                    return AZStd::any(false);
                }

                auto entry = m_behaviorClass->m_methods.find(method);
                if (m_behaviorClass->m_methods.end() == entry)
                {
                    AZ_Warning("SceneAPI", false, "Missing method %.*s from class %s",
                        aznumeric_cast<int>(method.size()),
                        method.data(),
                        m_behaviorClass->m_name.c_str());

                    return AZStd::any(false);
                }

                return InvokeBehaviorMethod(entry->second, argList);
            }

            template <typename FROM, typename TO>
            bool ConvertFromTo(AZStd::any& input, const AZ::BehaviorParameter* argBehaviorInfo, AZ::BehaviorArgument& behaviorParam)
            {
                if (input.get_type_info().m_id != azrtti_typeid<FROM>())
                {
                    return false;
                }
                if (argBehaviorInfo->m_typeId != azrtti_typeid<TO>())
                {
                    return false;
                }
                TO* storage = reinterpret_cast<TO*>(behaviorParam.m_tempData.allocate(argBehaviorInfo->m_azRtti->GetTypeSize(), 16));
                *storage = aznumeric_cast<TO>(*AZStd::any_cast<FROM>(&input));
                behaviorParam.m_typeId = azrtti_typeid<TO>();
                behaviorParam.m_value = storage;
                return true;
            }

            bool GraphObjectProxy::Convert(AZStd::any& input, const AZ::BehaviorParameter* argBehaviorInfo, AZ::BehaviorArgument& behaviorParam)
            {
                if (input.get_type_info().m_id == argBehaviorInfo->m_typeId)
                {
                    behaviorParam.m_typeId = input.get_type_info().m_id;
                    behaviorParam.m_value = AZStd::any_cast<void>(&input);
                    return true;
                }

#define CONVERT_ANY_NUMERIC(TYPE) ( \
                ConvertFromTo<TYPE, double>(input, argBehaviorInfo, behaviorParam) || \
                ConvertFromTo<TYPE, float>(input, argBehaviorInfo, behaviorParam)  || \
                ConvertFromTo<TYPE, AZ::s8>(input, argBehaviorInfo, behaviorParam) || \
                ConvertFromTo<TYPE, AZ::u8>(input, argBehaviorInfo, behaviorParam) || \
                ConvertFromTo<TYPE, AZ::s16>(input, argBehaviorInfo, behaviorParam) || \
                ConvertFromTo<TYPE, AZ::u16>(input, argBehaviorInfo, behaviorParam) || \
                ConvertFromTo<TYPE, AZ::s32>(input, argBehaviorInfo, behaviorParam) || \
                ConvertFromTo<TYPE, AZ::u32>(input, argBehaviorInfo, behaviorParam) || \
                ConvertFromTo<TYPE, AZ::s64>(input, argBehaviorInfo, behaviorParam) || \
                ConvertFromTo<TYPE, AZ::u64>(input, argBehaviorInfo, behaviorParam) )

                if (CONVERT_ANY_NUMERIC(double)  || CONVERT_ANY_NUMERIC(float)   ||
                    CONVERT_ANY_NUMERIC(AZ::s8)  || CONVERT_ANY_NUMERIC(AZ::u8)  ||
                    CONVERT_ANY_NUMERIC(AZ::s16) || CONVERT_ANY_NUMERIC(AZ::u16) ||
                    CONVERT_ANY_NUMERIC(AZ::s32) || CONVERT_ANY_NUMERIC(AZ::u32) ||
                    CONVERT_ANY_NUMERIC(AZ::s64) || CONVERT_ANY_NUMERIC(AZ::u64) )
                {
                    return true;
                }
#undef CONVERT_ANY_NUMERIC

                return false;
            }
        } // Containers
    } // SceneAPI
} // AZ
