/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Asset/SimpleAsset.h>

#include "TextureAtlas/TextureAtlas.h"


namespace TextureAtlasNamespace
{

    class TextureAtlasNotifications : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
        //! Listener for new atlases being loaded
        virtual void OnAtlasLoaded(const TextureAtlas* atlas) = 0;

        //! Notify that an atlas is being unloaded
        virtual void OnAtlasUnloaded(const TextureAtlas* atlas) = 0;

        //! Notify that an atlas is being reloaded
        virtual void OnAtlasReloaded(const TextureAtlas* atlas)
        {
            OnAtlasUnloaded(atlas); 
            OnAtlasLoaded(atlas);
        };
    };
    using TextureAtlasNotificationBus = AZ::EBus<TextureAtlasNotifications>;
} // namespace TextureAtlasNamespace
