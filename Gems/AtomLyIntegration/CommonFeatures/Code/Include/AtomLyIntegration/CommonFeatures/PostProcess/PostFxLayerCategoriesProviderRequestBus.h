/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <Atom/Feature/PostProcess/PostFxLayerCategoriesConstants.h>
/**
* the EBus is used to request registered tags
*/

namespace AZ
{
    namespace Render
    {
        class PostFxLayerCategoriesProviderRequests
            : public AZ::EBusTraits
        {
        public:
            // EBusTraits
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            //! allows multiple threads to call
            using MutexType = AZStd::recursive_mutex;

            virtual void GetLayerCategories(PostFx::LayerCategoriesMap& layerCategories) const = 0;
        };

        typedef AZ::EBus<PostFxLayerCategoriesProviderRequests> PostFxLayerCategoriesProviderRequestBus;
    }
}
