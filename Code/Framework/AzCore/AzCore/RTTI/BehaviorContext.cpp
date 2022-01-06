/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Component/EntityBus.h>

namespace AZ
{
    bool MethodReturnsAzEventByReferenceOrPointer(const AZ::BehaviorMethod& method)
    {
        const AZ::BehaviorParameter* resultParameter = method.GetResult();
        if (resultParameter == nullptr)
        {
            return false;
        }

        // The return parameter must have AZ Rtti to in order for it be an AZ::Event parameter
        AZ::IRttiHelper* rttiHelper = resultParameter->m_azRtti;
        if (!rttiHelper || rttiHelper->GetGenericTypeId() != azrtti_typeid<AZ::Event>())
        {
            return false;
        }

        constexpr auto PointerValueTrait = AZ::BehaviorParameter::Traits::TR_REFERENCE | AZ::BehaviorParameter::Traits::TR_POINTER;
        return (resultParameter->m_traits & PointerValueTrait) != AZ::BehaviorParameter::Traits::TR_NONE;
    }

    bool ValidateAzEventDescription(const BehaviorContext& context, const AZ::BehaviorMethod& method)
    {
        const AZ::BehaviorParameter* resultParameter = method.GetResult();
        if (resultParameter == nullptr)
        {
            return false;
        }

        // The return parameter must have AZ Rtti to in order for it be an AZ::Event& or AZ::Event* parameter
        AZ::IRttiHelper* rttiHelper = resultParameter->m_azRtti;
        if (!rttiHelper || rttiHelper->GetGenericTypeId() != azrtti_typeid<AZ::Event>())
        {
            return false;
        }
        constexpr auto PointerValueTrait = AZ::BehaviorParameter::Traits::TR_REFERENCE | AZ::BehaviorParameter::Traits::TR_POINTER;
        const auto parameterTraits = static_cast<AZ::BehaviorParameter::Traits>(resultParameter->m_traits) & PointerValueTrait;
        if (parameterTraits == AZ::BehaviorParameter::Traits::TR_NONE)
        {
            return false;
        }

        bool azEventDescValid = true;
        AZ::Attribute* azEventDescAttribute = AZ::FindAttribute(AZ::Script::Attributes::AzEventDescription, method.m_attributes);
        AZ::AttributeReader azEventDescAttributeReader(nullptr, azEventDescAttribute);
        AZ::BehaviorAzEventDescription behaviorAzEventDesc;
        if (!azEventDescAttributeReader.Read<decltype(behaviorAzEventDesc)>(behaviorAzEventDesc))
        {
            AZ_Error("BehaviorContext", false, "Unable to read AzEventDescription attribute of method %s"
                " that returns an AZ::Event", method.m_name.c_str());
            return false;
        }

        if (behaviorAzEventDesc.m_eventName.empty())
        {
            AZ_Error("BehaviorContext", false, "AzEventDescription attribute on method %s"
                " has an empty event name", method.m_name.c_str());
            azEventDescValid = false;
        }

        auto azEventClassIter = context.m_typeToClassMap.find(rttiHelper->GetTypeId());
        if (azEventClassIter != context.m_typeToClassMap.end() && azEventClassIter->second != nullptr)
        {
            AZ::BehaviorClass* azEventClass = azEventClassIter->second;
            AZ::Attribute* eventParameterTypesAttribute = AZ::FindAttribute(AZ::Script::Attributes::EventParameterTypes,
                azEventClass->m_attributes);
            AZStd::vector<AZ::BehaviorParameter> eventParameterTypes;
            if (AZ::AttributeReader(nullptr, eventParameterTypesAttribute).Read<decltype(eventParameterTypes)>(eventParameterTypes))
            {
                if (eventParameterTypes.size() != behaviorAzEventDesc.m_parameterNames.size())
                {
                    AZ_Error("BehaviorContext", false, "AzEventDescription only contains names for %zu parameters,"
                        " while the AZ::Event(%s) accepts %zu parameters", behaviorAzEventDesc.m_parameterNames.size(),
                        behaviorAzEventDesc.m_eventName.c_str(), eventParameterTypes.size());
                    azEventDescValid = false;
                }

                size_t parameterIndex = 0;
                for (AZStd::string_view parameterName : behaviorAzEventDesc.m_parameterNames)
                {
                    if (parameterName.empty())
                    {
                        AZ_Error("BehaviorContext", false, "AzEventDescription parameter %zu contains an empty name parameter"
                            " for AZ::Event(%s)", parameterIndex, behaviorAzEventDesc.m_eventName.c_str());
                        azEventDescValid = false;
                    }
                    ++parameterIndex;
                }
            }
        }

        return azEventDescValid;
    }

