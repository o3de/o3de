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