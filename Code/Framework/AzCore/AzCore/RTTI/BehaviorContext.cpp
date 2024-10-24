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
    // Definitions for TypeInfo and RTTI functions
    AZ_TYPE_INFO_WITH_NAME_IMPL(BehaviorContext, "BehaviorContext", "{ED75FE05-9196-4F69-A3E5-1BDF5FF034CF}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(BehaviorContext, ReflectContext);

    AZ_TYPE_INFO_WITH_NAME_IMPL(BehaviorEBusHandler, "BehaviorEBusHandler", "{10FBCB9D-8A0D-47E9-8A51-CBD9BFBBF60D}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(BehaviorEBusHandler);

    AZ_TYPE_INFO_WITH_NAME_IMPL(BehaviorObject, "BehaviorObject", "{2813CDFB-0A4A-411C-9216-72A7B644D1DD}");
    AZ_TYPE_INFO_WITH_NAME_IMPL(BehaviorParameter, "BehaviorParameter", "{BD7B664E-5B8C-4B51-84F3-DE89B271E075}");
    AZ_TYPE_INFO_WITH_NAME_IMPL(BehaviorArgument, "BehaviorArgument", "{B1680AE9-4DBE-4803-B12F-1E99A32990B7}");

    AZ_TYPE_INFO_WITH_NAME_IMPL(BehaviorAzEventDescription, "BehaviorAzEventDescription", "{B5D95E87-FA17-41C7-AC90-7258A520FE82}");

    AZ_TYPE_INFO_WITH_NAME_IMPL(InputRestriction, "InputRestriction", "{9DF4DDBE-63BE-4749-9921-52C82BF5E307}");
    AZ_TYPE_INFO_WITH_NAME_IMPL(BranchOnResultInfo, "BranchOnResultInfo", "{C063AB6F-462F-485F-A911-DE3A8946A019}");
    AZ_TYPE_INFO_WITH_NAME_IMPL(CheckedOperationInfo, "CheckedOperationInfo", "{9CE9560F-ECAB-46EF-B341-3A86973E71CD}");
    AZ_TYPE_INFO_WITH_NAME_IMPL(OverloadArgumentGroupInfo, "OverloadArgumentGroupInfo", "{AEFEFC42-3ED8-43A9-AE1F-6D8F32A280D2}");
    AZ_TYPE_INFO_WITH_NAME_IMPL(ExplicitOverloadInfo, "ExplicitOverloadInfo", "{B6064A17-E907-4CB5-8EAE-C4888E468CD5}");

    AZ_TYPE_INFO_WITH_NAME_IMPL(EventHandlerCreationFunctionHolder, "EventHandlerCreationFunctionHolder", "{40F7C5D8-8DA0-4979-BC8C-0A52EDA80633}");

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

                [[maybe_unused]] size_t parameterIndex = 0;
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

    // BehaviorParameterOverrides member definitions
    BehaviorParameterOverrides::BehaviorParameterOverrides(AZStd::string_view name, AZStd::string_view toolTip, BehaviorDefaultValuePtr defaultValue,
        u32 addTraits, u32 removeTraits)
        : m_name(name)
        , m_toolTip(toolTip)
        , m_defaultValue(defaultValue)
        , m_addTraits(addTraits)
        , m_removeTraits(removeTraits)
    {}

    // BehaviorDefaultValue member definitions
    BehaviorDefaultValue::~BehaviorDefaultValue()
    {
        if (m_value.m_value && m_destructor)
        {
            m_destructor(m_value.m_value);
        }
    }

    const BehaviorArgument& BehaviorDefaultValue::GetValue() const
    {
        return m_value;
    }

    // BehaviorObject member functions
    BehaviorObject::BehaviorObject()
        : m_address(nullptr)
        , m_typeId(AZ::Uuid::CreateNull())
    {
    }

    BehaviorObject::BehaviorObject(void* address, const Uuid& typeId)
        : m_address(address)
        , m_typeId(typeId)
    {
    }

    BehaviorObject::BehaviorObject(void* address, IRttiHelper* rttiHelper)
        : m_address(address)
        , m_rttiHelper(rttiHelper)
    {
        m_typeId = rttiHelper ? rttiHelper->GetTypeId() : AZ::Uuid::CreateNull();
    }

    bool BehaviorObject::IsValid() const
    {
        return m_address && !m_typeId.IsNull();
    }


    // BehaviorArgument member functions
    BehaviorArgument::BehaviorArgument()
        : m_value(nullptr)
    {
        m_name = nullptr;
        m_typeId = Uuid::CreateNull();
        m_azRtti = nullptr;
        m_traits = 0;
    }

    BehaviorArgument::BehaviorArgument(BehaviorArgument&& other)
        : BehaviorParameter(AZStd::move(other))
        , m_value(AZStd::move(other.m_value))
        , m_onAssignedResult(AZStd::move(other.m_onAssignedResult))
        , m_tempData(AZStd::move(other.m_tempData))
    {
    }

    BehaviorArgument::BehaviorArgument(BehaviorObject* value)
    {
        Set(value);
    }

    BehaviorArgument::BehaviorArgument(BehaviorArgumentValueTypeTag_t, BehaviorObject* value)
    {
        Set(BehaviorArgumentValueTypeTag, value);
    }

    void BehaviorArgument::Set(BehaviorObject* value)
    {
        m_value = &value->m_address;
        m_typeId = value->m_typeId;
        m_traits = BehaviorParameter::TR_POINTER;
        m_name = value->m_rttiHelper ? value->m_rttiHelper->GetActualTypeName(value->m_address) : nullptr;
        m_azRtti = value->m_rttiHelper;
    }

    void BehaviorArgument::Set(BehaviorArgumentValueTypeTag_t, BehaviorObject* value)
    {
        m_value = value->m_address;
        m_typeId = value->m_typeId;
        m_traits = BehaviorParameter::TR_NONE;
        m_name = value->m_rttiHelper ? value->m_rttiHelper->GetActualTypeName(value->m_address) : nullptr;
        m_azRtti = value->m_rttiHelper;
    }

    void BehaviorArgument::Set(const BehaviorParameter& param)
    {
        *static_cast<BehaviorParameter*>(this) = param;
    }

    void BehaviorArgument::Set(const BehaviorArgument& param)
    {
        *static_cast<BehaviorParameter*>(this) = static_cast<const BehaviorParameter&>(param);
        m_value = param.m_value;
        m_onAssignedResult = param.m_onAssignedResult;
        m_tempData = param.m_tempData;
    }

    void* BehaviorArgument::GetValueAddress() const
    {
        void* valueAddress = m_value;
        if (m_traits & BehaviorParameter::TR_POINTER)
        {
            valueAddress = *reinterpret_cast<void**>(valueAddress); // pointer to a pointer
        }
        return valueAddress;
    }


    bool BehaviorArgument::ConvertTo(const AZ::Uuid& typeId)
    {
        if (m_azRtti)
        {
            void* valueAddress = GetValueAddress();
            if (valueAddress) // should we make null value to convert to anything?
            {
                return AZ::Internal::ConvertValueTo(valueAddress, m_azRtti, typeId, m_value, m_tempData);
            }
        }
        return m_typeId == typeId;
    }

    BehaviorArgument::operator BehaviorObject() const
    {
        return BehaviorObject(m_value, m_azRtti);
    }

    BehaviorArgument& BehaviorArgument::operator=(BehaviorArgument&& other)
    {
        *static_cast<BehaviorParameter*>(this) = AZStd::move(static_cast<BehaviorParameter&&>(other));
        m_value = AZStd::move(other.m_value);
        m_onAssignedResult = AZStd::move(other.m_onAssignedResult);
        m_tempData = AZStd::move(other.m_tempData);
        return *this;
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

    void BehaviorMethod::SetDeprecatedName(AZStd::string name)
    {
        m_deprecatedName = AZStd::move(name);
    }
    const AZStd::string& BehaviorMethod::GetDeprecatedName() const
    {
        return m_deprecatedName;
    }

    //////////////////////////////////////////////////////////////////////////
    bool BehaviorMethod::Invoke() const
    {
        return Call(nullptr, 0, nullptr);
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


    // BehaviorMethod legacy Call forwarder
    bool BehaviorMethod::Call(BehaviorArgument* arguments, unsigned int numArguments, BehaviorArgument* result) const
    {
        return Call(AZStd::span(arguments, numArguments), result);
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

    void BehaviorMethod::ProcessAuxiliaryMethods(BehaviorContext* context, BehaviorMethod& method)
    {
        if (Attribute* attribute = AZ::FindAttribute(ScriptCanvasAttributes::CheckedOperation, method.m_attributes))
        {
            CheckedOperationInfo checkedOperationInfo;
            if (AttributeReader(nullptr, attribute).Read<CheckedOperationInfo>(checkedOperationInfo))
            {
                auto iter = context->m_methods.find(checkedOperationInfo.m_safetyCheckName);
                if (iter != context->m_methods.end())
                {
                    context->m_checksByOperations.insert(AZStd::make_pair(&method, AZStd::make_pair(iter->second, nullptr)));
                }
                else
                {
                    AZ_Error("BehaviorContext", false, "Method %s declared safety check %s, but it was not found in context.s",
                        m_name.c_str(), checkedOperationInfo.m_safetyCheckName.c_str());
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
                    auto iter = context->m_methods.find(branchOnResultInfo.m_nonBooleanResultCheckName);
                    if (iter != context->m_methods.end())
                    {
                        context->m_checksByOperations.insert(AZStd::make_pair(&method, AZStd::make_pair(iter->second, nullptr)));
                    }
                    else
                    {
                        AZ_Error("BehaviorContext", false, "Method %s declared safety check %s, but it was not found in context.",
                            m_name.c_str(), branchOnResultInfo.m_nonBooleanResultCheckName.c_str());
                    }
                }
            }
        }
    }

    //=========================================================================
    // GetTypeId
    //=========================================================================
    AZ::TypeId BehaviorProperty::GetTypeId() const
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
    BehaviorEBus::VirtualProperty::VirtualProperty(BehaviorEBusEventSender* getter, BehaviorEBusEventSender* setter)
        : m_getter(getter)
        , m_setter(setter)
    {}

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
            m_method->ProcessAuxiliaryMethods(Base::m_context, *m_method);
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

    // Reflect the following types by default
    // AZStd::string
    // AZStd::string_view
    // AZStd::fixed_string<1024>
    // This skips over the need to reflect the type via OnDemandReflection system
    // saving build time across the board
    BehaviorContext::BehaviorContext()
    {
        CommonOnDemandReflections::ReflectCommonString(this);
        CommonOnDemandReflections::ReflectCommonFixedString(this);
        CommonOnDemandReflections::ReflectCommonStringView(this);
    }

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

    // BehaviorContext bound objects query functions
    const BehaviorMethod* BehaviorContext::FindMethodByReflectedName(AZStd::string_view reflectedName) const
    {
        auto methodIt = m_methods.find(reflectedName);
        return methodIt != m_methods.end() ? methodIt->second : nullptr;
    }
    const BehaviorProperty* BehaviorContext::FindPropertyByReflectedName(AZStd::string_view reflectedName) const
    {
        auto propertyIt = m_properties.find(reflectedName);
        return propertyIt != m_properties.end() ? propertyIt->second : nullptr;
    }
    const BehaviorMethod* BehaviorContext::FindGetterByReflectedName(AZStd::string_view reflectedName) const
    {
        auto propertyIt = m_properties.find(reflectedName);
        return propertyIt != m_properties.end() && propertyIt->second != nullptr ? propertyIt->second->m_getter : nullptr;
    }
    const BehaviorMethod* BehaviorContext::FindSetterByReflectedName(AZStd::string_view reflectedName) const
    {
        auto propertyIt = m_properties.find(reflectedName);
        return propertyIt != m_properties.end() && propertyIt->second != nullptr ? propertyIt->second->m_setter : nullptr;
    }
    const BehaviorClass* BehaviorContext::FindClassByReflectedName(AZStd::string_view reflectedName) const
    {
        auto classIt = m_classes.find(reflectedName);
        return classIt != m_classes.end() ? classIt->second : nullptr;
    }
    const BehaviorClass* BehaviorContext::FindClassByTypeId(const AZ::TypeId& typeId) const
    {
        auto classTypeIt = m_typeToClassMap.find(typeId);
        return classTypeIt != m_typeToClassMap.end() ? classTypeIt->second : nullptr;
    }
    const BehaviorEBus* BehaviorContext::FindEBusByReflectedName(AZStd::string_view reflectedName) const
    {
        auto ebusIt = m_ebuses.find(reflectedName);
        return ebusIt != m_ebuses.end() ? ebusIt->second: nullptr;
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



    // ScopedBehaviorObject class constructor/destructor definition
    BehaviorClass::ScopedBehaviorObject::ScopedBehaviorObject() = default;
    BehaviorClass::ScopedBehaviorObject::ScopedBehaviorObject(AZ::BehaviorObject&& behaviorObject, CleanupFunction cleanupFunction)
        : m_behaviorObject(behaviorObject)
        , m_cleanupFunction(AZStd::move(cleanupFunction))
    {

    }

    // Move assignment needs to deallocate the previous scoped object
    auto BehaviorClass::ScopedBehaviorObject::operator=(ScopedBehaviorObject&& other) ->ScopedBehaviorObject&
    {
        if (this != &other)
        {
            // Swap with the other scoped behavior object
            // When the other object goes out of scope it will cleanup the old BehaviorObject instance.
            AZStd::swap(m_behaviorObject, other.m_behaviorObject);
            AZStd::swap(m_cleanupFunction, other.m_cleanupFunction);
        }
        return *this;
    }

    BehaviorClass::ScopedBehaviorObject::~ScopedBehaviorObject()
    {
        if (m_cleanupFunction)
        {
            m_cleanupFunction(m_behaviorObject);
        }
    }

    bool BehaviorClass::ScopedBehaviorObject::IsValid() const
    {
        return m_behaviorObject.IsValid();
    }

    auto BehaviorClass::CreateWithScope() const -> ScopedBehaviorObject
    {
        return ScopedBehaviorObject(Create(Allocate()), [this](const AZ::BehaviorObject& behaviorObject)
            {
                this->Destroy(behaviorObject);
            });
    }

    auto BehaviorClass::CreateWithScope(void* address) const -> ScopedBehaviorObject
    {
        return ScopedBehaviorObject(Create(address), [this](const AZ::BehaviorObject& behaviorObject)
            {
                if (behaviorObject.m_typeId == m_typeId && this->m_destructor && behaviorObject.m_address)
                {
                    this->m_destructor(behaviorObject.m_address, m_userData);
                }
            });
    }

    auto BehaviorClass::CreateWithScope(AZStd::span<BehaviorArgument> arguments) const -> ScopedBehaviorObject
    {
        // If the arguments list is empty fallback to using the default constructor overload
        if (arguments.empty())
        {
            return CreateWithScope();
        }

        AZ::BehaviorObject selfObject(Allocate(), m_azRtti);
        AZStd::fixed_vector<AZ::BehaviorArgument, 64> addressPlusArguments{ AZ::BehaviorArgument{&selfObject} };
        addressPlusArguments.append_range(arguments);

        bool constructorInvoked{};
        for (AZ::BehaviorMethod* constructor : m_constructors)
        {
            if (constructor->IsCallable(addressPlusArguments) && constructor->Call(addressPlusArguments))
            {
                constructorInvoked = true;
                break;
            }
        }

        if (constructorInvoked)
        {
            return ScopedBehaviorObject(AZStd::move(selfObject), [this](const AZ::BehaviorObject& behaviorObject)
                {
                    this->Destroy(behaviorObject);
                });
        }

        // No the constructor has been invoked, so deallocate the new object instance created in this function
        Deallocate(selfObject.m_address);

        return {};
    }

    auto BehaviorClass::CreateWithScope(void* address, AZStd::span<BehaviorArgument> arguments) const -> ScopedBehaviorObject
    {
        // If the arguments list is empty fallback to using the default constructor overload with the supplied memory address
        if (arguments.empty())
        {
            return CreateWithScope(address);
        }

        // Use the supplied memory address to create a new object instance
        AZ::BehaviorObject selfObject(address, m_azRtti);
        AZStd::fixed_vector<AZ::BehaviorArgument, 64> addressPlusArguments{ AZ::BehaviorArgument{&selfObject} };
        addressPlusArguments.append_range(arguments);

        bool constructorInvoked{};
        for (AZ::BehaviorMethod* constructor : m_constructors)
        {
            if (constructor->IsCallable(addressPlusArguments) && constructor->Call(addressPlusArguments))
            {
                constructorInvoked = true;
                break;
            }
        }

        if (constructorInvoked)
        {
            return ScopedBehaviorObject(AZStd::move(selfObject), [this](const AZ::BehaviorObject& behaviorObject)
                {
                    this->Destroy(behaviorObject);
                });
        }

        return {};
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

    // Property method query functions implementations
    const BehaviorProperty* BehaviorClass::FindPropertyByReflectedName(AZStd::string_view reflectedName) const
    {
        auto propertyIter = m_properties.find(reflectedName);
        return propertyIter != m_properties.end() ? propertyIter->second : nullptr;
    }

    const BehaviorMethod* BehaviorClass::FindGetterByReflectedName(AZStd::string_view reflectedName) const
    {
        const BehaviorProperty* behaviorProperty = FindPropertyByReflectedName(reflectedName);
        return behaviorProperty != nullptr ? behaviorProperty->m_getter : nullptr;
    }

    const BehaviorMethod* BehaviorClass::FindSetterByReflectedName(AZStd::string_view reflectedName) const
    {
        const BehaviorProperty* behaviorProperty = FindPropertyByReflectedName(reflectedName);
        return behaviorProperty != nullptr ? behaviorProperty->m_setter : nullptr;
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
            return azmalloc(m_size, m_alignment, AZ::SystemAllocator);
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

    // Deleter operator for the unwrapper unique_ptr deleter
    void UnwrapperFuncDeleter::operator()(void* ptr) const
    {
        if (m_deleter && ptr)
        {
            m_deleter(ptr);
        }
    }

    UnwrapperUserData::UnwrapperUserData() = default;
    UnwrapperUserData::UnwrapperUserData(UnwrapperUserData&& other) = default;
    UnwrapperUserData& UnwrapperUserData::operator=(UnwrapperUserData&& other) = default;
    UnwrapperUserData::~UnwrapperUserData() = default;


    //////////////////////////////////////////////////////////////////////////
    void BehaviorContextEvents::OnAddGlobalMethod(const char*, BehaviorMethod*) {}
    void BehaviorContextEvents::OnRemoveGlobalMethod(const char*, BehaviorMethod*) {}

    /// Called when a new global property is reflected in behavior context or remove from it
    void BehaviorContextEvents::OnAddGlobalProperty(const char*, BehaviorProperty*) {}
    void BehaviorContextEvents::OnRemoveGlobalProperty(const char*, BehaviorProperty*) {}

    /// Called when a class is added or removed
    void BehaviorContextEvents::OnAddClass(const char*, BehaviorClass*) {}
    void BehaviorContextEvents::OnRemoveClass(const char*, BehaviorClass*) {}

    /// Called when a ebus is added or removed
    void BehaviorContextEvents::OnAddEBus(const char*, BehaviorEBus*) {}
    void BehaviorContextEvents::OnRemoveEBus(const char*, BehaviorEBus*) {}
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

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

        AZ::TypeId GetUnderlyingTypeId(const IRttiHelper& enumRttiHelper)
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

        bool ConvertValueTo(void* sourceAddress, const IRttiHelper* sourceRtti, const AZ::Uuid& targetType, void*& targetAddress, BehaviorParameter::TempValueParameterAllocator& tempAllocator)
        {
            // Check see if the underlying typeid is an enum whose typeIds match
            if (GetUnderlyingTypeId(*sourceRtti) == targetType)
            {
                return true;
            }
            // convert
            void* convertedAddress = sourceRtti->Cast(sourceAddress, targetType);
            if (convertedAddress && convertedAddress != sourceAddress) // if we converted as we have a different address
            {
                // allocate temp storage and store it
                targetAddress = tempAllocator.allocate(sizeof(void*), AZStd::alignment_of<void*>::value, 0);
                *reinterpret_cast<void**>(targetAddress) = convertedAddress;
            }
            return convertedAddress != nullptr;
        }
    }
} // namespace AZ