    //=========================================================================
    // BehaviorMethod
    //=========================================================================
    BehaviorMethod::BehaviorMethod(BehaviorContext* context)
        : OnDemandReflectionOwner(*context)
        , m_debugDescription(nullptr)
    {}

    //=========================================================================
    // ~BehaviorMethod
    //=========================================================================
    BehaviorMethod::~BehaviorMethod()
    {
        for (auto attrIt : m_attributes)
        {
            delete attrIt.second;
        }

        if (m_overload)
        {
            delete m_overload;
        }

        m_attributes.clear();
    }

    //=========================================================================
    // BehaviorProperty
    //=========================================================================
    BehaviorProperty::BehaviorProperty(BehaviorContext* context)
        : OnDemandReflectionOwner(*context)
        , m_getter(nullptr)
        , m_setter(nullptr)
    {
    }

    //=========================================================================
    // ~BehaviorProperty
    //=========================================================================
    BehaviorProperty::~BehaviorProperty()
    {
        delete m_getter;
        delete m_setter;

        for (auto attrIt : m_attributes)
        {
            delete attrIt.second;
        }
    }

    bool BehaviorMethod::AddOverload(BehaviorMethod* overload)
    {
        // \todo add attribute to all overloads
        // \todo verify that we check class member functions vs free

        if (HasResult() != overload->HasResult())
        {
            AZ_Error("Reflection", false, "Overload failure, all methods must have the same result, or none at all: %s", m_name.c_str());
            return false;
        }

        if (HasResult())
        {
            auto methodResult = GetResult();
            auto overloadResult = overload->GetResult();

            if (!(methodResult->m_typeId == overloadResult->m_typeId && methodResult->m_traits == overloadResult->m_traits))
            {
                AZ_Error("Reflection", false, "Overload failure, all methods must have the same result, or none at all: %s", m_name.c_str());
                return false;
            }
        }

        if (GetNumArguments() == overload->GetNumArguments())
        {
            bool anyDifference = false;

            for (size_t i(0), sentinel(GetNumArguments()); !anyDifference && i < sentinel; ++i)
            {
                const BehaviorParameter* thisArg = GetArgument(i);
                const BehaviorParameter* overloadArg = overload->GetArgument(i);
                anyDifference = !(thisArg->m_typeId == overloadArg->m_typeId && thisArg->m_traits == overloadArg->m_traits);
            }

            if (!anyDifference)
            {
                AZ_Error("Reflection", false, "Overload failure, all methods must report different parameters");
                return false;
            }
        }

        if (m_overload)
        {
            return m_overload->AddOverload(overload);
        }
        else
        {
            m_overload = overload;
            return true;
        }
    }

    bool BehaviorMethod::IsAnOverload(BehaviorMethod* candidate) const
    {
        if (candidate)
        {
            auto iter = this;
            while (iter)
            {
                if (iter->m_overload == candidate)
                {
                    return true;
                }

                iter = iter->m_overload;
            }
        }

        return false;
    }

    //=========================================================================
    // GetTypeId
    //=========================================================================
    const AZ::Uuid& BehaviorProperty::GetTypeId() const
    {
        if (m_getter)
        {
            // if we have result, we validate on reflection that we have a result, so no need to check.
            return m_getter->GetResult()->m_typeId;
        }
        else
        {
            // if the property is Write only we should have a setter and the setter last argument is the property type
            return m_setter->GetArgument(m_setter->GetNumArguments() - 1)->m_typeId;
        }
    }

