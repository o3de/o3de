/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ::Internal
{
    // Contains template definitions for the BehaviorContext EBusBuilder<EBus>
    // and EBusBuilderBase template functions
    struct EBusBuilderBase;

    class EBusAttributes
        : public GenericAttributes<EBusBuilderBase>
    {
    protected:

        using Base = GenericAttributes<EBusBuilderBase>;

        EBusAttributes(BehaviorContext* context);

        void SetEBusEventSender(BehaviorEBusEventSender* ebusSender);

    public:
        using Base::Attribute;

        template<class T>
        EBusBuilderBase* Attribute(Crc32 idCrc, T value);

        /**
        * Applies Attribute to the Event BehaviorMethod if an EBusEventSender is set
        */
        template<class T>
        EBusBuilderBase* BroadcastAttribute(Crc32 idCrc, const T& value);

        /**
        * Applies Attribute to the Event BehaviorMethod if the EBusEventSender is set
        * and the EBus supports firing individual events
        */
        template<class T>
        EBusBuilderBase* EventAttribute(Crc32 idCrc, const T& value);

        /**
        * Applies Attribute to the Event BehaviorMethod if the EBusEventSender is set
        * and the EBus supports queuing broadcast events
        */
        template<class T>
        EBusBuilderBase* QueueBroadcastAttribute(Crc32 idCrc, const T& value);

        /**
        * Applies Attribute to the Event BehaviorMethod if the EBusEventSender is set
        * and the EBus supports queuing individual events
        */
        template<class T>
        EBusBuilderBase* QueueEventAttribute(Crc32 idCrc, const T& value);

    private:
        BehaviorEBusEventSender* m_currentEBusSender = nullptr;
    };

    /// Internal structure to maintain EBus information while describing it.
    struct EBusBuilderBase
        : public AZ::Internal::EBusAttributes
    {
        using Base = AZ::Internal::EBusAttributes;

        EBusBuilderBase(BehaviorContext* context, BehaviorEBus* behaviorEBus);
        ~EBusBuilderBase();
        EBusBuilderBase* operator->();

        /**
        * Reflects an EBus event, valid only when used with in the context of an EBus reflection.
        * We will automatically add all possible variations (Broadcast,Event,QueueBroadcast and QueueEvent)
        */
        template<class Bus, class Function>
        EBusBuilderBase* EventWithBus(const char* name, Function f, const char* deprecatedName = nullptr);

        template<class Bus, class Function>
        EBusBuilderBase* EventWithBus(const char* name, Function f, const BehaviorParameterOverridesArray<Function>& args);

        template<class Bus, class Function>
        EBusBuilderBase* EventWithBus(const char* name, Function f, const char* deprecatedName, const BehaviorParameterOverridesArray<Function>& args);

        /**
        * Every \ref EBus has two components, sending and receiving. Sending is reflected via \ref BehaviorContext::Event calls and Handler/Received
        * use handled via the receiver class. This class will receive EBus events and forward them to the behavior context functions.
        * Since we can't write a class (without using a codegen) while reflecting, you will need to implement that class
        * with the help of \ref AZ_EBUS_BEHAVIOR_BINDER. This class in mandated because you can't hook to individual events at the moment.
        * In this version of Handler you can provide a custom function to create and destroy that handler (usually where aznew/delete is not
        * applicable or you have a better pooling schema that our memory allocators already have). For most cases use the function \ref
        * Handler()
        */
        template<typename HandlerType, typename HandlerCreator, typename HandlerDestructor>
        EBusBuilderBase* Handler(HandlerCreator creator, HandlerDestructor destructor);

        /** Set the Handler/Receiver for ebus evens that will be forwarded to BehaviorFunctions. This is a helper implementation where aznew/delete
        * is ready to called on the handler.
        */
        template<class H>
        EBusBuilderBase* Handler();

        /**
         * With request buses (please refer to component communication patterns documentation) we ofter have EBus events
         * that represent a getter and a setter for a value. To allow our tools to take advantage of it, you can reflect
         * VirtualProperty to indicate which event is the getter and which is the setter.
         * This function validates that getter event has no argument and a result and setter function has no results and only
         * one argument which is the same type as the result of the getter.
         * \note Make sure you call this function after you have reflected the getter and setter events as it will report an error
         * if we can't find the function
         */
        EBusBuilderBase* VirtualProperty(const char* name, const char* getterEvent, const char* setterEvent);

        BehaviorEBus* m_ebus;
    };
}

