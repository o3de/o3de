/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "Method.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <Libraries/Core/MethodUtility.h>
#include <Core/SlotConfigurationDefaults.h>

namespace MethodCPP
{
    using namespace ScriptCanvas;

    enum eVersion
    {
        Initial = 0,
        Unnamed1,
        Unnamed2,
        PluralizeResults,
        AddedPrettyNameFieldToSerialization,
        // add your version above
        Current,
    };

    bool MethodVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElementNode)
    {
        if (rootElementNode.GetVersion() < MethodCPP::eVersion::PluralizeResults)
        {
            SlotId resultSlotId;
            if (!rootElementNode.GetChildData(AZ_CRC("resultSlotID", 0xb527ade6), resultSlotId))
            {
                AZ_Error("ScriptCanvas", false, "Failed to read resultSlotID from Method node data");
                return false;
            }

            rootElementNode.AddElementWithData(context, "resultSlotIDs", AZStd::vector<SlotId> { resultSlotId });
            rootElementNode.RemoveElementByName(AZ_CRC("resultSlotID", 0xb527ade6));
        }

        return true;
    }
}

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> IsExposable(const AZ::BehaviorMethod& method)
    {
        if (method.GetNumArguments() > BehaviorContextMethodHelper::MaxCount)
        {
            return AZ::Failure(AZStd::string("Too many arguments for a Script Canvas method"));
        }

        for (size_t argIndex(0), sentinel(method.GetNumArguments()); argIndex != sentinel; ++argIndex)
        {
            if (const AZ::BehaviorParameter* argument = method.GetArgument(argIndex))
            {
                const Data::Type type = AZ::BehaviorContextHelper::IsStringParameter(*argument) ? Data::Type::String() : Data::FromAZType(argument->m_typeId);

                if (!type.IsValid())
                {
                    return AZ::Failure(AZStd::string::format("Argument type at index: %zu is not valid in ScriptCanvas, TypeId: %s", argIndex, argument->m_typeId.ToString<AZStd::string>().data()));
                }

                if ((argument->m_traits & AZ::BehaviorParameter::TR_THIS_PTR) && Data::IsValueType(type))
                {
                    return AZ::Failure(AZStd::string::format("No member functions on value types, like, %s, are allowed in ScriptCanvas", Data::GetName(type).data()));
                }
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Missing argument at index: %zu", argIndex));
            }
        }

        return AZ::Success();
    }

    namespace Nodes
    {
        namespace Core
        {
            void Method::OnInputSignal(const SlotId&)
            {
                AZ_Error("Script Canvas", m_method, "Method node called with no initialized method!");

                if (m_method)
                {
                    AZStd::array<AZ::BehaviorValueParameter, BehaviorContextMethodHelper::MaxCount> params;
                    AZ::BehaviorValueParameter* paramFirst(params.begin());
                    AZ::BehaviorValueParameter* paramIter = paramFirst;

                    // all input should have been pushed into this node already
                    int argIndex(0);                    

                    Node::DatumVector inputDatums = GatherDatumsForDescriptor(SlotDescriptors::DataIn());

                    for (const Datum* datum : inputDatums)
                    {
                        auto behaviorParameter = m_method->GetArgument(argIndex);
                        AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> inputParameter = datum->ToBehaviorValueParameter(*behaviorParameter);

                        if (!inputParameter.IsSuccess())
                        {
                            if (behaviorParameter->m_traits & (AZ::BehaviorParameter::TR_REFERENCE | AZ::BehaviorParameter::TR_THIS_PTR))
                            {
                                SCRIPTCANVAS_REPORT_ERROR((*this), "BehaviorContext method input problem at parameter index %d: %s. Most likely there is a data pin missing a connection. Connect the missing data pin and try again.", argIndex, behaviorParameter->m_name);
                            }
                            else
                            {
                                SCRIPTCANVAS_REPORT_ERROR((*this), "BehaviorContext method input problem at parameter index %d: %s", argIndex, inputParameter.GetError().data());
                            }

                            return;
                        }

                        paramIter->Set(inputParameter.GetValue());
                        ++paramIter;
                        ++argIndex;
                    }

                    auto callResult = BehaviorContextMethodHelper::Call(*this, m_isOutcomeOutputMethod, m_method, paramFirst, paramIter, m_resultSlotIDs);
                    SignalOutput(GetSlotId(callResult.m_executionOutOverride));
                }
                else
                {
                    SignalOutput(GetSlotId("Out"));
                }
            }

            void Method::InitializeMethod(const MethodConfiguration& config)
            {
                m_namespaces = config.m_namespaces ? *config.m_namespaces : m_namespaces;
                m_className = config.m_className ? *config.m_className : m_className;
                m_classNamePretty = m_className;
                m_methodType = config.m_methodType;
                m_lookupName = config.m_lookupName ? *config.m_lookupName : config.m_method.m_name;

                auto isExposableOutcome = IsExposable(config.m_method);
                AZ_Warning("ScriptCanvas", isExposableOutcome.IsSuccess(), "BehaviorContext Method %s is no longer exposable to ScriptCanvas: %s", isExposableOutcome.GetError().data());
                ConfigureMethod(config.m_method);
                InitializeInput(config);
                AddSlot(CommonSlots::GeneralInSlot());
                InitializeOutput(config);
            }

            SlotId Method::AddMethodInputSlot(AZStd::string_view argName, AZStd::string_view toolTip, const AZ::BehaviorParameter& argument)
            {
                DataSlotConfiguration slotConfiguration;
                slotConfiguration.m_name = argName;
                slotConfiguration.m_toolTip = toolTip;
                slotConfiguration.SetConnectionType(ConnectionType::Input);

                // Create a slot with a default value
                if (argument.m_typeId == azrtti_typeid<AZ::EntityId>())
                {
                    slotConfiguration.ConfigureDatum(Datum(Data::Type::EntityID(), Datum::eOriginality::Copy, &ScriptCanvas::GraphOwnerId, AZ::Uuid::CreateNull()));
                }
                else
                {
                    Data::Type scType = !AZ::BehaviorContextHelper::IsStringParameter((argument)) ? Data::FromAZType(argument.m_typeId) : Data::Type::String();
                    slotConfiguration.ConfigureDatum(Datum(scType, Datum::eOriginality::Copy, nullptr, AZ::Uuid::CreateNull()));
                }

                return AddSlot(slotConfiguration);
            }

            AZStd::string Method::GetArgumentName(size_t argIndex, const AZ::BehaviorMethod& method, [[maybe_unused]] const AZ::BehaviorClass* bcClass, AZStd::string_view replaceTypeName)
            {
                if (const AZ::BehaviorParameter* argument = method.GetArgument(argIndex))
                {
                    const AZStd::string argumentTypeName = replaceTypeName.empty()
                        ? AZ::BehaviorContextHelper::IsStringParameter(*argument)
                            ? Data::GetName(Data::Type::String())
                            : Data::GetName(Data::FromAZType(argument->m_typeId))
                        : AZStd::string(replaceTypeName);

                    const AZStd::string* argumentNamePtr = method.GetArgumentName(argIndex);

                    return argumentNamePtr && !argumentNamePtr->empty()
                        ? *argumentNamePtr
                        : (AZStd::string::format("%s:%2zu", argumentTypeName.data(), argIndex));
                }

                return {};
            }

            void Method::InitializeInput(const MethodConfiguration& config)
            {
                for (size_t argIndex(0), sentinel(config.m_method.GetNumArguments()); argIndex != sentinel; ++argIndex)
                {
                    if (const AZ::BehaviorParameter* argument = config.m_method.GetArgument(argIndex))
                    {
                        const AZStd::string argName = GetArgumentName(argIndex, config.m_method, config.m_class, "");

                        const AZStd::string* argumentTooltipPtr = config.m_method.GetArgumentToolTip(argIndex);
                        const AZStd::string_view argumentTooltip = argumentTooltipPtr ? argumentTooltipPtr->c_str() : AZStd::string_view();

                        SlotId addedSlot = AddMethodInputSlot(argName, argumentTooltip, *argument);

                        if (addedSlot.IsValid())
                        {
                            if (const auto defaultValue = config.m_method.GetDefaultValue(argIndex))
                            {
                                ModifiableDatumView datumView;
                                FindModifiableDatumView(addedSlot, datumView);

                                if (datumView.IsValid() && Data::IsValueType(datumView.GetDataType()))
                                {
                                    datumView.AssignToDatum(AZStd::move(Datum(defaultValue->m_value)));
                                }
                            }
                        }
                    }
                }
            }

            void Method::InitializeOutput(const MethodConfiguration& config)
            {
                const AZ::BehaviorMethod& method = config.m_method;
                const AZ::BehaviorClass* bcClass = config.m_class;

                bool useOutcomeExecutionSlots = false;

                if (method.HasResult())
                {
                    if (const AZ::BehaviorParameter* result = method.GetResult())
                    {
                        if (m_isOutcomeOutputMethod)
                        {
                            useOutcomeExecutionSlots = true;

                            for (const auto& tupleIndexGetMethodPair : m_tupleGetMethods)
                            {
                                if (const AZ::BehaviorMethod* tupleGetMethod = tupleIndexGetMethodPair.second)
                                {
                                    if (const AZ::BehaviorParameter* tupleGetResult = tupleGetMethod->GetResult())
                                    {
                                        Data::Type outputType(AZ::BehaviorContextHelper::IsStringParameter(*tupleGetResult) ? Data::Type::String() : Data::FromAZType(tupleGetResult->m_typeId));
                                        AZStd::string_view outcomeName = tupleGetMethod->m_name;

                                        if (tupleGetMethod->m_name.find("Error") != AZStd::string::npos)
                                        {
                                            outcomeName = "Error";
                                        }
                                        else if (tupleGetMethod->m_name.find("Value") != AZStd::string::npos)
                                        {
                                            outcomeName = "Value";
                                        }

                                        const AZStd::string resultSlotName(AZStd::string::format("%s: %s", outcomeName.data(), Data::GetName(outputType).data()));
                                        {
                                            DataSlotConfiguration slotConfiguration;
                                            slotConfiguration.m_name = resultSlotName.c_str();
                                            slotConfiguration.SetType(outputType);
                                            slotConfiguration.SetConnectionType(ConnectionType::Output);
                                            m_resultSlotIDs.emplace_back(AddSlot(slotConfiguration));
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            Data::Type outputType(AZ::BehaviorContextHelper::IsStringParameter(*result) ? Data::Type::String() : Data::FromAZType(result->m_typeId));
                            const AZStd::string resultSlotName(AZStd::string::format("Result: %s", Data::GetName(outputType).data()));
                            {
                                DataSlotConfiguration slotConfiguration;
                                slotConfiguration.m_name = resultSlotName.c_str();
                                slotConfiguration.SetType(outputType);
                                slotConfiguration.SetConnectionType(ConnectionType::Output);
                                m_resultSlotIDs.emplace_back(AddSlot(slotConfiguration));
                            }
                        }
                    }
                }

                if (useOutcomeExecutionSlots)
                {
                    AZStd::string successName{ "Success" };
                    if (auto successOverridePtr = AZ::FindAttribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSuccessSlotName, method.m_attributes))
                    {
                        AZ::AttributeReader(nullptr, successOverridePtr).Read<AZStd::string>(successName);
                    }
                    
                    {
                        ExecutionSlotConfiguration slotConfiguration(successName, ConnectionType::Output);
                        AddSlot(slotConfiguration);
                    }

                    AZStd::string failureName{ "Failure" };
                    if (auto failureOverridePtr = AZ::FindAttribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, method.m_attributes))
                    {
                        AZ::AttributeReader(nullptr, failureOverridePtr).Read<AZStd::string>(failureName);
                    }
                    
                    {
                        ExecutionSlotConfiguration slotConfiguration(failureName, ConnectionType::Output);
                        AddSlot(slotConfiguration);
                    }
                }
                else
                {
                    AddSlot(CommonSlots::GeneralOutSlot());
                }

                if (m_resultSlotIDs.empty())
                {
                    m_resultSlotIDs.emplace_back(SlotId{});
                }
            }

            void Method::InitializeClassOrBus(const Namespaces& namespaces, const AZStd::string& className, const AZStd::string& methodName)
            {
                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "Can't create the ebus sender without a behavior context!");
                    return;
                }

                const auto& ebusIterator = behaviorContext->m_ebuses.find(className);
                if (ebusIterator == behaviorContext->m_ebuses.end())
                {
                    InitializeClass(namespaces, className, methodName);
                }
                else
                {
                    InitializeEvent(namespaces, className, methodName);
                }

                PopulateNodeType();
            }

            void Method::InitializeClass(const Namespaces& namespaces, const AZStd::string& className, const AZStd::string& methodName)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                const AZ::BehaviorMethod* method{};
                const AZ::BehaviorClass* bcClass{};
                if (FindClass(method, bcClass, namespaces, className, methodName))
                {
                    MethodConfiguration config(*method, MethodType::Member);
                    config.m_class = bcClass;
                    config.m_namespaces = &m_namespaces;
                    config.m_className = &className;
                    config.m_lookupName = &methodName;
                    InitializeMethod(config);
                }
            }

            void Method::InitializeEvent(const Namespaces& namespaces, const AZStd::string& ebusName, const AZStd::string& eventName)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                const AZ::BehaviorMethod* method{};
                if (FindEvent(method, namespaces, ebusName, eventName))
                {
                    MethodConfiguration config(*method, MethodType::Event);
                    config.m_namespaces = &m_namespaces;
                    config.m_className = &ebusName;
                    config.m_lookupName = &eventName;
                    InitializeMethod(config);
                }
            }

            void Method::InitializeFree(const Namespaces& namespaces, const AZStd::string& methodName)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                const AZ::BehaviorMethod* method{};
                if (FindFree(method, namespaces, methodName))
                {
                    MethodConfiguration config(*method, MethodType::Free);
                    config.m_namespaces = &m_namespaces;
                    config.m_lookupName = &methodName;
                    InitializeMethod(config);
                }
            }

            const AZ::BehaviorMethod* Method::GetMethod() const
            {
                return m_method;
            }

            void Method::SetMethodUnchecked(const AZ::BehaviorMethod* method)
            {
                m_method = method;
            }

            void Method::ConfigureMethod(const AZ::BehaviorMethod& method)
            {
                if (IsConfigured())
                {
                    return;
                }

                m_method = &method;
                m_isOutcomeOutputMethod = (AZ::FindAttribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, m_method->m_attributes) != nullptr);

                if (method.HasResult())
                {
                    if (const AZ::BehaviorParameter* result = method.GetResult())
                    {
                        m_tupleGetMethods = GetTupleGetMethods(result->m_typeId);
                    }
                }

                if (m_classNamePretty.empty())
                {
                    m_classNamePretty = m_className;
                }
            }

            bool Method::FindClass(const AZ::BehaviorMethod*& outMethod, const AZ::BehaviorClass*& outClass, [[maybe_unused]] const Namespaces& namespaces, AZStd::string_view className, AZStd::string_view methodName)
            {
                AZ::BehaviorContext* behaviorContext(nullptr);
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "A behavior context is required!");
                    return false;
                }

                auto classIter(behaviorContext->m_classes.find(className));
                if (classIter == behaviorContext->m_classes.end())
                {
                    AZ_Error("Script Canvas", false, "No class by name of %s in the behavior context!", className.data());
                    return false;
                }

                const AZ::BehaviorClass* behaviorClass(classIter->second);
                AZ_Assert(behaviorClass, "BehaviorContext Class entry %s has no class pointer", className.data());
                
                const auto methodIter(behaviorClass->m_methods.find(methodName.data()));
                if (methodIter == behaviorClass->m_methods.end())
                {
                    AZ_Error("Script Canvas", false, "No method by name of %s in BehaviorContext class %s", methodName.data(), className.data());
                    return false;
                }

                // this argument is the first argument...so perhaps remove the distinction between class and member functions, since it probably won't follow polymorphism
                // if it will, keep the distinction, and add the first argument separately
                AZ::BehaviorMethod* method(methodIter->second);
                if (!method)
                {
                    AZ_Error("Script Canvas", false, "BehaviorContext Method entry %s has no method pointer", methodName.data());
                    return false;
                }
                
                m_classNamePretty = className;

                if (AZ::Attribute* prettyNameAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::PrettyName, behaviorClass->m_attributes))
                {
                    AZ::AttributeReader(nullptr, prettyNameAttribute).Read<AZStd::string>(m_classNamePretty, *behaviorContext);
                }

                outClass = behaviorClass;
                outMethod = method;
                return true;
            }

            bool Method::FindEvent(const AZ::BehaviorMethod*& outMethod, [[maybe_unused]] const Namespaces& namespaces, AZStd::string_view ebusName, AZStd::string_view eventName)
            {
                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "Can't create the ebus sender without a behavior context!");
                    return false;
                }

                const auto& ebusIterator = behaviorContext->m_ebuses.find(ebusName);
                if (ebusIterator == behaviorContext->m_ebuses.end())
                {
                    AZ_Error("Script Canvas", false, "No ebus by name of %s in the behavior context!", ebusName.data());
                    return false;
                }

                AZ::BehaviorEBus* ebus = ebusIterator->second;
                AZ_Assert(ebus, "ebus == nullptr in %s", ebusName.data());

                auto sender = ebus->m_events.find(eventName);

                if (sender == ebus->m_events.end())
                {
                    sender = ebus->m_events.begin();

                    while (sender != ebus->m_events.end())
                    {
                        if (sender->second.m_deprecatedName == eventName)
                        {
                            m_lookupName = sender->first;
                            break;
                        }

                        ++sender;
                    }

                    if (sender == ebus->m_events.end())
                    {
                        AZ_Error("Script Canvas", false, "No event by name of %s found in the ebus %s", eventName.data(), ebusName.data());
                        return false;
                    }
                }

                AZ::EBusAddressPolicy addressPolicy
                    = ebus->m_idParam.m_typeId.IsNull()
                    ? AZ::EBusAddressPolicy::Single
                    : AZ::EBusAddressPolicy::ById;

                AZ::BehaviorMethod* method
                    = ebus->m_queueFunction
                    ? (addressPolicy == AZ::EBusAddressPolicy::ById ? sender->second.m_queueEvent : sender->second.m_queueBroadcast)
                    : (addressPolicy == AZ::EBusAddressPolicy::ById ? sender->second.m_event : sender->second.m_broadcast);

                if (!method)
                {
                    AZ_Error("Script Canvas", false, "Queue function mismatch in %s-%s", eventName.data(), ebusName.data());
                    return false;
                }

                outMethod = method;

                // If the bus name and the class name are different. Assumedly we
                // are using a deprecated name
                if (ebus->m_name.compare(m_className) != 0)
                {
                    if (m_className.compare(m_classNamePretty) == 0)
                    {
                        m_classNamePretty = ebus->m_name;
                    }

                    m_className = ebus->m_name;
                }

                return true;
            }

            bool Method::FindFree(const AZ::BehaviorMethod*& outMethod, [[maybe_unused]] const Namespaces& namespaces, AZStd::string_view methodName)
            {
                AZ::BehaviorContext* behaviorContext(nullptr);
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "A behavior context is required!");
                    return false;
                }

                const auto methodIter(behaviorContext->m_methods.find(methodName));
                if (methodIter == behaviorContext->m_methods.end())
                {
                    AZ_Error("Script Canvas", false, "No method by name of %s in the behavior context!", methodName.data());
                    return false;
                }

                AZ::BehaviorMethod* method(methodIter->second);
                if (!method)
                {
                    AZ_Error("Script Canvas", false, "BehaviorContext Method entry %s has no method pointer", methodName.data());
                    return false;
                }

                outMethod = method;
                return true;
            }

            bool Method::IsExpectingResult() const
            {
                AZ_Assert(!m_resultSlotIDs.empty(), "m_resultSlotIDs must never be empty");

                for (auto& slotID : m_resultSlotIDs)
                {
                    if (slotID.IsValid())
                    {
                        return true;
                    }
                }

                return false;
            }

            SlotId Method::GetBusSlotId() const
            {
                if (m_method && m_method->HasBusId())
                {
                    const int busIndex{ 0 };
                    const AZ::BehaviorParameter& busArgument = *m_method->GetArgument(busIndex);
                    const AZStd::string argumentTypeName = AZ::BehaviorContextHelper::IsStringParameter(busArgument) ? Data::GetName(Data::Type::String()) : Data::GetName(Data::FromAZType(busArgument.m_typeId));
                    const AZStd::string* argumentNamePtr = m_method->GetArgumentName(busIndex);
                    const AZStd::string argName = argumentNamePtr && !argumentNamePtr->empty()
                        ? *argumentNamePtr
                        : AZStd::string::format("%s:%2d", argumentTypeName.data(), busIndex);

                    return GetSlotId(argName);
                }

                return {};
            }

            void Method::OnWriteEnd()
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                const AZ::BehaviorMethod* method{};
                switch (m_methodType)
                {
                case MethodType::Event:
                {
                    if (FindEvent(method, m_namespaces, m_className, m_lookupName))
                    {
                        ConfigureMethod(*method);
                    }
                    break;
                }
                case MethodType::Free:
                {
                    if (FindFree(method, m_namespaces, m_lookupName))
                    {
                        ConfigureMethod(*method);
                    }
                    break;
                }
                case MethodType::Member:
                {
                    const AZ::BehaviorClass* bcClass{};
                    if (FindClass(method, bcClass, m_namespaces, m_className, m_lookupName))
                    {
                        ConfigureMethod(*method);
                    }
                    break;
                }
                default:
                {
                    AZ_Error("Script Canvas", false, "unsupported method type in method");
                    break;
                }
                }

                if (m_resultSlotIDs.empty())
                {
                    m_resultSlotIDs.emplace_back(SlotId{});
                }
            }

            void Method::Reflect(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<Method, Node>()
                        ->Version(MethodCPP::eVersion::Current, &MethodCPP::MethodVersionConverter)
                        ->EventHandler<SerializeContextEventHandlerDefault<Method>>()
                        ->Field("methodType", &Method::m_methodType)
                        ->Field("methodName", &Method::m_lookupName)
                        ->Field("className", &Method::m_className)
                        ->Field("namespaces", &Method::m_namespaces)
                        ->Field("resultSlotIDs", &Method::m_resultSlotIDs)
                        ->Field("prettyClassName", &Method::m_classNamePretty)
                        ;

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<Method>("Method", "Method")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                            ;
                    }
                }
            }

        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas
