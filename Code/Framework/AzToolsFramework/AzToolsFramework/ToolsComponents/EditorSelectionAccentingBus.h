/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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