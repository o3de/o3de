/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace Render
    {
        //! Components that own and update a SkinnedMeshRenderProxy should inherit from the SkinnedMeshFeatureProcessorNotificationBus to receive an event telling them when to update bone transforms
        class SkinnedMeshFeatureProcessorNotifications
            : public AZ::EBusTraits
        {
        public :
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            virtual void OnUpdateSkinningMatrices() = 0;
        };
        using SkinnedMeshFeatureProcessorNotificationBus = AZ::EBus<SkinnedMeshFeatureProcessorNotifications>;
    } // namespace Render
} // namespace AZ
