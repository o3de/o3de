/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Method.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <Core/SlotConfigurationDefaults.h> 
#include <Core/SlotExecutionMap.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>
#include <AzCore/StringFunc/StringFunc.h>

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
        StoreInputSlotIdsToSupportNullCheck,
        // add your version above
        Current,
    };

    bool MethodVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElementNode)
    {
        if (rootElementNode.GetVersion() < MethodCPP::eVersion::PluralizeResults)
        {
            SlotId resultSlotId;
            if (!rootElementNode.GetChildData(AZ_CRC_CE("resultSlotID"), resultSlotId))
            {
                AZ_Error("ScriptCanvas", false, "Failed to read resultSlotID from Method node data");
                return false;
            }

            rootElementNode.AddElementWithData(context, "resultSlotIDs", AZStd::vector<SlotId> { resultSlotId });
            rootElementNode.RemoveElementByName(AZ_CRC_CE("resultSlotID"));
        }

        return true;
    }
}

namespace ScriptCanvas
{
    Grammar::FunctionPrototype ToSignature(const AZ::BehaviorMethod& method)
    {
        Grammar::FunctionPrototype signature;

        for (size_t argIndex(0), sentinel(method.GetNumArguments()); argIndex != sentinel; ++argIndex)
        {
            if (const AZ::BehaviorParameter* argument = method.GetArgument(argIndex))
            {
                AZStd::string argumentTypeName = AZ::BehaviorContextHelper::IsStringParameter(*argument) ? Data::GetName(Data::Type::String()) : Data::GetName(Data::FromAZType(argument->m_typeId));
                const AZStd::string* argumentNamePtr = method.GetArgumentName(argIndex);
                AZStd::string argName = argumentNamePtr && !argumentNamePtr->empty()
                    ? *argumentNamePtr
                    : (AZStd::string::format("%s:%2zu", argumentTypeName.data(), argIndex));

                signature.m_inputs.emplace_back(AZStd::make_shared<Grammar::Variable>(AZStd::move(Datum(*argument, Datum::eOriginality::Original, nullptr)), AZStd::move(argName), Grammar::TraitsFlags()));
            }
        }

        if (method.HasResult())
        {
            if (const AZ::BehaviorParameter* result = method.GetResult())
            {
                AZStd::vector<AZ::TypeId> unpackedTypes = BehaviorContextUtils::GetUnpackedTypes(result->m_typeId);

                for (auto& type : unpackedTypes)
                {
                    signature.m_outputs.push_back(AZStd::make_shared<Grammar::Variable>(AZStd::move(Datum(Data::FromAZType(type), Datum::eOriginality::Original))));
                }
            }
        }

        return signature;
    }

