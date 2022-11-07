/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Metrics/IEventLogger.h>
#include <AzCore/Metrics/EventLoggerReflectUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ::Metrics
{
    inline namespace Script
    {
        // Script Event Arguments use container types with persistent storage such as AZStd::string
        // instead of AZStd::string_view for storing string arguments and
        // AZStd::vector instead AZStd::span for storing arrays
        // This persistent storage is needed in script to allow scripters
        // to not deal with lifetime concerns related to the event argument structures
        // The C++ event argument structures located in IEventLogger.h uses non-persistent
        // storage for the EventArg derived structures and expect users to provide a lifetime
        // for fields supplied to the EventArg structure at least as long as the call
        // to the `IEventLogger::Record*Event` call.

        //! Represents a value of a field in the arguments supplied to Record*Event
        //! It is also used to supply arguments to JSON arrays in event arguments
        struct ScriptEventValue;
        //! Represents a field that contains a name and ScriptEventValue for supplying
        //! arguments to JSON objects in event arguments
        struct ScriptEventField;

        //! Encapsulates an array structure suitable for storing
        //! JSON array arguments to supply as part of the event arguments
        struct ScriptEventArray
        {
            AZ_TYPE_INFO(ScriptEventArray, "{830941A8-ECF4-421B-81AC-5C53D9224881}");

            ScriptEventArray() = default;
            ScriptEventArray(AZStd::vector<ScriptEventValue> value)
                : m_value(AZStd::move(value))
            {}
            AZStd::vector<ScriptEventValue> m_value;
        };

        //! Encapsulates an array structure suitable for storing
        //! JSON object arguments to supply as part of the event arguments
        struct ScriptEventObject
        {
            AZ_TYPE_INFO(ScriptEventObject, "{8970F79F-0613-4B54-872B-2B48702EE5AB}");

            ScriptEventObject() = default;
            ScriptEventObject(AZStd::vector<ScriptEventField> value)
                : m_value(AZStd::move(value))
            {}

            AZStd::vector<ScriptEventField> m_value;
        };

        struct ScriptEventValue
        {
            AZ_TYPE_INFO(ScriptEventValue, "{665521F8-C9C3-440E-86CE-3BD1D6471ECE}");

            // Need constructors that can initialize a ScriptEventValue for each type of variant
            // in order to use the BehaviorContext::ClassBuilder::Constructor function
            ScriptEventValue() = default;
            ScriptEventValue(AZStd::string value)
                : m_value(AZStd::move(value))
            {}
            ScriptEventValue(bool value)
                : m_value(value)
            {}
            ScriptEventValue(AZ::s64 value)
                : m_value(value)
            {}
            ScriptEventValue(AZ::u64 value)
                : m_value(value)
            {}
            ScriptEventValue(double value)
                : m_value(value)
            {}
            ScriptEventValue(ScriptEventArray value)
                : m_value(AZStd::move(value))
            {}
            ScriptEventValue(ScriptEventObject value)
                : m_value(AZStd::move(value))
            {}

            using ArgsVariant = AZStd::variant<
                AZStd::string,
                bool,
                AZ::s64,
                AZ::u64,
                double,
                ScriptEventArray,
                ScriptEventObject
            >;

            //! Stores the data associated with an event field
            ArgsVariant m_value;
        };

        struct ScriptEventField
        {
            AZ_TYPE_INFO(ScriptEventField, "{3823CEFA-30BA-41BB-81E6-FD22FA5CBC1B}");

            ScriptEventField() = default;

            ScriptEventField(AZStd::string name, ScriptEventValue value)
                : m_name(AZStd::move(name))
                , m_value(AZStd::move(value))
            {}

            // field name
            AZStd::string m_name;
            // field value
            ScriptEventValue m_value;
        };
        struct ScriptEventArgs
        {
            //! name of the event
            AZStd::string m_name;
            //! comma separated list of categories associated with the event
            AZStd::string m_cat;
            //! Arguments used to fill the "args" field for each trace event
            AZStd::vector<ScriptEventField> m_args;
        };


        //! Represents the module to use when importing the event logger metrics functions
        //! This is used in Python automation. ex. `import azlmbr.metrics`
        static constexpr const char* MetricsModuleName = "metrics";
        //! Represents the category to organize the event logger classes and functions
        //! witin the ScriptCanvas NodePalette
        static constexpr const char* MetricsCategory = "Metrics";

        void Reflect(AZ::ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
                behaviorContext != nullptr)
            {
                ReflectScript(*behaviorContext);
            }
        }

        static void ReflectEventDataTypes(AZ::BehaviorContext& behaviorContext)
        {
            // Reflect the EventValue
            behaviorContext.Class<ScriptEventValue>("EventValue")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Constructor()
                ->Constructor<AZStd::string>()
                ->Constructor<bool>()
                ->Constructor<AZ::s64>()
                ->Constructor<AZ::u64>()
                ->Constructor<double>()
                ->Constructor<ScriptEventArray>()
                ->Constructor<ScriptEventObject>()
                ;

            // Reflect the EventField
            behaviorContext.Class<ScriptEventField>("EventField")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Constructor()
                ->Constructor<AZStd::string, ScriptEventValue>()
                ;

            // Reflect the EventArray
            behaviorContext.Class<ScriptEventArray>("EventArray")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Constructor()
                ->Constructor<AZStd::vector<ScriptEventValue>>()
                ;

            // Reflect the EventObject
            behaviorContext.Class<ScriptEventObject>("EventObject")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Constructor()
                ->Constructor<AZStd::vector<ScriptEventField>>()

                ;
        }

        void ReflectScript(AZ::BehaviorContext& behaviorContext)
        {
            ReflectEventDataTypes(behaviorContext);
        }
    } // inline namespace Utility
} // namespace AZ::Metrics
