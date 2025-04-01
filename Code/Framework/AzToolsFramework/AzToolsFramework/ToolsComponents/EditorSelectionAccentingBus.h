/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    namespace Components
    {

        //////////////////////////////////////////////////////////////////////////
        // Applies selection accents to all selected entities
        //////////////////////////////////////////////////////////////////////////
        class EditorSelectionAccentingRequests
            : public AZ::EBusTraits
        {
        public:

            //////////////////////////////////////////////////////////////////////////
            // Bus configuration
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; 
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            /**
            * \brief Invalidates then Recalculates and Applies accenting on the currently selected set of entities
            *        This shouldn't be necessary to call except in unique circumstances, as it is automatically done 
            *        on highlight and selection changes.
            */
            virtual void ForceSelectionAccentRefresh() = 0;
        };

        using EditorSelectionAccentingRequestBus = AZ::EBus<EditorSelectionAccentingRequests>;
    } // namespace Components
} // namespace AzToolsFramework

DECLARE_EBUS_EXTERN(AzToolsFramework::Components::EditorSelectionAccentingRequests);
