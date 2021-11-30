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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

namespace AZ
{
    namespace Python
    {
        static const char* const None = "None";

        class PythonBehaviorInfo final
        {
        public:
            AZ_RTTI(PythonBehaviorInfo, "{8055BD03-5B3B-490D-AEC5-1B1E2616D529}");
            AZ_CLASS_ALLOCATOR(PythonBehaviorInfo, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            PythonBehaviorInfo(const AZ::BehaviorClass* behaviorClass);
            PythonBehaviorInfo() = delete;

        protected:
            bool IsMemberLike(const AZ::BehaviorMethod& method, const AZ::TypeId& typeId) const;
            AZStd::string FetchPythonType(const AZ::BehaviorParameter& param) const;
            void WriteMethod(AZStd::string_view methodName, const AZ::BehaviorMethod& behaviorMethod);

        private:
            const AZ::BehaviorClass* m_behaviorClass = nullptr;
            AZStd::vector<AZStd::string> m_methodList;
        };

        PythonBehaviorInfo::PythonBehaviorInfo(const AZ::BehaviorClass* behaviorClass)
            : m_behaviorClass(behaviorClass)
        {
            AZ_Assert(m_behaviorClass, "PythonBehaviorInfo requires a valid behaviorClass pointer");
            for (const auto& entry : behaviorClass->m_methods)
            {
                WriteMethod(entry.first, *entry.second);
            }
        }

        void PythonBehaviorInfo::Reflect(AZ::ReflectContext* context)
        {
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<PythonBehaviorInfo>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "scene.graph")
                    ->Property("className", [](const PythonBehaviorInfo& self)
                        { return self.m_behaviorClass->m_name; }, nullptr)
                    ->Property("classUuid", [](const PythonBehaviorInfo& self)
                        { return self.m_behaviorClass->m_typeId.ToString<AZStd::string>(); }, nullptr)
                    ->Property("methodList", BehaviorValueGetter(&PythonBehaviorInfo::m_methodList), nullptr);
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

        void PythonBehaviorInfo::WriteMethod(AZStd::string_view methodName, const AZ::BehaviorMethod& behaviorMethod)
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

                AZStd::string_view type = FetchPythonType(*behaviorMethod.GetArgument(argIndex));
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
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene.graph")
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ->Method("CastWithTypeName", &GraphObjectProxy::CastWithTypeName)
                        ->Method("Invoke", &GraphObjectProxy::Invoke)
                        ->Method("GetClassInfo", [](GraphObjectProxy& self) -> Python::PythonBehaviorInfo*
                            {
                                if (self.m_pythonBehaviorInfo)
                                {
                                    return self.m_pythonBehaviorInfo.get();
                                }
                                if (self.m_behaviorClass)
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

            GraphObjectProxy::~GraphObjectProxy()
            {
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

            AZStd::any GraphObjectProxy::Invoke(AZStd::string_view method, AZStd::vector<AZStd::any> argList)
            {
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
                if (!serializeContext)
                {
                    AZ_Error("SceneAPI", false, "AZ::SerializeContext should be prepared.");
                    return AZStd::any(false);
                }

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

                AZ::BehaviorMethod* behaviorMethod = entry->second;

                constexpr size_t behaviorParamListSize = 8;
                if (behaviorMethod->GetNumArguments() > behaviorParamListSize)
                {
                    AZ_Error("SceneAPI", false, "Unsupported behavior method; supports max %zu but %s has %zu argument slots",
                        behaviorParamListSize,
                        behaviorMethod->m_name.c_str(),
                        behaviorMethod->GetNumArguments());
                    return AZStd::any(false);
                }
                AZ::BehaviorValueParameter behaviorParamList[behaviorParamListSize];

                // detect if the method passes in a "this" pointer which can be true if the method is a member function
                // or if the first argument is the same as the behavior class such as from a lambda function
                bool hasSelfPointer = behaviorMethod->IsMember();
                if (!hasSelfPointer)
                {
                    hasSelfPointer = behaviorMethod->GetArgument(0)->m_typeId == m_behaviorClass->m_typeId;
                }

                // record the "this" pointer's metadata like its RTTI so that it can be
                // down casted to a parent class type if needed to invoke a parent method
                if (const AZ::BehaviorParameter* thisInfo = behaviorMethod->GetArgument(0); hasSelfPointer)
                {
                    // avoiding the "Special handling for the generic object holder." since it assumes
                    // the BehaviorObject.m_value is a pointer; the reference version is already dereferenced
                    AZ::BehaviorValueParameter theThisPointer;
                    const void* self = reinterpret_cast<const void*>(m_graphObject.get());
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

                AZ::BehaviorValueParameter returnBehaviorValue;
                if (behaviorMethod->HasResult())
                {
                    returnBehaviorValue.Set(*behaviorMethod->GetResult());
                    returnBehaviorValue.m_value =
                        returnBehaviorValue.m_tempData.allocate(returnBehaviorValue.m_azRtti->GetTypeSize(), 16);
                }

                if (!entry->second->Call(behaviorParamList, paramCount, &returnBehaviorValue))
                {
                    return AZStd::any(false);
                }

                if (!behaviorMethod->HasResult())
                {
                    return AZStd::any(true);
                }

                // Create temporary any to get its type info to construct a new AZStd::any with new data
                AZStd::any tempAny = serializeContext->CreateAny(returnBehaviorValue.m_typeId);
                return AZStd::move(AZStd::any(returnBehaviorValue.m_value, tempAny.get_type_info()));
            }

            template <typename FROM, typename TO>
            bool ConvertFromTo(AZStd::any& input, const AZ::BehaviorParameter* argBehaviorInfo, AZ::BehaviorValueParameter& behaviorParam)
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

            bool GraphObjectProxy::Convert(AZStd::any& input, const AZ::BehaviorParameter* argBehaviorInfo, AZ::BehaviorValueParameter& behaviorParam)
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
