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
#include <AzCore/Asset/AssetCommon.h>

//#include <Atom/RPI.Reflect/Shader/ShaderCommonTypes.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

namespace AZ
{
    namespace RPI
    {
        class Shader2;
        class ShaderAsset2;

        /**
         * Connect to this EBus to get notifications whenever a Data::Instance<Shader> reloads its ShaderAsset.
         * The bus address is the AssetId of the ShaderAsset.
         */
        class ShaderReloadNotifications2
            : public EBusTraits
        {

        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef Data::AssetId                 BusIdType;
            //////////////////////////////////////////////////////////////////////////

            virtual ~ShaderReloadNotifications2() {}

            //! Called when the ShaderAsset reinitializes itself in response to another asset being reloaded.
            virtual void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset2>& shaderAsset) { AZ_UNUSED(shaderAsset); }

            //! Called when the Shader instance reinitializes itself in response to the ShaderAsset being reloaded.
            virtual void OnShaderReinitialized(const Shader2& shader) { AZ_UNUSED(shader); }

            //! Called when a particular shader variant is reinitialized.
            virtual void OnShaderVariantReinitialized(const Shader2& shader, const ShaderVariantId& shaderVariantId, ShaderVariantStableId shaderVariantStableId)
            { AZ_UNUSED(shader); AZ_UNUSED(shaderVariantId); AZ_UNUSED(shaderVariantStableId) }
        };

        typedef EBus<ShaderReloadNotifications2> ShaderReloadNotificationBus2;

    } // namespace RPI
} //namespace AZ
