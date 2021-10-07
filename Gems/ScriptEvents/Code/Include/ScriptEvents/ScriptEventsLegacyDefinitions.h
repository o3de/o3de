/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>




namespace ScriptEventsLegacy
{
    /**
    * This class represents an EBus event parameter.
    * void Foo(parameterType parameterName)
    *          ^^^^^^^^^^^^^^^^^^^^^^^^^^^
    *          parameter
    */
    struct ParameterDefinition
    {
        AZ_TYPE_INFO(ParameterDefinition, "{6586FFB5-0FF6-424F-A542-C797E2FF3458}");
        AZ_CLASS_ALLOCATOR(ParameterDefinition, AZ::SystemAllocator, 0);

        ParameterDefinition() = default;
        ParameterDefinition(const AZStd::string& name, const AZStd::string& tooltip, const AZ::Uuid& type)
            : m_name(name)
            , m_tooltip(tooltip)
            , m_type(type)
        {}

        AZStd::string m_name;
        AZStd::string m_tooltip;
        AZ::Uuid m_type = AZ::BehaviorContext::GetVoidTypeId();
    };

    /**
    * This class represents an EBus event.
    * void          Foo        (parameterType parameterName, parameterType2 parameterName2)
    * ^^^^          ^^^         ^^^^^^^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    * m_returnType, m_name,     m_parameters
    */
    struct EventDefinition
    {
        AZ_TYPE_INFO(EventDefinition, "{211BB356-FA42-400F-B3DD-9326C6A686B6}");
        AZ_CLASS_ALLOCATOR(EventDefinition, AZ::SystemAllocator, 0);

        EventDefinition() = default;
        EventDefinition(const AZStd::string& eventName, const AZStd::string& tooltip, const AZ::Uuid& returnValue, const AZStd::vector<ParameterDefinition>& parameters)
            : m_name(eventName)
            , m_tooltip(tooltip)
            , m_returnType(returnValue)
            , m_parameters(parameters)
        {}

        AZStd::string m_name;
        AZStd::string m_tooltip;
        AZ::Uuid m_returnType = AZ::BehaviorContext::GetVoidTypeId();
        AZStd::vector<ParameterDefinition> m_parameters;
    };

    /**
    * This class represents EBus type traits.
    * At the moment only bus id type is supported.
    */
    struct TypeTraitsDefinition
    {
        AZ_TYPE_INFO(TypeTraitsDefinition, "{EC374DE0-8003-4572-BC26-C4A8DBE50AB6}");
        AZ_CLASS_ALLOCATOR(TypeTraitsDefinition, AZ::SystemAllocator, 0);

        TypeTraitsDefinition() = default;
        TypeTraitsDefinition(const AZ::Uuid& busIdType)
            : m_busIdType(busIdType) {}

        AZ::Uuid m_busIdType = AZ::BehaviorContext::GetVoidTypeId();
    };

    /**
    * This class represents an EBus.
    * An EBus has a name, traits, and a collection of events
    * Configurable EBuses are added to the Behavior Context as both Request and Notification buses
    */
    struct Definition
    {
        AZ_TYPE_INFO(Definition, "{4663215E-8137-4A16-979D-26B48401F40D}");
        AZ_CLASS_ALLOCATOR(Definition, AZ::SystemAllocator, 0);

        Definition() = default;
        Definition(const AZStd::string& name, const AZStd::string& tooltip, const TypeTraitsDefinition& traits, const AZStd::vector<EventDefinition>& events)
            : m_name(name)
            , m_tooltip(tooltip)
            , m_traits(traits)
            , m_events(events)
        {}

        EventDefinition FindEvent(const char* name) const
        {
            for (const EventDefinition& eventDefinition : m_events)
            {
                if (eventDefinition.m_name.compare(name) == 0)
                {
                    return eventDefinition;
                }
            }

            return EventDefinition();
        }

        AZStd::string m_name;
        AZStd::string m_tooltip;
        AZStd::string m_category = "Custom Events";
        TypeTraitsDefinition m_traits;
        AZStd::vector<EventDefinition> m_events;
    };
}