    AZ::Outcome<void, AZStd::string> IsExposable(const AZ::BehaviorMethod& method)
    {
        for (size_t argIndex(0), sentinel(method.GetNumArguments()); argIndex != sentinel; ++argIndex)
        {
            if (const AZ::BehaviorParameter* argument = method.GetArgument(argIndex))
            {
                const Data::Type type = AZ::BehaviorContextHelper::IsStringParameter(*argument) ? Data::Type::String() : Data::FromAZType(argument->m_typeId);

                if (!type.IsValid())
                {
                    return AZ::Failure(AZStd::string::format("Argument type at index: %zu is not valid in ScriptCanvas, TypeId: %s", argIndex, argument->m_typeId.ToString<AZStd::string>().data()));
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
            bool Method::CanAcceptNullInput([[maybe_unused]] const Slot& executionSlot, const Slot& inputSlot) const
            {
                if (m_method)
                {
                    auto candidateID = inputSlot.GetId();
                    auto slotIter = AZStd::find(m_inputSlots.begin(), m_inputSlots.end(), candidateID);

                    if (slotIter != m_inputSlots.end())
                    {
                        const size_t index = slotIter - m_inputSlots.begin();
                        if (index < m_method->GetNumArguments())
                        {
                            const auto* argument = m_method->GetArgument(index);
                            if (argument->m_traits & (AZ::BehaviorParameter::TR_REFERENCE | AZ::BehaviorParameter::TR_THIS_PTR))
                            {
                                // references and this pointers cannot accept null input
                                return false;
                            }

                            if (!(argument->m_traits & AZ::BehaviorParameter::TR_POINTER))
                            {
                                // values cannot accept null input
                                return false;
                            }
                        }
                    }
                }                    

                return true;
            }

            const AZ::BehaviorClass* Method::GetClass() const
            {
                return m_class;
            }

            AZ::Outcome<DependencyReport, void> Method::GetDependencies() const
            {
                if (!m_method)
                {
                    return AZ::Failure();
                }

                DependencyReport dependencyNames;
                for (size_t index(0), sentinel = m_method->GetNumArguments(); index < sentinel; ++index)
                {
                    const NamespacePath entry = { Data::GetName(Data::FromAZType(m_method->GetArgument(index)->m_typeId)) };
                    dependencyNames.nativeLibraries.insert(entry);
                }

                return AZ::Success(dependencyNames);
            }

            AZ::Outcome<Grammar::LexicalScope, void> Method::GetFunctionCallLexicalScope(const Slot* /*slot*/) const
            {
                if (m_method)
                {
                    Grammar::LexicalScope lexicalScope;

                    if (m_method->IsMember()
                        || AZ::FindAttribute(AZ::Script::Attributes::TreatAsMemberFunction, m_method->m_attributes))
                    {
                        lexicalScope.m_type = Grammar::LexicalScopeType::Variable;
                    }
                    else
                    {
                        lexicalScope.m_type = Grammar::LexicalScopeType::Namespace;
                        lexicalScope.m_namespaces.push_back(m_className);
                    }

                    return AZ::Success(lexicalScope);
                }

                return AZ::Failure();
            }

            AZ::Outcome<AZStd::string, void> Method::GetFunctionCallName(const Slot*) const
            {
                if (m_method)
                {
                    if (m_method->IsMember()
                        || AZ::FindAttribute(AZ::Script::Attributes::TreatAsMemberFunction, m_method->m_attributes))
                    {
                        auto name = BehaviorContextUtils::FindExposedMethodName(*m_method, m_class);
                        if (!name.empty())
                        {
                            return AZ::Success(name);
                        }
                    }
                }

                return AZ::Success(m_lookupName);
            }

            EventType Method::GetFunctionEventType(const Slot*) const
            {
                return m_eventType;
            }

            DynamicDataType Method::GetOverloadedOutputType(size_t) const
            {
                return DynamicDataType::Any;
            }

            PropertyStatus Method::GetPropertyStatus() const
            {
                switch (m_methodType)
                {
                case MethodType::Getter:
                    return PropertyStatus::Getter;

                case MethodType::Setter:
                    return PropertyStatus::Setter;

                default:
                    return PropertyStatus::None;
                }
            }

            void Method::InitializeMethod(const MethodConfiguration& config)
            {
                m_namespaces = config.m_namespaces ? *config.m_namespaces : m_namespaces;
                m_className = config.m_className ? AZStd::string(*config.m_className) : m_className;
                m_classNamePretty = !config.m_prettyClassName.empty() ? config.m_prettyClassName : m_className;
                m_lookupName = config.m_lookupName ? AZStd::string(*config.m_lookupName) : config.m_method.m_name;
                m_methodType = config.m_methodType;
                m_eventType = config.m_eventType;

                auto isExposableOutcome = IsExposable(config.m_method);
                AZ_Warning("ScriptCanvas", isExposableOutcome.IsSuccess(), "BehaviorContext Method %s is no longer exposable to ScriptCanvas: %s", isExposableOutcome.GetError().data());
                ConfigureMethod(config.m_method, config.m_class);
                InitializeInput(config);
                {
                    ExecutionSlotConfiguration slotConfiguration("In", ConnectionType::Input);
                    AddSlot(slotConfiguration);
                }
                InitializeOutput(config);
            }

            SlotId Method::AddMethodInputSlot(const MethodConfiguration& config, size_t argumentIndex) // AZStd::string_view argName, AZStd::string_view toolTip, const AZ::BehaviorParameter& argument)
            {
                return AddSlot(MethodHelper::ToInputSlotConfig(config, argumentIndex));
            }

            void Method::InitializeInput(const MethodConfiguration& config)
            {
                for (size_t argIndex(0), sentinel(config.m_method.GetNumArguments()); argIndex != sentinel; ++argIndex)
                {
                    SlotId addedSlot = AddMethodInputSlot(config, argIndex);

                    if (addedSlot.IsValid())
                    {
                        MethodHelper::SetSlotToDefaultValue(*this, addedSlot, config, argIndex);
                        m_inputSlots.push_back(addedSlot);
                    }
                    else
                    {
                        AZ_Warning("ScriptCanvas", false, "Failed to add method input slot to Method node: %s-%s", config.m_prettyClassName.c_str(), config.m_method.m_name.c_str());
                    }
                }
            }

            void Method::InitializeOutput(const MethodConfiguration& config)
            {
                MethodOutputConfig outputConfig(this, config);
                outputConfig.resultSlotIdsOut = &m_resultSlotIDs;

                AZStd::vector<SlotId> outputSlotIds;
                outputConfig.resultSlotIdsOut = &outputSlotIds;
                OnInitializeOutputPre(outputConfig);
                MethodHelper::AddMethodOutputSlot(outputConfig);
                OnInitializeOutputPost(outputConfig);
            }

            void Method::InitializeBehaviorMethod(const NamespacePath& namespaces, AZStd::string_view className, AZStd::string_view methodName, PropertyStatus propertyStatus)
            {
                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "Can't create the ebus sender without a behavior context!");
                    return;
                }

                if (!InitializeOverloaded(namespaces, className, methodName))
                {
                    if (className.empty())
                    {
                        InitializeFree(namespaces, methodName);
                    }
                    else if (auto ebusIterator = behaviorContext->m_ebuses.find(className); ebusIterator != behaviorContext->m_ebuses.end())
                    {
                        InitializeEvent(namespaces, className, methodName);
                    }
                    else
                    {
                        InitializeClass(namespaces, className, methodName, propertyStatus);
                    }
                }

                PopulateNodeType();
                m_warnOnMissingFunction = true;
            }

            bool Method::InitializeOverloaded([[maybe_unused]] const NamespacePath& namespaces, AZStd::string_view className, AZStd::string_view methodName)
            {
                const AZ::BehaviorMethod* method{};
                const AZ::BehaviorClass* bcClass{};
                AZStd::string prettyClassName;

                if (IsMethodOverloaded() && BehaviorContextUtils::FindExplicitOverload(method, bcClass, className, methodName, &prettyClassName))
                {
                    MethodConfiguration config(*method, MethodType::Member);
                    config.m_class = bcClass;
                    config.m_namespaces = &m_namespaces;
                    config.m_className = &className;
                    config.m_lookupName = &methodName;
                    config.m_prettyClassName = prettyClassName;
                    InitializeMethod(config);
                    return true;
                }
                else
                {
                    return false;
                }
            }

            void Method::InitializeClass(const NamespacePath&, AZStd::string_view className, AZStd::string_view methodName, PropertyStatus propertyStatus)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                const AZ::BehaviorMethod* method{};
                const AZ::BehaviorClass* bcClass{};
                AZStd::string prettyClassName;

                if (BehaviorContextUtils::FindClass(method, bcClass, className, methodName, propertyStatus, &prettyClassName))
                {
                    const auto methodType = propertyStatus == PropertyStatus::None ? MethodType::Member : propertyStatus == PropertyStatus::Getter ? MethodType::Getter : MethodType::Setter;

                    MethodConfiguration config(*method, methodType);
                    config.m_class = bcClass;
                    config.m_namespaces = &m_namespaces;
                    config.m_className = &className;
                    config.m_lookupName = &methodName;
                    config.m_prettyClassName = prettyClassName;
                    InitializeMethod(config);
                }
            }

            void Method::InitializeEvent(const NamespacePath&, AZStd::string_view ebusName, AZStd::string_view eventName)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                const AZ::BehaviorMethod* method{};
                EventType eventType;
                if (BehaviorContextUtils::FindEvent(method, ebusName, eventName, &eventType))
                {
                    MethodConfiguration config(*method, MethodType::Event);
                    config.m_namespaces = &m_namespaces;
                    config.m_className = &ebusName;
                    config.m_lookupName = &eventName;
                    config.m_eventType = eventType;
                    InitializeMethod(config);
                }
            }