    ScopedBehaviorOnDemandReflector::ScopedBehaviorOnDemandReflector(BehaviorContext& behaviorContext)
        : OnDemandReflectionOwner(behaviorContext)
    {
    }

    //=========================================================================
    // BehaviorEBus
    //=========================================================================
    BehaviorEBus::BehaviorEBus()
        : m_createHandler(nullptr)
        , m_destroyHandler(nullptr)
        , m_queueFunction(nullptr)
        , m_getCurrentId(nullptr)
    {
        m_idParam.m_name = "BusIdType";
        m_idParam.m_typeId = AZ::Uuid::CreateNull();
        m_idParam.m_traits = BehaviorParameter::TR_REFERENCE;
        m_idParam.m_azRtti = nullptr;
    }

    //=========================================================================
    // ~BehaviorEBus
    //=========================================================================
    BehaviorEBus::~BehaviorEBus()
    {
        // Clear all lists of things now to prevent double deleting
        // (if they're found in the lists later, they'll be deleted again)
        auto events = AZStd::move(m_events);
        auto attributes = AZStd::move(m_attributes);

        // Actually delete everything
        for (const auto &propertyIt : events)
        {
            delete propertyIt.second.m_broadcast;
            delete propertyIt.second.m_event;
            delete propertyIt.second.m_queueBroadcast;
            delete propertyIt.second.m_queueEvent;
            for (auto attrIt : propertyIt.second.m_attributes)
            {
                delete attrIt.second;
            }
        }

        for (auto attrIt : attributes)
        {
            delete attrIt.second;
        }

        delete m_createHandler;
        delete m_destroyHandler;

        delete m_queueFunction;
        delete m_getCurrentId;
    }

    //=========================================================================
    // BehaviorContext::GlobalMethodBuilder
    //=========================================================================
    BehaviorContext::GlobalMethodBuilder::GlobalMethodBuilder(BehaviorContext* context, const char* methodName, BehaviorMethod* method)
        : Base::GenericAttributes(context)
        , m_name(methodName)
        , m_method(method)
    {
        if (method)
        {
            m_currentAttributes = &method->m_attributes;
        }
    }

    //=========================================================================
    // BehaviorContext::~GlobalMethodBuilder
    //=========================================================================
    BehaviorContext::GlobalMethodBuilder::~GlobalMethodBuilder()
    {
        // process all on demand queued reflections
        m_context->ExecuteQueuedOnDemandReflections();

        if (m_method)
        {
            if (MethodReturnsAzEventByReferenceOrPointer(*m_method))
            {
                ValidateAzEventDescription(*Base::m_context, *m_method);
            }
            BehaviorContextBus::Event(m_context, &BehaviorContextBus::Events::OnAddGlobalMethod, m_name, m_method);
        }
    }

    //=========================================================================
    // BehaviorContext::GlobalMethodBuilder::operator->
    //=========================================================================
    BehaviorContext::GlobalMethodBuilder* BehaviorContext::GlobalMethodBuilder::operator->()
    {
        return this;
    }

    //=========================================================================
    // BehaviorContext::GlobalPropertyBuilder
    //=========================================================================
    BehaviorContext::GlobalPropertyBuilder::GlobalPropertyBuilder(BehaviorContext* context, BehaviorProperty* prop)
        : Base::GenericAttributes(context)
        , m_prop(prop)
    {
        if (prop)
        {
            m_currentAttributes = &prop->m_attributes;
        }
    }

