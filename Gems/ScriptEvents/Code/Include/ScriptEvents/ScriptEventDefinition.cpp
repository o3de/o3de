/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEventDefinition.h"
#include "ScriptEventsBus.h"

#include <AzCore/std/string/regex.h>

namespace ScriptEvents
{
    void ScriptEvent::RegisterInternal()
    {
        // Register the bus with the system component
        ScriptEventBus::Broadcast(&ScriptEventRequests::RegisterScriptEventFromDefinition, *this);
    }

    void ScriptEvent::Register(AZ::ScriptDataContext&)
    {
        RegisterInternal();
    }

    void ScriptEvent::Release(AZ::ScriptDataContext&)
    {
    }

    void ScriptEvent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptEvent>()
                ->Version(2)
                ->Field("m_version", &ScriptEvent::m_version)
                ->Field("m_name", &ScriptEvent::m_name)
                ->Field("m_category", &ScriptEvent::m_category)
                ->Field("m_tooltip", &ScriptEvent::m_tooltip)
                ->Field("m_addressType", &ScriptEvent::m_addressType)
                ->Field("m_methods", &ScriptEvent::m_methods)
                ->Field("scriptCanvasSerializedData", &ScriptEvent::m_scriptCanvasSerializedData)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ScriptEvent>("Script Event Definition", "Data driven script event definition")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::ChildNameLabelOverride, &ScriptEvent::GetLabel)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptEvent::m_name, "Name", "Name of the Script Event")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptEvent::m_tooltip, "Tooltip", "The name of this Script Event")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptEvent::m_category, "Category", "The category that the Event will be put into")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ScriptEvent::m_addressType, "Address Type", "If required, this defines the address type for this event")
                        ->Attribute(AZ::Edit::Attributes::GenericValueList, &Types::GetValidAddressTypes)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptEvent::m_methods, "Events", "The list of events available.")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ScriptEvent>("ScriptEvent")
                ->Constructor<AZ::ScriptDataContext&>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("AddMethod", &ScriptEvent::AddMethod)
                ->Method("Register", &ScriptEvent::Register)
                ->Property("Name", BehaviorValueProperty(&ScriptEvent::m_name))
                ->Property("AddressType", BehaviorValueProperty(&ScriptEvent::m_addressType))
                ->Property("Events", BehaviorValueProperty(&ScriptEvent::m_methods))
                ;
        }
    }

    AZ::Outcome<bool, AZStd::string> ScriptEvent::Validate() const
    {
        const AZStd::string& name = GetName();
        const AZ::Uuid& addressType = GetAddressType();

        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
        AZ_Assert(behaviorContext, "A valid Behavior Context is expected");

        if (m_version == 0 && behaviorContext->m_ebuses.find(name.c_str()) != behaviorContext->m_ebuses.end())
        {
            // An EBus with the same name already is registered, this is not allowed.
            return AZ::Failure(AZStd::string::format("A Script Event with the name \"%s\" already exist, consider renaming this Script Event as duplicate names are not supported", name.c_str()));
        }

        // Validate address type
        if (!Types::ValidateAddressType(addressType))
        {
            return AZ::Failure(AZStd::string::format("The specified type %s is not valid as an address for Script Events: %s", addressType.ToString<AZStd::string>().c_str(), name.c_str()));
        }
        
        // Definition name cannot be empty
        if (name.empty())
        {
            return AZ::Failure(AZStd::string("Event name cannot be empty"));
        }

        // Name cannot start with a number
        if (isdigit(name.at(0)))
        {
            return AZ::Failure(AZStd::string::format("%s, names cannot start with a number", name.c_str()));
        }

        AZStd::smatch match;

        // Ascii-only
        AZStd::regex asciionly_regex("[^\x0A\x0D\x20-\x7E]");
        AZStd::regex_match(name, match, asciionly_regex);
        if (!match.empty())
        {
            return AZ::Failure(AZStd::string::format("%s, invalid name, names may only contain ASCII characters", name.c_str()));
        }

        // No whitespace
        AZStd::regex nowhitespace_regex("[^\\S]");
        AZStd::regex_match(name, match, nowhitespace_regex);
        if (!match.empty())
        {
            return AZ::Failure(AZStd::string::format("%s, invalid name, event names should not contain white space", name.c_str()));
        }

        // Conform to valid function names
        AZStd::regex validate_regex("[_[:alpha:]][_[:alnum:]]*");
        AZStd::regex_match(name, match, validate_regex);
        if (match.empty())
        {
            return AZ::Failure(AZStd::string::format("%s, invalid name specified, event name must only have alpha numeric characters, may not start with a number and may not have white space", name.c_str()));
        }

        AZ_Warning("Script Events", !m_methods.empty(), AZStd::string::format("Script Events (%s) must provide at least one event, otherwise they are unusable.", name.c_str()).c_str());

        // Validate each method
        AZStd::string methodName;
        int methodIndex = 0;
        for (const Method& method : m_methods)
        {
            auto outcome = method.Validate();
            if (!outcome.IsSuccess())
            {
                return outcome;
            }

            int duplicateCount = 0;
            for (auto item = m_methods.begin(); item != m_methods.end(); item++)
            {
                if (item->GetName().compare(methodName) == 0)
                {
                    duplicateCount++;
                }
                if (duplicateCount > 1)
                {
                    return AZ::Failure(AZStd::string::format(
                        "Cannot have duplicate method names (%d: %s) make sure each method name is unique",
                        methodIndex,
                        methodName.c_str()));
                }
            }
            methodName = method.GetName();
            ++methodIndex;

        }

        return AZ::Success(true);
    }

}
