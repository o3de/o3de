/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/EBus/EBus.h>

namespace OpenParticleSystemEditor
{
    class ParticleGraphicsViewRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = AZStd::string;

        virtual AZStd::vector<AZStd::string> GetItemNamesByOrder() = 0;
        virtual void SetCheckedSolo(AZStd::string& name, bool except, bool checked) = 0;
        virtual void SetChecked(AZStd::string& name, bool except, bool checked) = 0;
        virtual void CheckAllParticleItemWidgetWithoutBusNotification() = 0;
        virtual void RestoreAllParticleItemWidgetStatusAfterCheckAll() = 0;
    };

    using ParticleGraphicsViewRequestsBus = AZ::EBus<ParticleGraphicsViewRequests>;
} // namespace OpenParticleSystemEditor