namespace AZ::Internal
{
    //////////////////////////////////////////////////////////////////////////
    template<class Bus,class Function>
    auto EBusBuilderBase::EventWithBus(const char* name, Function e, const char* deprecatedName /*=nullptr*/)
        -> EBusBuilderBase*
    {
        BehaviorParameterOverridesArray<Function> parameterOverrides;
        return EventWithBus<Bus>(name, e, deprecatedName, parameterOverrides);
    }

    template<class Bus, class Function>
    auto EBusBuilderBase::EventWithBus(const char* name, Function e, const char* deprecatedName, const BehaviorParameterOverridesArray<Function>& args)
        -> EBusBuilderBase*
    {
        if (m_ebus)
        {
            BehaviorEBusEventSender ebusEvent;

            ebusEvent.Set<Bus>(e, name, Base::m_context);

            auto insertIt = m_ebus->m_events.insert(AZStd::make_pair(name, ebusEvent));

            if (!insertIt.second)
            {
                AZ_Error("BehaviorContext", false, "Reflection inserted a duplicate event: '%s' for bus '%s' - please check that you are not reflecting the same event repeatedly. This will cause memory leaks.", name, m_ebus->m_name.c_str());
            }
            else
            {
                // do we have a deprecated name for this event?
                if (deprecatedName != nullptr)
                {
                    // ensure deprecated name is not in use as a existing name
                    auto itr = m_ebus->m_events.find(deprecatedName);

                    if (itr != m_ebus->m_events.end())
                    {
                        AZ_Warning("BehaviorContext", false, "Event %s is attempting to use %s as a deprecated name, but the deprecated name is already in use! The deprecated name is ignored!", name, deprecatedName);
                    }
                    else
                    {
                        // ensure that we won't have a duplicate deprecated name
                        bool isDuplicated = false;
                        for (const auto& i : m_ebus->m_events)
                        {
                            if (i.second.m_deprecatedName == deprecatedName)
                            {
                                isDuplicated = true;
                                AZ_Warning("BehaviorContext", false, "Event %s is attempting to use %s as a deprecated name, but the deprecated name is already used as a deprecated name for the Event %s! The deprecated name is ignored!", name, deprecatedName, i.first.c_str());
                                break;
                            }
                        }

                        if (!isDuplicated)
                        {
                            insertIt.first->second.m_deprecatedName = deprecatedName;
                        }
                    }
                }

                for (BehaviorMethod* method : { ebusEvent.m_event, ebusEvent.m_broadcast })
                {
                    if (method)
                    {
                        size_t busIdParameterIndex = method->HasBusId() ? 1 : 0;
                        for (size_t i = 0; i < args.size(); ++i)
                        {
                            method->SetArgumentName(i + busIdParameterIndex, args[i].m_name);
                            method->SetArgumentToolTip(i + busIdParameterIndex, args[i].m_toolTip);
                            method->SetDefaultValue(i + busIdParameterIndex, args[i].m_defaultValue);
                            method->OverrideParameterTraits(i + busIdParameterIndex, args[i].m_addTraits, args[i].m_removeTraits);
                        }
                    }
                }

                Base::m_currentAttributes = &insertIt.first->second.m_attributes;
                Base::SetEBusEventSender(&insertIt.first->second);
            }
        }

        return this;
    }

