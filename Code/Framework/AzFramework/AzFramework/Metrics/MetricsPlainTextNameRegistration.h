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

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>

namespace AzFramework
{
    // Bus for things to register their UUIDs if they want their naked, raw name sent back
    // to Amazon for metrics. Only Amazon created components/classes/UUIDs should register with this
    class MetricsPlainTextNameRegistrationBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual ~MetricsPlainTextNameRegistrationBusTraits() {}

        virtual void RegisterForNameSending(const AZStd::vector<AZ::Uuid>& /* typeIdsThatCanBeSentAsPlainText */) {}

        virtual bool IsTypeRegisteredForNameSending(const AZ::Uuid& /* typeId */) { return false; }
    };
    using MetricsPlainTextNameRegistrationBus = AZ::EBus<MetricsPlainTextNameRegistrationBusTraits>;

} // namespace AzFramework


