/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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


