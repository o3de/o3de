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

namespace AZ
{
    namespace Render
    {
        //! Components that own and update a HairRenderObject should inherit from the
        //!   HairFeatureProcessorNotificationBus to receive the bone transforms update event
        
        [To Do] Adi: Make sure this is connected to retrieve the bone matrices for the Hair. 
        class HairFeatureProcessorNotifications
            : public AZ::EBusTraits
        {
        public :
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            virtual void OnUpdateSkinningMatrices() = 0;
        };
        using HairFeatureProcessorNotificationBus = AZ::EBus<HairFeatureProcessorNotifications>;
    } // namespace Render
} // namespace AZ
