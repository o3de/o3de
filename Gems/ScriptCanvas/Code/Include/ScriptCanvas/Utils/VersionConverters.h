/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvas
{
    class VersionConverters
    {
    public:
        static bool ContainsStringVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool DelayVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool DurationVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool EBusEventHandlerVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool ForEachVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool FunctionNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool HeartBeatVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool InputNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool LerpBetweenVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool OnceNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool ReceiveScriptEventVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool RepeaterVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool TickDelayVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool TimeDelayVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool TimerVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);
        static bool ScriptEventBaseVersionConverter(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode& rootElement);


    private:
        static bool MarkSlotAsLatent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement, const AZStd::vector<AZStd::string>& startWith);
    };
}
