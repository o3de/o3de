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
#include <AzCore/Asset/AssetCommon.h>
#include <smartptr.h>
#include <IMaterial.h>

namespace LmbrCentral
{
    /*!
     * Material asset type configuration.
     * Reflect as: AzFramework::SimpleAssetReference<MaterialAsset>
     */
    class MaterialAsset
    {
    public:
        AZ_TYPE_INFO(MaterialAsset, "{F46985B5-F7FF-4FCB-8E8C-DC240D701841}")
        static const char* GetFileFilter() {
            return "*.mtl";
        }
    };

    /*!
    * Source scene file Material asset type configuration.
    * Reflect as: AzFramework::SimpleAssetReference<DccMaterialAsset>
    */
    class DccMaterialAsset
    {
    public:
        AZ_TYPE_INFO(DccMaterialAsset, "{C88469CF-21E7-41EB-96FD-BF14FBB05EDC}")
            static const char* GetFileFilter() {
            return "*.dccmtl";
        }
    };

    /*!
     * Texture asset type configuration.
     * Reflect as: AzFramework::SimpleAssetReference<TextureAsset>
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