    template<class Bus, class Function>
    auto EBusBuilderBase::EventWithBus(const char* name, Function e, const BehaviorParameterOverridesArray<Function>& args)
        -> EBusBuilderBase*
    {
        return EventWithBus<Bus>(name, e, nullptr, args);
    }
    template<typename HandlerType, typename HandlerCreator, typename HandlerDestructor>
    auto EBusBuilderBase::Handler(HandlerCreator creator, HandlerDestructor destructor)
        -> EBusBuilderBase*
    {
        if (m_ebus)
        {
            AZ_Assert(creator != nullptr && destructor != nullptr, "Both creator and destructor should be provided!");
            using CreatorFunctionTraits = AZStd::function_traits<HandlerCreator>;
            using CreatorFunctionCastType = AZStd::conditional_t<
                !AZStd::is_same_v<typename CreatorFunctionTraits::class_type, AZStd::Internal::error_type>,
                typename CreatorFunctionTraits::type,
                typename CreatorFunctionTraits::function_object_signature*>;
            using DestructorFunctionTraits = AZStd::function_traits<HandlerDestructor>;
            using DestructorFunctionCastType = AZStd::conditional_t<
                !AZStd::is_same_v<typename DestructorFunctionTraits::class_type, AZStd::Internal::error_type>,
                typename DestructorFunctionTraits::type,
                typename DestructorFunctionTraits::function_object_signature*>;

            BehaviorMethod* createHandler = aznew AZ::Internal::BehaviorMethodImpl(static_cast<CreatorFunctionCastType>(creator), Base::m_context, m_ebus->m_name + "::CreateHandler");
            BehaviorMethod* destroyHandler = aznew AZ::Internal::BehaviorMethodImpl(static_cast<DestructorFunctionCastType>(destructor), Base::m_context, m_ebus->m_name + "::DestroyHandler");
            // OnDemandReflect the types in all the handler Event functions
            m_ebus->m_ebusHandlerOnDemandReflector = AZStd::make_unique<ScopedBehaviorOnDemandReflector>(*Base::m_context);
            AZ::Internal::OnDemandReflectFunctions(m_ebus->m_ebusHandlerOnDemandReflector.get(), typename HandlerType::EventFunctionsParameterPack{});

            // check than the handler returns the expected type
            if (createHandler->GetResult()->m_typeId != AzTypeInfo<BehaviorEBusHandler>::Uuid() || destroyHandler->GetArgument(0)->m_typeId != AzTypeInfo<BehaviorEBusHandler>::Uuid())
            {
                AZ_Assert(false, "HandlerCreator my return a BehaviorEBusHandler* object and HandlerDestrcutor should have an argument that can handle BehaviorEBusHandler!");
                delete createHandler;
                delete destroyHandler;
                createHandler = nullptr;
                destroyHandler = nullptr;
            }
            else
            {
                Base::m_currentAttributes = &createHandler->m_attributes;
                Base::SetEBusEventSender(nullptr);
            }
            m_ebus->m_createHandler = createHandler;
            m_ebus->m_destroyHandler = destroyHandler;
        }
        return this;
    }

    template<class H>
    auto EBusBuilderBase::Handler() -> EBusBuilderBase*
    {
        Handler<H>(&AZ::Internal::BehaviorEBusHandlerFactory<H>::Create, &AZ::Internal::BehaviorEBusHandlerFactory<H>::Destroy);
        return this;
    }

    // EBusAttributes members functions
    template<class T>
    auto EBusAttributes::BroadcastAttribute(Crc32 idCrc, const T& value)
        -> EBusBuilderBase*
    {
        if (m_currentEBusSender && m_currentEBusSender->m_broadcast)
        {
            AZ::Attribute* eventAttribute = aznew AttributeContainerType<T>(value);
            Base::template SetAttributeContextData<T>(value, eventAttribute,
                AZStd::bool_constant<AZStd::is_member_function_pointer_v<T> || AZStd::is_function_v<AZStd::remove_pointer_t<T>>>());
            m_currentEBusSender->m_broadcast->m_attributes.emplace_back(AttributePair(idCrc, eventAttribute));
        }
        return static_cast<EBusBuilderBase*>(this);
    }

    template<class T>
    auto EBusAttributes::EventAttribute(Crc32 idCrc, const T& value)
        -> EBusBuilderBase*
    {
        if (!Base::m_context->IsRemovingReflection() && m_currentEBusSender && m_currentEBusSender->m_event)
        {
            AZ::Attribute* eventAttribute = aznew AttributeContainerType<T>(value);
            Base::template SetAttributeContextData<T>(value, eventAttribute,
                AZStd::bool_constant<AZStd::is_member_function_pointer_v<T> || AZStd::is_function_v<AZStd::remove_pointer_t<T>>>());
            m_currentEBusSender->m_event->m_attributes.emplace_back(AttributePair(idCrc, eventAttribute));
        }
        return static_cast<EBusBuilderBase*>(this);
    }

    template<class T>
    auto EBusAttributes::QueueBroadcastAttribute(Crc32 idCrc, const T& value)
        -> EBusBuilderBase*
    {
        if (!Base::m_context->IsRemovingReflection() && m_currentEBusSender && m_currentEBusSender->m_queueBroadcast)
        {
            AZ::Attribute* eventAttribute = aznew AttributeContainerType<T>(value);
            Base::template SetAttributeContextData<T>(value, eventAttribute,
                AZStd::bool_constant<AZStd::is_member_function_pointer_v<T> || AZStd::is_function_v<AZStd::remove_pointer_t<T>>>());
            m_currentEBusSender->m_queueBroadcast->m_attributes.emplace_back(AttributePair(idCrc, eventAttribute));
        }
        return static_cast<EBusBuilderBase*>(this);
    }