    //=========================================================================
    // BehaviorContext::~PropertyInfo
    //=========================================================================
    BehaviorContext::GlobalPropertyBuilder::~GlobalPropertyBuilder()
    {
        // process all on demand queued reflections
        m_context->ExecuteQueuedOnDemandReflections();

        if (m_prop)
        {
            // Only the property getter needs to be validated to determine if it returns an AZ::Event
            // and have the AzEventDescription attribute attached to that event
            if (m_prop->m_getter && MethodReturnsAzEventByReferenceOrPointer(*m_prop->m_getter))
            {
                ValidateAzEventDescription(*Base::m_context, *m_prop->m_getter);
            }
            BehaviorContextBus::Event(m_context, &BehaviorContextBus::Events::OnAddGlobalProperty, m_prop->m_name.c_str(), m_prop);
        }
    }

    //=========================================================================
    // BehaviorContext::GlobalPropertyBuilder::operator->
    //=========================================================================
    BehaviorContext::GlobalPropertyBuilder* BehaviorContext::GlobalPropertyBuilder::operator->()
    {
        return this;
    }

    //=========================================================================
    // BehaviorContext
    //=========================================================================
    BehaviorContext::BehaviorContext() = default;

    //=========================================================================
    // ~BehaviorContext
    //=========================================================================
    BehaviorContext::~BehaviorContext()
    {
        // Clear all lists of things now to prevent double deleting
        // (if they're found in the lists later, they'll be deleted again)
        auto methods = AZStd::move(m_methods);
        auto properties = AZStd::move(m_properties);
        auto classes = AZStd::move(m_classes);
        auto ebuses = AZStd::move(m_ebuses);

        m_typeToClassMap.clear();

        // Actually delete everything
        for (auto methodIt : methods)
        {
            delete methodIt.second;
        }

        for (auto propertyIt : properties)
        {
            delete propertyIt.second;
        }

        for (auto classIt : classes)
        {
            delete classIt.second;
        }

        for (auto ebusIt : ebuses)
        {
            delete ebusIt.second;
        }
    }

    bool BehaviorContext::IsTypeReflected(AZ::Uuid typeId) const
    {
        auto classTypeIt = m_typeToClassMap.find(typeId);
        return (classTypeIt != m_typeToClassMap.end());
    }

    //=========================================================================
    // BehaviorClass
    //=========================================================================
    BehaviorClass::BehaviorClass()
        : m_allocate(nullptr)
        , m_deallocate(nullptr)
        , m_defaultConstructor(nullptr)
        , m_destructor(nullptr)
        , m_cloner(nullptr)
        , m_equalityComparer(nullptr)
        , m_userData(nullptr)
        , m_typeId(Uuid::CreateNull())
        , m_alignment(0)
        , m_size(0)
        , m_unwrapper(nullptr)
        , m_unwrapperUserData(nullptr)
        , m_wrappedTypeId(Uuid::CreateNull())
    {
    }

    //=========================================================================
    // ~BehaviorClass
    //=========================================================================
    BehaviorClass::~BehaviorClass()
    {
        // Clear all lists of things now to prevent double deleting
        // (if they're found in the lists later, they'll be deleted again)
        auto constructors = AZStd::move(m_constructors);
        auto methods = AZStd::move(m_methods);
        auto properties = AZStd::move(m_properties);
        auto attributes = AZStd::move(m_attributes);

        // Actually delete everything
        for (BehaviorMethod* constructor : constructors)
        {
            delete constructor;
        }

        for (auto methodIt : methods)
        {
            delete methodIt.second;
        }

        for (auto propertyIt : properties)
        {
            delete propertyIt.second;
        }

        for (auto attrIt : attributes)
        {
            delete attrIt.second;
        }
    }

    //=========================================================================
    // Create
    //=========================================================================
    BehaviorObject BehaviorClass::Create() const
    {
        return Create(Allocate());
    }

    BehaviorObject BehaviorClass::Create(void* address) const
    {
        if (m_defaultConstructor && address)
        {
            m_defaultConstructor(address, m_userData);
        }

        return BehaviorObject(address, m_azRtti);
    }

