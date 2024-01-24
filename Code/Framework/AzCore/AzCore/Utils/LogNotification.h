/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Gruber patch begin // AE -- update log while waiting for assets - the whole file

#include <AzCore/PlatformDef.h>
#include <AzCore/base.h>

namespace AZ
{
    namespace LogNotification
    {
        /*
        * Log notification to update queued log messages
        */
        class LogNotificator : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            /// Update log messages
            virtual void Update()
            {
            }
        };
        typedef EBus<LogNotificator> LogNotificatorBus;
    }
}

// Gruber patch end // AE -- update log while waiting for assets - the whole file