            void Method::InitializeFree(const NamespacePath&, AZStd::string_view methodName)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                const AZ::BehaviorMethod* method{};

                if (BehaviorContextUtils::FindFree(method, methodName))
                {
                    MethodConfiguration config(*method, MethodType::Free);
                    config.m_namespaces = &m_namespaces;
                    config.m_lookupName = &methodName;
                    config.m_prettyClassName = methodName;
                    AZ::StringFunc::Replace(config.m_prettyClassName, "::Getter", "");
                    AZ::StringFunc::Replace(config.m_prettyClassName, "::Setter", "");
                    InitializeMethod(config);
                }
            }

            bool Method::GetBranchOnResultCheckName(AZStd::string& exposedName, Grammar::LexicalScope& lexicalScope) const
            {
                if (m_method)
                {
                    if (auto branchOnResultAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::BranchOnResult, m_method->m_attributes))
                    {
                        AZ::BranchOnResultInfo info;
                        if (AZ::AttributeReader(nullptr, branchOnResultAttribute).Read<AZ::BranchOnResultInfo>(info))
                        {
                            AZStd::pair<const AZ::BehaviorMethod*, const AZ::BehaviorClass*> check = BehaviorContextUtils::GetCheck(*m_method);
                            if (check.first && SanityCheckBranchOnResultMethod(*check.first))
                            {
                                if (check.first->IsMember()
                                    || AZ::FindAttribute(AZ::Script::Attributes::TreatAsMemberFunction, check.first->m_attributes))
                                {
                                    lexicalScope.m_type = Grammar::LexicalScopeType::Variable;
                                }
                                else
                                {
                                    lexicalScope.m_type = Grammar::LexicalScopeType::Namespace;
                                    if (check.second)
                                    {
                                        lexicalScope.m_namespaces.push_back(check.second->m_name);
                                    }
                                }

                                exposedName = BehaviorContextUtils::FindExposedMethodName(*check.first, check.second);
                                return true;
                            }
                        }
                        else
                        {
                            AZ_Error("ScriptCanvas", false, "failed to read BranchOnResult attribute for method %s", m_method->m_name.data());
                        }
                    }
                }

                return false;
            }

            bool Method::GetCheckedOperationInfo(AZ::CheckedOperationInfo& info, AZStd::string& exposedName, Grammar::LexicalScope& lexicalScope) const
            {
                if (m_method)
                {
                    if (auto checkOpAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::CheckedOperation, m_method->m_attributes))
                    {
                        if (AZ::AttributeReader(nullptr, checkOpAttribute).Read<AZ::CheckedOperationInfo>(info))
                        {
                            AZStd::pair<const AZ::BehaviorMethod*, const AZ::BehaviorClass*> check = BehaviorContextUtils::GetCheck(*m_method);
                            if (check.first)
                            {
                                if (check.first->IsMember()
                                    || AZ::FindAttribute(AZ::Script::Attributes::TreatAsMemberFunction, check.first->m_attributes))
                                {
                                    lexicalScope.m_type = Grammar::LexicalScopeType::Variable;
                                }
                                else
                                {
                                    lexicalScope.m_type = Grammar::LexicalScopeType::Namespace;
                                    if (check.second)
                                    {
                                        lexicalScope.m_namespaces.push_back(check.second->m_name);
                                    }
                                }

                                exposedName = BehaviorContextUtils::FindExposedMethodName(*check.first, check.second);
                                return true;
                            }
                            else
                            {
                                AZ_Error("ScriptCanvas", false, "method check by name of %s not found in behavior context", info.m_safetyCheckName.data())
                            }
                        }
                        else
                        {
                            AZ_Error("ScriptCanvas", false, "failed to read CheckedOperation attribute for method %s", m_method->m_name.data());
                        }
                    }
                }

                return false;
            }

            const Slot* Method::GetIfBranchSlot(bool branch) const
            {
                if (m_method)
                {
                    if (auto checkOpAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::CheckedOperation, m_method->m_attributes))
                    {
                        AZ::CheckedOperationInfo checkedOpInfo;

                        if (AZ::AttributeReader(nullptr, checkOpAttribute).Read<AZ::CheckedOperationInfo>(checkedOpInfo))
                        {
                            return GetSlotByName(branch ? checkedOpInfo.m_successCaseName : checkedOpInfo.m_failureCaseName);
                        }
                        else
                        {
                            AZ_Error("ScriptCanvas", false, "Failed to read check operation info");
                        }
                    }

                    if (auto branchOpAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::BranchOnResult, m_method->m_attributes))
                    {
                        AZ::BranchOnResultInfo branchOpInfo;

                        if (AZ::AttributeReader(nullptr, branchOpAttribute).Read<AZ::BranchOnResultInfo>(branchOpInfo))
                        {
                            return GetSlotByName(branch ? branchOpInfo.m_trueName : branchOpInfo.m_falseName);
                        }
                        else
                        {
                            AZ_Error("ScriptCanvas", false, "Failed to read branch on result info");
                        }
                    }
                }

                return nullptr;
            }

            const Slot* Method::GetIfBranchFalseOutSlot() const
            {
                return GetIfBranchSlot(false);
            }

            const Slot* Method::GetIfBranchTrueOutSlot() const
            {
                return GetIfBranchSlot(true);
            }

            const AZ::BehaviorMethod* Method::GetMethod() const
            {
                return m_method;
            }

            Data::Type Method::GetResultType() const
            {
                if (m_method && m_method->HasResult())
                {
                    return Data::FromAZType(m_method->GetResult()->m_typeId);
                }

                return Data::Type::Invalid();
            }

            ConstSlotsOutcome Method::GetSlotsInExecutionThreadByTypeImpl(const Slot&, CombinedSlotType targetSlotType, const Slot*) const
            {
                return AZ::Success(GetSlotsByType(targetSlotType));
            }

            AZ::Outcome<Grammar::FunctionPrototype> Method::GetSimpleSignature() const
            {
                if (auto method = GetMethod())
                {
                    return AZ::Success(ToSignature(*method));
                }
                else
                {
                    return AZ::Failure();
                }
            }

            bool Method::SanityCheckBranchOnResultMethod(const AZ::BehaviorMethod& branchOnResultMethod) const
            {
                if (m_method && m_method->HasResult())
                {
                    if (branchOnResultMethod.GetNumArguments() == 1
                    && branchOnResultMethod.HasResult()
                    && Data::FromAZType(branchOnResultMethod.GetResult()->m_typeId) == Data::Type::Boolean())
                    {
                        AZ::Uuid methodResultType = m_method->GetResult()->m_typeId;
                        AZ::Uuid branchOnResultMethodArgType = branchOnResultMethod.GetArgument(0)->m_typeId;

                        return methodResultType == branchOnResultMethodArgType;
                    }
                }

                return false;
            }

            void Method::SetMethodUnchecked(const AZ::BehaviorMethod* method, const AZ::BehaviorClass* behaviorClass)
            {
                m_method = method;
                m_class = behaviorClass;

                if (behaviorClass)
                {
                    m_className = behaviorClass->m_name;
                }
                else
                {
                    m_className.clear();
                }
            }

            size_t Method::GenerateFingerprint() const
            {
                return BehaviorContextUtils::GenerateFingerprintForMethod(GetMethodType(), GetName(), GetRawMethodClassName());
            }

            void Method::ConfigureMethod(const AZ::BehaviorMethod& method, const AZ::BehaviorClass* bcClass)
            {
                if (IsConfigured())
                {
                    return;
                }

                m_method = &method;
                m_class = bcClass;
                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

                if (bcClass && behaviorContext)
                {
                    if (auto prettyNameAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::PrettyName, bcClass->m_attributes))
                    {
                        AZ::AttributeReader operatorAttrReader(nullptr, prettyNameAttribute);
                        operatorAttrReader.Read<AZStd::string>(m_classNamePretty, *behaviorContext);
                    }
                }

                if (m_classNamePretty.empty())
                {
                    m_classNamePretty = m_className;
                }
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

            bool Method::GetBehaviorContextClassMethod(const AZ::BehaviorClass*& outClass, const AZ::BehaviorMethod*& outMethod, EventType& outType) const
            {
                if (m_lookupName.empty() && m_className.empty())
                {
                    return false;
                }

                AZStd::string prettyClassName;
                AZStd::string methodName = m_lookupName;

                const AZ::BehaviorClass* bcClass{};
                const AZ::BehaviorMethod* method{};

                if (BehaviorContextUtils::FindExplicitOverload(method, bcClass, m_className, methodName, &prettyClassName))
                {
                    outClass = bcClass;
                    outMethod = method;
                    outType = EventType::Count;
                    return true;
                }
                else
                {
                    switch (m_methodType)
                    {
                    case MethodType::Event:
                    {
                        EventType eventType;
                        if (BehaviorContextUtils::FindEvent(method, m_className, methodName, &eventType))
                        {
                            outClass = bcClass;
                            outMethod = method;
                            outType = eventType;
                            return true;
                        }
                        
                        AZ_Warning("Script Canvas"
                            , !m_warnOnMissingFunction
                            , "Could not find event: %s, in bus: %s, anywhere in BehaviorContext"
                            , methodName.c_str()
                            , m_className.c_str());
                        return false;
                    }
                    break;

                    case MethodType::Free:
                    {
                        if (BehaviorContextUtils::FindFree(method, methodName))
                        {
                            outClass = bcClass;
                            outMethod = method;
                            outType = EventType::Count;
                            return true;
                        }

                        AZ_Warning("Script Canvas"
                            , !m_warnOnMissingFunction
                            , "Could not find free method: %s anywhere in BehaviorContext"
                            , methodName.c_str());
                        return false;
                    }
                    break;

                    case MethodType::Member:
                    case MethodType::Getter:
                    case MethodType::Setter:
                    {
                        PropertyStatus status = m_methodType == MethodType::Getter ? PropertyStatus::Getter : m_methodType == MethodType::Setter ? PropertyStatus::Setter : PropertyStatus::None;

                        if (BehaviorContextUtils::FindClass(method, bcClass, m_className, methodName, status, nullptr, m_warnOnMissingFunction))
                        {
                            outClass = bcClass;
                            outMethod = method;
                            outType = EventType::Count;
                            return true;
                        }

                        AZ_Warning("Script Canvas"
                            , !m_warnOnMissingFunction
                            , "Could not find method or property: %s in class %s: , anywhere in BehaviorContext"
                            , methodName.c_str()
                            , m_className.c_str());
                        return false;
                    }
                    break;

                    default:    
                        break;
                    }
                }

                AZ_Warning("Script Canvas"
                    , !m_warnOnMissingFunction
                    , "Could not find overloaded method: %s, class or event name: %s, anywhere in BehaviorContext"
                    , methodName.c_str()
                    , m_className.c_str());

                return false;
            }

            AZStd::tuple<const AZ::BehaviorMethod*, MethodType, EventType, const AZ::BehaviorClass*> Method::LookupMethod() const
            {
                using TupleType = AZStd::tuple<const AZ::BehaviorMethod*, MethodType, EventType, const AZ::BehaviorClass*>;
                
                AZStd::string prettyClassName;

                const AZ::BehaviorClass* bcClass{};
                const AZ::BehaviorMethod* method{};
                EventType eventType;

                if (GetBehaviorContextClassMethod(bcClass, method, eventType))
                {
                    return TupleType{ method, m_methodType, eventType, bcClass };
                }

                return TupleType{ nullptr, MethodType::Count, EventType::Count, nullptr };
            }

            void Method::OnDeserialize()
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                if (!m_lookupName.empty() || !m_className.empty())
                {
                    m_warnOnMissingFunction = true;
                    const AZ::BehaviorClass* bcClass{};
                    const AZ::BehaviorMethod* method{};
                    EventType eventType;

                    if (GetBehaviorContextClassMethod(bcClass, method, eventType))
                    {
                        m_eventType = eventType;
                        ConfigureMethod(*method, bcClass);
                    }
                    else
                    {
                        if (!m_method)
                        {
                            AZ_Warning("ScriptCanvas", !m_warnOnMissingFunction, "method node failed to deserialize properly");
                        }
                    }

                    if (m_resultSlotIDs.empty())
                    {
                        m_resultSlotIDs.emplace_back(SlotId{});
                    }
                }

                Node::OnDeserialize();
            }

