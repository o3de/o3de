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
