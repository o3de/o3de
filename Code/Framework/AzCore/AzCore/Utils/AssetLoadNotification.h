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
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace AssetLoadNotification
    {
        /*
        * AssetLoad notification to update loading screen stuff
        */
        class AssetLoadNotificator : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            //////////////////////////////////////////////////////////////////////////

            /// Update loading screen stuff
            virtual void WaitForAssetUpdate() {}
        };
        typedef EBus<AssetLoadNotificator> AssetLoadNotificatorBus;
    }
}
#endif // defined (CARBONATED)