    //=========================================================================
    // Clone
    //=========================================================================
    BehaviorObject BehaviorClass::Clone(const BehaviorObject& object) const
    {
        BehaviorObject result;
        if (m_cloner && object.m_typeId == m_typeId)
        {
            result.m_address = Allocate();
            if (result.m_address)
            {
                m_cloner(result.m_address, object.m_address, m_userData);
            }
            result.m_typeId = m_typeId;
            result.m_rttiHelper = m_azRtti;
        }
        return result;
    }

    AZStd::vector<BehaviorMethod*> BehaviorClass::GetOverloads(const AZStd::string& name) const
    {
        AZStd::vector<BehaviorMethod*> overloads;

        auto methodIter = m_methods.find(name);
        if (methodIter != m_methods.end())
        {
            overloads = GetOverloadsIncludeMethod(methodIter->second);
        }

        return overloads;
    }

    AZStd::vector<BehaviorMethod*> BehaviorClass::GetOverloadsIncludeMethod(BehaviorMethod* method) const
    {
        AZStd::vector<BehaviorMethod*> overloads;

        auto iter = method;
        while (iter)
        {
            overloads.push_back(iter);
            iter = iter->m_overload;
        }

        return overloads;
    }

    AZStd::vector<BehaviorMethod*> BehaviorClass::GetOverloadsExcludeMethod(BehaviorMethod* method) const
    {
        AZStd::vector<BehaviorMethod*> overloads;

        auto iter = method->m_overload;
        while (iter)
        {
            overloads.push_back(iter);
            iter = iter->m_overload;
        }

        return overloads;
    }

    void BehaviorClass::PostProcessMethod(BehaviorContext* context, BehaviorMethod& method)
    {
        if (Attribute* attribute = AZ::FindAttribute(ScriptCanvasAttributes::CheckedOperation, method.m_attributes))
        {
            CheckedOperationInfo checkedOperationInfo;
            if (AttributeReader(nullptr, attribute).Read<CheckedOperationInfo>(checkedOperationInfo))
            {
                auto iter = m_methods.find(checkedOperationInfo.m_safetyCheckName);
                if (iter != m_methods.end())
                {
                    context->m_checksByOperations.insert(AZStd::make_pair(&method, AZStd::make_pair(iter->second, this)));
                }
                else
                {
                    AZ_Error("BehaviorContext", false, "Method: %s, declared safety check: %s, but it was not found in class: %s", method.m_name.c_str(), m_name.c_str(), checkedOperationInfo.m_safetyCheckName.c_str());
                }
            }
        }

        if (Attribute* attribute = AZ::FindAttribute(ScriptCanvasAttributes::BranchOnResult, method.m_attributes))
        {
            BranchOnResultInfo branchOnResultInfo;
            if (AttributeReader(nullptr, attribute).Read<BranchOnResultInfo>(branchOnResultInfo))
            {
                if (!branchOnResultInfo.m_nonBooleanResultCheckName.empty())
                {
                    auto iter = m_methods.find(branchOnResultInfo.m_nonBooleanResultCheckName);
                    if (iter != m_methods.end())
                    {
                        context->m_checksByOperations.insert(AZStd::make_pair(&method, AZStd::make_pair(iter->second, this)));
                    }
                    else
                    {
                        AZ_Error("BehaviorContext", false, "safety check declared for method %s but it was not found in the class");
                    }
                }
            }
        }

        if (Attribute* attribute = AZ::FindAttribute(ScriptCanvasAttributes::ExplicitOverloadCrc, method.m_attributes))
        {
            AZ::ExplicitOverloadInfo explicitOverloadInfo;

            if (AZ::AttributeReader(nullptr, attribute).Read<AZ::ExplicitOverloadInfo>(explicitOverloadInfo))
            {
                auto overloadIter = context->m_explicitOverloads.find(explicitOverloadInfo);

                if (overloadIter == context->m_explicitOverloads.end())
                {
                    explicitOverloadInfo.m_overloads.push_back({ &method, this });
                    context->m_explicitOverloads.insert(AZStd::move(explicitOverloadInfo));
                }
                else
                {
                    overloadIter->m_overloads.push_back({ &method, this });
                }
            }
        }
    }

