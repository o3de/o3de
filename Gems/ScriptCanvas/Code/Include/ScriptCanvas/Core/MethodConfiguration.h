/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "SlotExecutionMap.h"
#include "SlotConfigurations.h"
#include "SlotConfigurationDefaults.h"
#include "ModifiableDatumView.h"
#include "Node.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class Method;
        }
    }

    enum class MethodType
    {
        Event,
        Free,
        Member,
        Getter,
        Setter,
        Count,
    };

    struct MethodConfiguration
    {
        const AZ::BehaviorMethod& m_method;
        const AZ::BehaviorClass* m_class = nullptr;
        const NamespacePath* m_namespaces = nullptr;
        const AZStd::string_view* m_className = nullptr;
        const AZStd::string_view* m_lookupName = nullptr; // the look-up name in the class, rather than method.m_name
        AZStd::string m_prettyClassName;
        MethodType m_methodType = MethodType::Count;
        EventType m_eventType = EventType::Count;

        AZ_INLINE MethodConfiguration(const AZ::BehaviorMethod& method, MethodType type) : m_method(method), m_methodType(type) {}
    };

    struct MethodOutputConfig
    {
        Nodes::Core::Method* methodNode;
        const MethodConfiguration& config;
        bool isReturnValueOverloaded = false;
        bool isOutcomeOutputMethod = false;
        AZStd::string outcomeNamePrefix;
        AZStd::string outputNamePrefix;
        AZStd::vector<SlotId>* resultSlotIdsOut = nullptr;

        AZ_INLINE MethodOutputConfig(Nodes::Core::Method* method, const MethodConfiguration& config)
            : methodNode(method)
            , config(config)
        {}
    };

    namespace MethodHelper
    {
        AZStd::string GetArgumentName(size_t argIndex, const AZ::BehaviorMethod& method, const AZ::BehaviorClass* bcClass, AZStd::string_view replaceTypeName = "");

        AZStd::pair<AZStd::string, AZStd::string> GetArgumentNameAndToolTip(const MethodConfiguration& config, size_t argumentIndex);

        void AddMethodOutputSlot(const MethodOutputConfig& outputConfig);

        void SetSlotToDefaultValue(Node& node, const SlotId& slotId, const MethodConfiguration& config, size_t argumentIndex);

        DataSlotConfiguration ToInputSlotConfig(const MethodConfiguration& config, size_t argumentIndex);
    }

}