    template<class T>
    auto EBusAttributes::QueueEventAttribute(Crc32 idCrc, const T& value)
        -> EBusBuilderBase*
    {
        if (!Base::m_context->IsRemovingReflection() && m_currentEBusSender && m_currentEBusSender->m_queueEvent)
        {
            AZ::Attribute* eventAttribute = aznew AttributeContainerType<T>(value);
            Base::template SetAttributeContextData<T>(value, eventAttribute,
                AZStd::bool_constant<AZStd::is_member_function_pointer_v<T> || AZStd::is_function_v<AZStd::remove_pointer_t<T>>>());
            m_currentEBusSender->m_queueEvent->m_attributes.emplace_back(AttributePair(idCrc, eventAttribute));
        }
        return static_cast<EBusBuilderBase*>(this);
    }

    template<class T>
    auto EBusAttributes::Attribute(Crc32 idCrc, T value)
        -> EBusBuilderBase*
    {
        if (Base::m_context->IsRemovingReflection())
        {
            return static_cast<EBusBuilderBase*>(this);
        }

        // Apply attributes to each EBusEventSender BehaviorMethods if one is set on this instance
        BroadcastAttribute(idCrc, value);
        EventAttribute(idCrc, value);
        QueueBroadcastAttribute(idCrc, value);
        QueueEventAttribute(idCrc, value);

        // Apply attribute to the current bound attribute address which could on a EBus, EBusEventSender or Ebus CreateHandler instance
        if (Base::m_currentAttributes)
        {
            AZ::Attribute* ebusAttribute = aznew AttributeContainerType<T>(value);
            Base::template SetAttributeContextData<T>(value, ebusAttribute, AZStd::bool_constant<AZStd::is_member_function_pointer_v<T> || AZStd::is_function_v<AZStd::remove_pointer_t<T>>>());
            Base::m_currentAttributes->emplace_back(AttributePair(idCrc, ebusAttribute));
        }

        return static_cast<EBusBuilderBase*>(this);
    }
} // namespace AZ::Internal


namespace AZ
{
    template<typename Bus>
    struct BehaviorContext::EBusBuilder
        : public Internal::EBusBuilderBase
    {
        using Internal::EBusBuilderBase::EBusBuilderBase;

        // arrow operator for derived EBusBuilder class which hides
        // base class version
        EBusBuilder* operator->();

        // Backward Compatibility functions
        // As the majority of BehaviorContext ebus reflection uses operator->
        // for reflection, definitions for the EBusBuilderBase member functions
        // have been added to this template class, where it delegates to calling the base class
        template<class U>
        EBusBuilder* Attribute(const char* id, U value);
        template<class U>
        EBusBuilder* Attribute(AZ::Crc32 id, U value);

        // Legacy Event functions which uses the Bus class template parameter
        // for calling the EBusBuilderBase EventWithBus<Bus> function
        template<class Function>
        EBusBuilder* Event(const char* name, Function f, const char* deprecatedName = nullptr);

        template<class Function>
        EBusBuilder* Event(const char* name, Function f, const BehaviorParameterOverridesArray<Function>& args);

        template<class Function>
        EBusBuilder* Event(const char* name, Function f, const char* deprecatedName, const BehaviorParameterOverridesArray<Function>& args);

        template<typename HandlerType, typename HandlerCreator, typename HandlerDestructor>
        EBusBuilder* Handler(HandlerCreator creator, HandlerDestructor destructor);

        template<class H>
        EBusBuilder* Handler();

        EBusBuilder* VirtualProperty(const char* name, const char* getterEvent, const char* setterEvent);
    };

    template<class Bus>
    auto BehaviorContext::EBusBuilder<Bus>::operator->() -> EBusBuilder*
    {
        return this;
    }

    template<class Bus>
    template<class U>
    auto BehaviorContext::EBusBuilder<Bus>::Attribute(const char* id, U value)
        -> EBusBuilder*
    {
        EBusBuilderBase::Attribute(id, AZStd::move(value));
        return this;
    }

    template<class Bus>
    template<class U>
    auto BehaviorContext::EBusBuilder<Bus>::Attribute(AZ::Crc32 id, U value)
        -> EBusBuilder*
    {
        EBusBuilderBase::Attribute(id, AZStd::move(value));
        return this;
    }


    template<class Bus>
    template<class Function>
    auto BehaviorContext::EBusBuilder<Bus>::Event(const char* name, Function f, const char* deprecatedName)
        -> EBusBuilder*
    {
        EBusBuilderBase::EventWithBus<Bus>(name, AZStd::move(f), deprecatedName);
        return this;
    }