    const BehaviorMethod* BehaviorClass::FindMethodByReflectedName(const AZStd::string& reflectedName) const
    {
        auto methodIter = m_methods.find(reflectedName);
        return methodIter != m_methods.end() ? methodIter->second : nullptr;
    }

    bool BehaviorClass::IsMethodOverloaded(const AZStd::string& name) const
    {
        auto methodIter = m_methods.find(name);
        return methodIter != m_methods.end() && methodIter->second->m_overload != nullptr;
    }

    //=========================================================================
    // Move
    //=========================================================================
    BehaviorObject BehaviorClass::Move(BehaviorObject&& object) const
    {
        BehaviorObject result;
        if (m_mover && object.m_typeId == m_typeId)
        {
            result.m_address = Allocate();
            if (result.m_address)
            {
                m_mover(result.m_address, object.m_address, m_userData);
                Destroy(object);
            }
            result.m_typeId = m_typeId;
            result.m_rttiHelper = m_azRtti;
        }
        return result;
    }

    //=========================================================================
    // Destroy
    //=========================================================================
    void BehaviorClass::Destroy(const BehaviorObject& object) const
    {
        if (object.m_typeId == m_typeId && m_destructor && object.m_address)
        {
            m_destructor(object.m_address, m_userData);
            Deallocate(object.m_address);
        }
    }

    //=========================================================================
    // Allocate
    //=========================================================================
    void* BehaviorClass::Allocate() const
    {
        if (m_allocate)
        {
            return m_allocate(m_userData);
        }
        else
        {
            return azmalloc(m_size, m_alignment, AZ::SystemAllocator, m_name.c_str());
        }
    }

    //=========================================================================
    // Deallocate
    //=========================================================================
    void  BehaviorClass::Deallocate(void* address) const
    {
        if (address == nullptr)
        {
            return;
        }

        if (m_deallocate)
        {
            m_deallocate(address, m_userData);
        }
        else
        {
            azfree(address, AZ::SystemAllocator, m_size, m_alignment);
        }
    }

    //=========================================================================
    // FindAttribute
    //=========================================================================
    AZ::Attribute* BehaviorClass::FindAttribute(const AttributeId& attributeId) const
    {
        return AZ::FindAttribute(attributeId, m_attributes);
    }

