/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonLogSymbolsComponent.h>

#include <Source/PythonCommon.h>
#include <Source/PythonUtility.h>
#include <Source/PythonTypeCasters.h>
#include <Source/PythonProxyBus.h>
#include <Source/PythonProxyObject.h>
#include <pybind11/embed.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/sort.h>
#include <AzCore/Serialization/Utils.h>

#include <AzFramework/CommandLine/CommandRegistrationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace EditorPythonBindings
{
    namespace Internal
    {
        struct FileHandle final
        {
            explicit FileHandle(AZ::IO::HandleType handle)
                : m_handle(handle)
            {}

            ~FileHandle()
            {
                Close();
            }

            void Close()
            {
                if (IsValid())
                {
                    AZ::IO::FileIOBase::GetInstance()->Close(m_handle);
                }
                m_handle = AZ::IO::InvalidHandle;
            }

            bool IsValid() const
            {
                return m_handle != AZ::IO::InvalidHandle;
            }

            operator AZ::IO::HandleType() const { return m_handle; }

            AZ::IO::HandleType m_handle;
        };

        void Indent(int level, AZStd::string& buffer)
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
    }

    void PythonLogSymbolsComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto&& serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PythonLogSymbolsComponent, AZ::Component>()
                ->Version(0);
        }
    }

    void PythonLogSymbolsComponent::Activate()
    {
        PythonSymbolEventBus::Handler::BusConnect();
        EditorPythonBindingsNotificationBus::Handler::BusConnect();
        AZ::Interface<AzToolsFramework::EditorPythonConsoleInterface>::Register(this);
    }

    void PythonLogSymbolsComponent::Deactivate()
    {
        AZ::Interface<AzToolsFramework::EditorPythonConsoleInterface>::Unregister(this);
        PythonSymbolEventBus::Handler::BusDisconnect();
        EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
    }

    void PythonLogSymbolsComponent::OnPostInitialize()
    {
        m_basePath.clear();
        if (AZ::IO::FileIOBase::GetInstance()->GetAlias("@user@"))
        {
            // clear out the previous symbols path
            char pythonSymbolsPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@user@/python_symbols", pythonSymbolsPath, AZ_MAX_PATH_LEN);
            AZ::IO::FileIOBase::GetInstance()->CreatePath(pythonSymbolsPath);
            m_basePath = pythonSymbolsPath;
        }
        EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
    }

    void PythonLogSymbolsComponent::WriteMethod(AZ::IO::HandleType handle, AZStd::string_view methodName, const AZ::BehaviorMethod& behaviorMethod, const AZ::BehaviorClass* behaviorClass)
    {
        AZStd::string buffer;
        int indentLevel = 0;
        AZStd::vector<AZStd::string> pythonArgs;
        const bool isMemberLike = behaviorClass ? PythonProxyObjectManagement::IsMemberLike(behaviorMethod, behaviorClass->m_typeId) : false;

        if (isMemberLike)
        {
            indentLevel = 1;
            Internal::Indent(indentLevel, buffer);
            pythonArgs.emplace_back("self");
        }
        else
        {
            indentLevel = 0;
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
        Internal::Indent(indentLevel + 1, buffer);
        AzFramework::StringFunc::Append(buffer, "pass\n\n");

        AZ::IO::FileIOBase::GetInstance()->Write(handle, buffer.c_str(), buffer.size());
    }

    void PythonLogSymbolsComponent::WriteProperty(AZ::IO::HandleType handle, int level, AZStd::string_view propertyName, const AZ::BehaviorProperty& property, [[maybe_unused]] const AZ::BehaviorClass * behaviorClass)
    {
        AZStd::string buffer;

        // property declaration
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

        AZ::IO::FileIOBase::GetInstance()->Write(handle, buffer.c_str(), buffer.size());
    }

    void PythonLogSymbolsComponent::LogClass(AZStd::string_view moduleName, AZ::BehaviorClass* behaviorClass)
    {
        LogClassWithName(moduleName, behaviorClass, behaviorClass->m_name.c_str());
    }

    void PythonLogSymbolsComponent::LogClassWithName(AZStd::string_view moduleName, AZ::BehaviorClass* behaviorClass, AZStd::string_view className)
    {
        Internal::FileHandle fileHandle(OpenModuleAt(moduleName));
        if (fileHandle.IsValid())
        {
            // Behavior Class types with member methods and properties
            AZStd::string buffer;
            AzFramework::StringFunc::Append(buffer, "class ");
            AzFramework::StringFunc::Append(buffer, className.data());
            AzFramework::StringFunc::Append(buffer, ":\n");
            AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());
            buffer.clear();

            if (behaviorClass->m_methods.empty() && behaviorClass->m_properties.empty())
            {
                AZStd::string body{ "    # behavior class type with no methods or properties \n" };
                Internal::Indent(1, body);
                AzFramework::StringFunc::Append(body, "pass\n\n");
                AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, body.c_str(), body.size());
            }
            else
            {
                for (const auto& properyEntry : behaviorClass->m_properties)
                {
                    AZ::BehaviorProperty* property = properyEntry.second;
                    AZStd::string propertyName{ properyEntry.first };
                    Scope::FetchScriptName(property->m_attributes, propertyName);
                    WriteProperty(fileHandle, 1, propertyName, *property, behaviorClass);
                }

                for (const auto& methodEntry : behaviorClass->m_methods)
                {
                    AZ::BehaviorMethod* method = methodEntry.second;
                    if (method && PythonProxyObjectManagement::IsMemberLike(*method, behaviorClass->m_typeId))
                    {
                        AZStd::string baseMethodName{ methodEntry.first };
                        Scope::FetchScriptName(method->m_attributes, baseMethodName);
                        WriteMethod(fileHandle, baseMethodName, *method, behaviorClass);
                    }
                }
            }
        }
    }

    void PythonLogSymbolsComponent::LogClassMethod(AZStd::string_view moduleName, AZStd::string_view globalMethodName, AZ::BehaviorClass* behaviorClass, AZ::BehaviorMethod* behaviorMethod)
    {
        AZ_UNUSED(behaviorClass);
        Internal::FileHandle fileHandle(OpenModuleAt(moduleName));
        if (fileHandle.IsValid())
        {
            WriteMethod(fileHandle, globalMethodName, *behaviorMethod, nullptr);
        }
    }

    void PythonLogSymbolsComponent::LogBus(AZStd::string_view moduleName, AZStd::string_view busName, AZ::BehaviorEBus* behaviorEBus)
    {
        if (behaviorEBus->m_events.empty())
        {
            return;
        }

        Internal::FileHandle fileHandle(OpenModuleAt(moduleName));
        if (fileHandle.IsValid())
        {
            AZStd::string buffer;

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
            AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());
            buffer.clear();

            auto eventInfoBuilder = [this](const AZ::BehaviorMethod* behaviorMethod, AZStd::string& inOutStrBuffer, [[maybe_unused]] TypeMap& typeCache)
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
                AZStd::string returnTypeStr = AZStd::string::format(") -> " AZ_STRING_FORMAT" \n",  AZ_STRING_ARG(returnType));
                AzFramework::StringFunc::Append(inOutStrBuffer, returnTypeStr.c_str());
            };

            // record the event names the behavior can send, their parameters and return type
            AZStd::string comment = behaviorEBus->m_toolTip;

            if (!behaviorEBus->m_events.empty())
            {
                AzFramework::StringFunc::Append(comment, "The following bus Call types, Event names and Argument types are supported by this bus:\n");
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

            AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());

            // can the EBus create & destroy a handler?
            if (behaviorEBus->m_createHandler && behaviorEBus->m_destroyHandler)
            {
                buffer.clear();
                AzFramework::StringFunc::Append(buffer, "def ");
                AzFramework::StringFunc::Append(buffer, busName.data());
                AzFramework::StringFunc::Append(buffer, "Handler() -> None:\n");
                Internal::Indent(1, buffer);
                AzFramework::StringFunc::Append(buffer, "pass\n\n");
                AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());
            }
        }
    }

    void PythonLogSymbolsComponent::LogGlobalMethod(AZStd::string_view moduleName, AZStd::string_view methodName, AZ::BehaviorMethod* behaviorMethod)
    {
        Internal::FileHandle fileHandle(OpenModuleAt(moduleName));
        if (fileHandle.IsValid())
        {
            WriteMethod(fileHandle, methodName, *behaviorMethod, nullptr);
        }

        auto functionMapIt = m_globalFunctionMap.find(moduleName);
        if (functionMapIt == m_globalFunctionMap.end())
        {
            auto moduleSetIt = m_moduleSet.find(moduleName);
            if (moduleSetIt != m_moduleSet.end())
            {
                m_globalFunctionMap[*moduleSetIt] = { AZStd::make_pair(behaviorMethod, methodName) };
            }
        }
        else
        {
            GlobalFunctionList& globalFunctionList = functionMapIt->second;
            globalFunctionList.emplace_back(AZStd::make_pair(behaviorMethod, methodName));
        }
    }

    void PythonLogSymbolsComponent::LogGlobalProperty(AZStd::string_view moduleName, AZStd::string_view propertyName, AZ::BehaviorProperty* behaviorProperty)
    {
        if (!behaviorProperty->m_getter || !behaviorProperty->m_getter->GetResult())
        {
            return;
        }

        Internal::FileHandle fileHandle(OpenModuleAt(moduleName));
        if (fileHandle.IsValid())
        {
            AZStd::string buffer;

            // add header 
            AZ::u64 filesize = 0;
            AZ::IO::FileIOBase::GetInstance()->Size(fileHandle, filesize);
            if (filesize == 0)
            {
                AzFramework::StringFunc::Append(buffer, "class property():\n");
            }

            Internal::Indent(1, buffer);
            AzFramework::StringFunc::Append(buffer, propertyName.data());
            AzFramework::StringFunc::Append(buffer, ": ClassVar[");

            const AZ::BehaviorParameter* resultParam = behaviorProperty->m_getter->GetResult();
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

            if (behaviorProperty->m_getter && !behaviorProperty->m_setter)
            {
                AzFramework::StringFunc::Append(buffer, " # read only");
            }
            AzFramework::StringFunc::Append(buffer, "\n");

            AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());
        }
    }

    void PythonLogSymbolsComponent::Finalize()
    {
        Internal::FileHandle fileHandle(OpenInitFileAt("azlmbr.bus"));
        if (fileHandle)
        {
            AZStd::string buffer;
            AzFramework::StringFunc::Append(buffer, "# Bus dispatch types:\n");
            AzFramework::StringFunc::Append(buffer, "from typing_extensions import Final\n");
            AzFramework::StringFunc::Append(buffer, "Broadcast: Final[int] = 0\n");
            AzFramework::StringFunc::Append(buffer, "Event: Final[int] = 1\n");
            AzFramework::StringFunc::Append(buffer, "QueueBroadcast: Final[int] = 2\n");
            AzFramework::StringFunc::Append(buffer, "QueueEvent: Final[int] = 3\n");
            AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, buffer.c_str(), buffer.size());
        }
        fileHandle.Close();
    }

    void PythonLogSymbolsComponent::GetModuleList(AZStd::vector<AZStd::string_view>& moduleList) const
    {
        moduleList.clear();
        moduleList.reserve(m_moduleSet.size());
        AZStd::copy(m_moduleSet.begin(), m_moduleSet.end(), AZStd::back_inserter(moduleList));
    }

    void PythonLogSymbolsComponent::GetGlobalFunctionList(GlobalFunctionCollection& globalFunctionCollection) const
    {
        globalFunctionCollection.clear();

        for (const auto& globalFunctionMapEntry : m_globalFunctionMap)
        {
            const AZStd::string_view moduleName{ globalFunctionMapEntry.first };
            const GlobalFunctionList& moduleFunctionList = globalFunctionMapEntry.second;

            AZStd::transform(moduleFunctionList.begin(), moduleFunctionList.end(), AZStd::back_inserter(globalFunctionCollection), [moduleName](auto& entry) -> auto
            {
                const GlobalFunctionEntry& globalFunctionEntry = entry;
                const AZ::BehaviorMethod* behaviorMethod = entry.first;
                return AzToolsFramework::EditorPythonConsoleInterface::GlobalFunction({ moduleName, globalFunctionEntry.second, behaviorMethod->m_debugDescription });
            });
        }
    }

    AZStd::string PythonLogSymbolsComponent::FetchListType(const AZ::TypeId& typeId)
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

    AZStd::string PythonLogSymbolsComponent::FetchMapType(const AZ::TypeId& typeId)
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
                type = AZStd::string::format("Dict[" AZ_STRING_FORMAT ", " AZ_STRING_FORMAT "]",
                    AZ_STRING_ARG(kType), AZ_STRING_ARG(vType));
            }
        }

        return type;
    }

    AZStd::string PythonLogSymbolsComponent::FetchOutcomeType(const AZ::TypeId& typeId)
    {
        AZStd::string type = "Outcome";
        AZStd::pair<AZ::Uuid, AZ::Uuid> outcomeTypes = AZ::Utils::GetOutcomeTypes(typeId);

        // trait info not available, so defaulting to TR_NONE
        AZStd::string_view valueT = FetchPythonTypeAndTraits(outcomeTypes.first, AZ::BehaviorParameter::TR_NONE);
        AZStd::string_view errorT = FetchPythonTypeAndTraits(outcomeTypes.second, AZ::BehaviorParameter::TR_NONE);
        if (!valueT.empty() && !errorT.empty())
        {
            type = AZStd::string::format("Outcome[" AZ_STRING_FORMAT ", " AZ_STRING_FORMAT "]",
                AZ_STRING_ARG(valueT), AZ_STRING_ARG(errorT));
        }

        return type;
    }

    AZStd::string PythonLogSymbolsComponent::TypeNameFallback(const AZ::TypeId& typeId)
    {
        // fall back to class data m_name
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        if (serializeContext)
        {
            auto classData = serializeContext->FindClassData(typeId);
            if (classData)
            {
                return classData->m_name;
            }
        }

        return "";
    }

    AZStd::string_view PythonLogSymbolsComponent::FetchPythonTypeAndTraits(const AZ::TypeId& typeId, AZ::u32 traits)
    {
        if (m_typeCache.find(typeId) == m_typeCache.end())
        {
            AZStd::string type;
            if (AZ::AzTypeInfo<AZStd::string_view>::Uuid() == typeId ||
                AZ::AzTypeInfo<AZStd::string>::Uuid() == typeId)
            {
                type = "str";
            }
            else if (AZ::AzTypeInfo<char>::Uuid() == typeId &&
                traits & AZ::BehaviorParameter::TR_POINTER &&
                traits & AZ::BehaviorParameter::TR_CONST)
            {
                type = "str";
            }
            else if (AZ::AzTypeInfo<float>::Uuid() == typeId ||
                AZ::AzTypeInfo<double>::Uuid() == typeId)
            {
                type = "float";
            }
            else if (AZ::AzTypeInfo<bool>::Uuid() == typeId)
            {
                type = "bool";
            }
            else if (AZ::AzTypeInfo<AZ::s8>::Uuid() == typeId ||
                AZ::AzTypeInfo<AZ::u8>::Uuid() == typeId ||
                AZ::AzTypeInfo<AZ::s16>::Uuid() == typeId ||
                AZ::AzTypeInfo<AZ::u16>::Uuid() == typeId ||
                AZ::AzTypeInfo<AZ::s32>::Uuid() == typeId ||
                AZ::AzTypeInfo<AZ::u32>::Uuid() == typeId ||
                AZ::AzTypeInfo<AZ::s64>::Uuid() == typeId ||
                AZ::AzTypeInfo<AZ::u64>::Uuid() == typeId)
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
                type = TypeNameFallback(typeId);
            }

            m_typeCache[typeId] = type;
        }

        return m_typeCache[typeId];
    }

    AZStd::string PythonLogSymbolsComponent::FetchPythonTypeName(const AZ::BehaviorParameter& param)
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

    AZ::IO::HandleType PythonLogSymbolsComponent::OpenInitFileAt(AZStd::string_view moduleName)
    {
        if (m_basePath.empty())
        {
            return AZ::IO::InvalidHandle;
        }

        // creates the __init__.py file in this path
        AZStd::string modulePath(moduleName);
        AzFramework::StringFunc::Replace(modulePath, ".", AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

        AZStd::string initFile;
        AzFramework::StringFunc::Path::Join(m_basePath.c_str(), modulePath.c_str(), initFile);
        AzFramework::StringFunc::Append(initFile, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        AzFramework::StringFunc::Append(initFile, "__init__.pyi");
        
        AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeText | AZ::IO::OpenMode::ModeWrite;
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::Result result = AZ::IO::FileIOBase::GetInstance()->Open(initFile.c_str(), openMode, fileHandle);
        AZ_Warning("python", result, "Could not open %s to write Python symbols.", initFile.c_str());
        if (result)
        {
            return fileHandle;
        }

        return AZ::IO::InvalidHandle;
    }

    AZ::IO::HandleType PythonLogSymbolsComponent::OpenModuleAt(AZStd::string_view moduleName)
    {
        if (m_basePath.empty())
        {
            return AZ::IO::InvalidHandle;
        }

        bool resetFile = false;
        if (m_moduleSet.find(moduleName) == m_moduleSet.end())
        {
            m_moduleSet.insert(moduleName);
            resetFile = true;
        }

        AZStd::vector<AZStd::string> moduleParts;
        AzFramework::StringFunc::Tokenize(moduleName.data(), moduleParts, '.');

        // prepare target PYI file
        AZStd::string targetModule = moduleParts.back();
        moduleParts.pop_back();
        AzFramework::StringFunc::Append(targetModule, ".pyi");

        AZStd::string modulePath;
        AzFramework::StringFunc::Append(modulePath, m_basePath.c_str());
        AzFramework::StringFunc::Append(modulePath, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        AzFramework::StringFunc::Join(modulePath, moduleParts.begin(), moduleParts.end(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

        // prepare the path
        AZ::IO::FileIOBase::GetInstance()->CreatePath(modulePath.c_str());

        // assemble the file path
        AzFramework::StringFunc::Append(modulePath, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        AzFramework::StringFunc::Append(modulePath, targetModule.c_str());
        AzFramework::StringFunc::AssetDatabasePath::Normalize(modulePath);

        AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeText;
        if (AZ::IO::SystemFile::Exists(modulePath.c_str()))
        {
            openMode |= (resetFile) ? AZ::IO::OpenMode::ModeWrite : AZ::IO::OpenMode::ModeAppend;
        }
        else
        {
            openMode |= AZ::IO::OpenMode::ModeWrite;
        }

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::Result result = AZ::IO::FileIOBase::GetInstance()->Open(modulePath.c_str(), openMode, fileHandle);
        AZ_Warning("python", result, "Could not open %s to write Python module symbols.", modulePath.c_str());
        if (result)
        {
            return fileHandle;
        }

        return AZ::IO::InvalidHandle;
    }
}
