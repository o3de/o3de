/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace RPI
    {
        class PassTemplate;

        //! Notifications about pass templates in the Pass System.
        class PassSystemTemplateNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            using BusIdType = AZ::Name;
            using EventQueueMutexType = AZStd::mutex;

            //! Notifies a pass template is being added to the Pass System (also triggers when reloading a pass template).
            //! Receivers of this call can modify the pass template if needed (add or remove slots, attachments, etc).
            virtual void OnAddingPassTemplate(const AZStd::shared_ptr<PassTemplate>& passTemplate){AZ_UNUSED(passTemplate);}
        };

        using PassSystemTemplateNotificationsBus = AZ::EBus<PassSystemTemplateNotifications>;       
    } // namespace RPI
} // namespace AZ