    //=========================================================================
    // HasAttribute
    //=========================================================================
    bool BehaviorClass::HasAttribute(const AttributeId& attributeId) const
    {
        return FindAttribute(attributeId) != nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //=========================================================================
    // InstallGenericHook
    //=========================================================================
    bool BehaviorEBusHandler::InstallGenericHook(int index, GenericHookType hook, void* userData)
    {
        if (index != -1)
        {
            // Check parameters
            m_events[index].m_isFunctionGeneric = true;
            m_events[index].m_function = reinterpret_cast<void*>(hook);
            m_events[index].m_userData = userData;
            return true;
        }

        return false;
    }

    //=========================================================================
    // InstallGenericHook
    //=========================================================================
    bool BehaviorEBusHandler::InstallGenericHook(const char* name, GenericHookType hook, void* userData)
    {
        return InstallGenericHook(GetFunctionIndex(name), hook, userData);
    }

    //=========================================================================
    // GetEvents
    //=========================================================================
    const BehaviorEBusHandler::EventArray& BehaviorEBusHandler::GetEvents() const
    {
        return m_events;
    }

    bool BehaviorEBusHandler::BusForwarderEvent::HasResult() const
    {
        return !m_parameters.empty() && !m_parameters.front().m_typeId.IsNull() && m_parameters.front().m_typeId != azrtti_typeid<void>();
    }

    CheckedOperationInfo::CheckedOperationInfo
        ( AZStd::string_view safetyCheckName
        , const InputRestriction& inputRestriction
        , AZStd::string_view successName
        , AZStd::string_view failureName
        , bool callCheckedFunctionInBothCases
        )
        : m_safetyCheckName(safetyCheckName)
        , m_inputRestriction(inputRestriction)
        , m_successCaseName(successName)
        , m_failureCaseName(failureName)
        , m_callCheckedFunctionInBothCases(callCheckedFunctionInBothCases)
    {}

    bool CheckedOperationInfo::operator==(const CheckedOperationInfo& other) const
    {
        return m_safetyCheckName == other.m_safetyCheckName;
    }

    bool CheckedOperationInfo::operator!=(const CheckedOperationInfo& other) const
    {
        return m_safetyCheckName != other.m_safetyCheckName;
    }

    OverloadArgumentGroupInfo::OverloadArgumentGroupInfo(const AZStd::vector<AZStd::string>&& parameterGroupNames, const AZStd::vector<AZStd::string>&& resultGroupNames)
        : m_parameterGroupNames(parameterGroupNames), m_resultGroupNames(resultGroupNames)
    {}

    ExplicitOverloadInfo::ExplicitOverloadInfo(AZStd::string_view name, AZStd::string_view categoryPath)
        : m_name(name)
        , m_categoryPath(categoryPath)
    {}

    bool ExplicitOverloadInfo::operator==(const ExplicitOverloadInfo& other) const
    {
        return m_name == other.m_name;
    }

    bool ExplicitOverloadInfo::operator!=(const ExplicitOverloadInfo& other) const
    {
        return m_name != other.m_name;
    }

    namespace BehaviorContextHelper
    {
        bool IsBehaviorClass(BehaviorContext* behaviorContext, const AZ::TypeId& id)
        {
            const auto& classIterator = behaviorContext->m_typeToClassMap.find(id);
            return (classIterator != behaviorContext->m_typeToClassMap.end());
        }

        AZ::BehaviorClass* GetClass(BehaviorContext* behaviorContext, const AZ::TypeId& id)
        {
            const auto& classIterator = behaviorContext->m_typeToClassMap.find(id);
            if (classIterator != behaviorContext->m_typeToClassMap.end())
            {
                return classIterator->second;
            }
            return nullptr;
        }

        const BehaviorClass* GetClass(const AZStd::string& classNameString)
        {
            const char* className = classNameString.c_str();
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Error("Behavior Context", false, "A behavior context is required!");
                return nullptr;
            }

            const auto classIter(behaviorContext->m_classes.find(className));
            if (classIter == behaviorContext->m_classes.end())
            {
                AZ_Warning("Behavior Context", false, "No class by name of %s in the behavior context!", className);
                return nullptr;
            }

            AZ_Assert(classIter->second, "BehaviorContext Class entry %s has no class pointer", className);
            return classIter->second;
        }

        const BehaviorClass* GetClass(const AZ::TypeId& typeID)
        {
            return GetClassAndContext(typeID).first;
        }

        AZStd::pair<const BehaviorClass*, BehaviorContext*> GetClassAndContext(const AZ::TypeId& typeID)
        {
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Error("Behavior Context", false, "A behavior context is required!");
                return { nullptr, nullptr };
            }

            if (!AZ::BehaviorContextHelper::IsBehaviorClass(behaviorContext, typeID))
            {
                return { nullptr, nullptr };
            }

            const auto classIter(behaviorContext->m_typeToClassMap.find(typeID));
            if (classIter == behaviorContext->m_typeToClassMap.end())
            {
                AZ_Assert(false, "No class by typeID of %s in the behavior context!", typeID.ToString<AZStd::string>().c_str());
                return { nullptr, nullptr };
            }

            AZ_Assert(classIter->second, "BehaviorContext class by typeID %s is nullptr in the behavior context!", typeID.ToString<AZStd::string>().c_str());
            return {classIter->second, behaviorContext};
        }

        AZ::TypeId GetClassType(const AZStd::string& classNameString)
        {
            const char* className = classNameString.c_str();
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Error("Behavior Context", false, "A behavior context is required!");
                return AZ::TypeId::CreateNull();
            }

