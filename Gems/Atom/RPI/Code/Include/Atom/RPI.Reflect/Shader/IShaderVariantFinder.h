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

#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderVariantTreeAsset;
        class ShaderVariantAsset;

        //! This is the AZ::Interface<> declaration for the singleton responsible
        //! for finding the best ShaderVariantAsset a shader can use.
        class IShaderVariantFinder
        {
        public:
            AZ_TYPE_INFO(IShaderVariantFinder, "{B39F7407-BA4E-4F2F-810A-8B57D8080A04}");

            static constexpr const char* LogName = "IShaderVariantFinder";

            virtual ~IShaderVariantFinder() = default;

            //! Given the AssetId of a ShaderAsset it will try to find and load its corresponding ShaderVariantTreeAsset from
            //! the asset cache. If found, the asset will be loaded asynchronously and the caller will be notified via
            //! ShaderVariantFinderNotificationBus on main thread when the ShaderVariantTreeAsset is fully loaded.
            //! It is possible the requested ShaderVariantTreeAsset will never come into existence and in such
            //! case the caller will NEVER be notified.
            //! Returns true if the request was queued successfully.
            virtual bool LoadShaderVariantTreeAsset(const Data::AssetId& shaderAssetId) = 0;

            //! This is a quick blocking call that will return a valid asset only if i's been fully loaded already,
            //! Otherwise it returns an invalid asset and the caller is supposed to call LoadShaderVariantTreeAsset().
            virtual Data::Asset<ShaderVariantTreeAsset> GetShaderVariantTreeAsset(const Data::AssetId& shaderAssetId) = 0;

            //! Given the AssetId of a ShaderVariantTreeAsset and the stable id of a ShaderVariantAsset it will try to
            //! find its corresponding ShaderVariantAsset from the asset cache. If found, the asset will be loaded
            //! asynchronously and the caller will be notified via ShaderVariantFinderNotificationBus on main thread when the
            //! ShaderVariantAsset is fully loaded.
            //! Returns true if the request was queued successfully.
            virtual bool LoadShaderVariantAsset(const Data::AssetId& shaderVariantTreeAssetId, ShaderVariantStableId variantStableId) = 0;

            //! This is a quick blocking call that will return a valid asset only if i's been fully loaded already,
            //! Otherwise it returns an invalid asset and the caller is supposed to call LoadShaderVariantAsset().
            virtual Data::Asset<ShaderVariantAsset> GetShaderVariantAsset(const Data::AssetId& shaderVariantTreeAssetId, ShaderVariantStableId variantStableId) = 0;

            //! Clears the cache of loaded ShaderVariantTreeAsset and ShaderVariantAsset objects.
            //! This is intended for testing.
            virtual void Reset() = 0;
        };

        //! IShaderVariantFinder will call on this notification bus on the main thread.
        class ShaderVariantFinderNotification
            : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef Data::AssetId BusIdType; // The AssetId of the shader asset.
            //////////////////////////////////////////////////////////////////////////

            virtual void OnShaderVariantTreeAssetReady(Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset, bool isError) = 0;
            virtual void OnShaderVariantAssetReady(Data::Asset<ShaderVariantAsset> shaderVariantAsset, bool isError) = 0;
        };
        using ShaderVariantFinderNotificationBus = AZ::EBus<ShaderVariantFinderNotification>;

    } // namespace RPI
}// namespace AZ