    template<class Bus>
    template<class Function>
    auto BehaviorContext::EBusBuilder<Bus>::Event(const char* name, Function f, const BehaviorParameterOverridesArray<Function>& args)
        -> EBusBuilder*
    {
        EBusBuilderBase::EventWithBus<Bus>(name, AZStd::move(f), args);
        return this;
    }

    template<class Bus>
    template<class Function>
    auto BehaviorContext::EBusBuilder<Bus>::Event(const char* name, Function f, const char* deprecatedName, const BehaviorParameterOverridesArray<Function>& args)
        -> EBusBuilder*
    {
        EBusBuilderBase::EventWithBus<Bus>(name, AZStd::move(f), deprecatedName, args);
        return this;
    }

    template<class Bus>
    template<typename HandlerType, typename HandlerCreator, typename HandlerDestructor>
    auto BehaviorContext::EBusBuilder<Bus>::Handler(HandlerCreator creator, HandlerDestructor destructor)
        -> EBusBuilder*
    {
        EBusBuilderBase::Handler<HandlerType>(AZStd::move(creator), AZStd::move(destructor));
        return this;
    }

    template<class Bus>
    template<class H>
    auto BehaviorContext::EBusBuilder<Bus>::Handler()
        -> EBusBuilder*
    {
        EBusBuilderBase::Handler<H>();
        return this;
    }

    template<class Bus>
    auto BehaviorContext::EBusBuilder<Bus>::VirtualProperty(const char* name, const char* getterEvent, const char* setterEvent)
        -> EBusBuilder*
    {
        EBusBuilderBase::VirtualProperty(name, getterEvent, setterEvent);
        return this;
    }
} // namespace AZ

namespace AZ
{
    // Returns a new EBusBuilder instance that can be used to configure a BehaviorEBus
    // which can reflect EBus Handlers as well as events to the Behavior Context
    template<class T>
    auto BehaviorContext::EBus(const char* name, const char* deprecatedName, const char* toolTip)
        -> EBusBuilder<T>
    {
        // should we require AzTypeInfo for EBus, technically we should if we want to work around the compiler issue that made us to do it in first place
        if (IsRemovingReflection())
        {
            auto ebusIt = m_ebuses.find(name);
            if (ebusIt != m_ebuses.end())
            {
                BehaviorContextBus::Event(this, &BehaviorContextBus::Events::OnRemoveEBus, name, ebusIt->second);

                // Erase the deprecated name as well
                auto deprecatedIt = m_ebuses.find(ebusIt->second->m_deprecatedName);
                if (deprecatedIt != m_ebuses.end())
                {
                    m_ebuses.erase(deprecatedIt);
                }

                delete ebusIt->second;
                m_ebuses.erase(ebusIt);
            }

            return EBusBuilder<T>(this, nullptr);
        }
        else
        {
            AZ_Error("BehaviorContext", m_ebuses.find(name) == m_ebuses.end(), "You shouldn't reflect an EBus multiple times (%s), subsequent reflections will not be registered!", name);

            BehaviorEBus* behaviorEBus = aznew BehaviorEBus();
            behaviorEBus->m_name = name;

            if (toolTip != nullptr)
            {
                behaviorEBus->m_toolTip = toolTip;
            }

            /*
            ** If we have a deprecated name, lets make sure the its not in use as an existing bus.
            */

            if (deprecatedName != nullptr)
            {
                if (*deprecatedName == '\0')
                {
                    AZ_Warning("BehaviorContext", false, "Deprecated name can't be a empty string!", deprecatedName);
                }
                else if (m_ebuses.find(deprecatedName) != m_ebuses.end())
                {
                    AZ_Warning("BehaviorContext", false, "EBus %s is attempting to use the deprecated name (%s) that is already used! Ignored!", name, deprecatedName);
                }
                else
                {
                    behaviorEBus->m_deprecatedName = deprecatedName;
                }
            }

            EBusSetIdFeatures<T>(behaviorEBus);
            behaviorEBus->m_queueFunction = QueueFunctionMethod<T>();

            // Switch to Set (we store the name in the class)
            m_ebuses.insert(AZStd::make_pair(behaviorEBus->m_name, behaviorEBus));
            if (!behaviorEBus->m_deprecatedName.empty())
            {
                m_ebuses.insert(AZStd::make_pair(behaviorEBus->m_deprecatedName, behaviorEBus));
            }

            return EBusBuilder<T>(this, behaviorEBus);
        }
    }
} // namespace AZ