            const auto classIter(behaviorContext->m_classes.find(className));
            if (classIter == behaviorContext->m_classes.end())
            {
                AZ_Error("Behavior Context", false, "No class by name of %s in the behavior context!", className);
                return AZ::TypeId::CreateNull();
            }

            AZ::BehaviorClass* behaviorClass(classIter->second);
            AZ_Assert(behaviorClass, "BehaviorContext Class entry %s has no class pointer", className);
            return behaviorClass->m_typeId;
        }

        bool IsStringParameter(const BehaviorParameter& parameter)
        {
            return (parameter.m_traits & AZ::BehaviorParameter::TR_STRING) == AZ::BehaviorParameter::TR_STRING;
        }
    }

    namespace Internal
    {
        bool IsInScope(const AttributeArray& attributes, const AZ::Script::Attributes::ScopeFlags scope)
        {
            // Scope defaults to Launcher
            Script::Attributes::ScopeFlags scopeType = Script::Attributes::ScopeFlags::Launcher;

            // If Scope is defined, read it
            Attribute* scopeAttribute = FindAttribute(Script::Attributes::Scope, attributes);
            if (scopeAttribute)
            {
                AZ::AttributeReader scopeAttributeReader(nullptr, scopeAttribute);
                scopeAttributeReader.Read<Script::Attributes::ScopeFlags>(scopeType);
            }

            // Do a bitwise & - if the result is equal to scope, the scope is correct.
            // This ensures that, for example, checking Common for Launcher returns true, but checking Launcher for Common does not.
            return ((static_cast<AZ::u64>(scopeType) & static_cast<AZ::u64>(scope)) == static_cast<AZ::u64>(scope));
        }

        const AZ::TypeId& GetUnderlyingTypeId(const IRttiHelper& enumRttiHelper)
        {
            const size_t underlyingTypeSize = enumRttiHelper.GetTypeSize();
            const TypeTraits underlyingTypeTraits = enumRttiHelper.GetTypeTraits();
            const bool isSigned = (underlyingTypeTraits & TypeTraits::is_signed) == TypeTraits::is_signed;
            const bool isUnsigned = (underlyingTypeTraits & TypeTraits::is_unsigned) == TypeTraits::is_unsigned;
            const bool isEnum = (underlyingTypeTraits & TypeTraits::is_enum) == TypeTraits::is_enum;
            if (isEnum)
            {
                if (isSigned)
                {
                    // Cast to either an int16_t, int32_t, int64_t depending on the size of the underlying type
                    switch (underlyingTypeSize)
                    {
                    case 1:
                        return azrtti_typeid<AZ::s8>();
                    case 2:
                        return azrtti_typeid<AZ::s16>();
                    case 4:
                        return azrtti_typeid<AZ::s32>();
                    case 8:
                        return azrtti_typeid<AZ::s64>();
                    default:
                        // The enum indicates that it is of signed type, but none of the sizes matches
                        // the fundamental integral types
                        AZ_Warning("BehaviorContext", false, "Type indicates that it is signed which is reserved for fundamental types, yet"
                            " the size of the type %zu does not match the size of a fundamental type(int8_t, int16_t, int32_t, int64_t)",
                            underlyingTypeSize);
                    }
                }
                else if (isUnsigned)
                {
                    switch (underlyingTypeSize)
                    {
                    case 1:
                        return azrtti_typeid<AZ::u8>();
                    case 2:
                        return azrtti_typeid<AZ::u16>();
                    case 4:
                        return azrtti_typeid<AZ::u32>();
                    case 8:
                        return azrtti_typeid<AZ::u64>();
                    default:
                        // The enum indicates that it is of signed type, but none of the sizes matches
                        // the fundamental integral types
                        AZ_Warning("BehaviorContext", false, "Type indicates that it is unsigned which is reserved for fundamental types, yet"
                            " the size of the type %zu does not match the size of a fundamental type(uint8_t, uint16_t, uint32_t, uint64_t)",
                            underlyingTypeSize);
                        break;
                    }
                }
            }
            return enumRttiHelper.GetTypeId();
        }
    }

} // namespace AZ
