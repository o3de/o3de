/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined (CARBONATED)

#include <AzCore/PlatformDef.h>
#include <AzCore/base.h>
#include <AzCore/EBus/Ebus.h>

namespace AZ
{
    namespace AssetLoadNotification
    {
        /*
        * Log notification to update queued log messages
        */
        class AssetLoadNotificator : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            /// Update log messages
            virtual void WaitForAssetUpdate()
            {
            }
        };
        typedef EBus<AssetLoadNotificator> AssetLoadNotificatorBus;
    }
}
#endif // defined (CARBONATED)