#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
            void Method::OnWriteEnd()
            {
                if (m_lookupName.empty() && m_className.empty())
                {
                    return;
                }

                OnDeserialize();
            }
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)

            bool Method::BranchesOnResult() const
            {
                return m_method && m_method->HasResult() &&
                    AZ::FindAttribute(AZ::ScriptCanvasAttributes::BranchOnResult, m_method->m_attributes) != nullptr;
            }

            bool Method::IsCheckedOperation(bool* callCheckedOpOnBothBothBranches) const
            {
                if (m_method)
                {
                    if (auto attribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::CheckedOperation, m_method->m_attributes))
                    {
                        if (callCheckedOpOnBothBothBranches)
                        {
                            AZ::CheckedOperationInfo checkedOpInfo;
                            AZ_VerifyError("ScriptCanvas", AZ::AttributeReader(nullptr, attribute).Read<AZ::CheckedOperationInfo>(checkedOpInfo), "Failed to read check operation info in Method::IsCheckedOperation");
                            *callCheckedOpOnBothBothBranches = checkedOpInfo.m_callCheckedFunctionInBothCases;
                        }

                        return true;
                    }
                }

                return false;
            }

            bool Method::IsDeprecated() const
            {
                bool isDeprecated = false;
                if (m_method)
                {
                    if (auto isDeprecatedAttributePtr = AZ::FindAttribute(AZ::Script::Attributes::Deprecated, m_method->m_attributes))
                    {
                        AZ::AttributeReader(nullptr, isDeprecatedAttributePtr).Read<bool>(isDeprecated);
                    }
                }

                return isDeprecated;
            }

            bool Method::IsIfBranch() const
            {
                return IsCheckedOperation() || BranchesOnResult();
            }

            bool Method::IsIfBranchPrefacedWithBooleanExpression() const
            {
                return IsIfBranch();
            }

            bool Method::IsOutOfDate(const VersionData& graphVersion) const
            {
                AZ_UNUSED(graphVersion);

                if (!m_method)
                {
                    return true;
                }

                auto nodeSignatureOutcome = GetSimpleSignature();
                if (!nodeSignatureOutcome.IsSuccess())
                {
                    return true;
                }

                auto methodSignature = ToSignature(*m_method);
                const auto& nodeSignature = nodeSignatureOutcome.GetValue();
                return !(nodeSignature == methodSignature);
            }

            void Method::Reflect(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<Method, Node>()
                        ->Version(MethodCPP::eVersion::Current, &MethodCPP::MethodVersionConverter)
#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
                        ->EventHandler<SerializeContextOnWriteEndHandler<Method>>()
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)
                        ->Field("methodType", &Method::m_methodType)
                        ->Field("methodName", &Method::m_lookupName)
                        ->Field("className", &Method::m_className)
                        ->Field("namespaces", &Method::m_namespaces)
                        ->Field("resultSlotIDs", &Method::m_resultSlotIDs)
                        ->Field("inputSlots", &Method::m_inputSlots)
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

        }
    }
}
