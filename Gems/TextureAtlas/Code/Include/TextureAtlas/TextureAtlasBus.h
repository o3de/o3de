/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    using AtlasCoordinateSets = AZStd::vector<AZStd::pair<AZStd::string, AtlasCoordinates>>;

    class TextureAtlasRequests : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
        //! Saves a texture atlas to file
        virtual void SaveAtlasToFile(const AZStd::string& outputPath,
            AtlasCoordinateSets& handles, int width, int height) = 0;

        //! Tells the TextureAtlas system to load an Atlas and return a pointer for the atlas
        virtual TextureAtlas* LoadAtlas(const AZStd::string& filePath) = 0;

        //! Tells the TextureAtlas system to unload an Atlas
        virtual void UnloadAtlas(TextureAtlas* atlas) = 0;

        //! Returns a pointer to the first Atlas that contains the image, or nullptr if no atlas contains it. 
        //! Does not add a reference, use the notification bus to know when to unload
        virtual TextureAtlas* FindAtlasContainingImage(const AZStd::string& filePath) = 0;

    };
    using TextureAtlasRequestBus = AZ::EBus<TextureAtlasRequests>;

    class TextureAtlasAsset
    {
    public:
        AZ_TYPE_INFO(TextureAtlasAsset, "{BFC6C91F-66CE-4D78-B68A-7F697C9EA2E8}")
            static const char* GetFileFilter() { return "*.texatlas"; }
    };
} // namespace TextureAtlasNamespace
