/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Asset/SimpleAsset.h>

namespace LmbrCentral
{
    /*!
     * Texture asset type configuration.
     * Reflect as: AzFramework::SimpleAssetReference<TextureAsset>
     * Currently only used by LyShine for Sprites and will require a bit of
     * work to move it and TextureAssetInfo into LyShine without breaking
     * existing .uicanvas files 
     */
    class TextureAsset
    {
    public:
        AZ_TYPE_INFO(TextureAsset, "{59D5E20B-34DB-4D8E-B867-D33CC2556355}")
        static const char* GetFileFilter()
        {
            return "*.dds; *.tif; *.bmp; *.gif; *.jpg; *.jpeg; *.jpe; *.tga; *.png; *.swf; *.gfx; *.sfd; *.sprite";
        }
    };
} // namespace LmbrCentral
