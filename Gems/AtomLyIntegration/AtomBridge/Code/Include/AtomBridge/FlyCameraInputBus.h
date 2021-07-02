/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    namespace AtomBridge
    {
        /// This bus is used to enable and disable the FlyCamera so that input can be used for UI etc.
        class FlyCameraInputInterface
            : public AZ::ComponentBus
        {
        public:
            virtual void SetIsEnabled(bool isEnabled) = 0;
            virtual bool GetIsEnabled() = 0;

        public: // static member data

            //! Only one component on a entity can implement the events
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        };

        typedef AZ::EBus<FlyCameraInputInterface> FlyCameraInputBus;
    }
}
