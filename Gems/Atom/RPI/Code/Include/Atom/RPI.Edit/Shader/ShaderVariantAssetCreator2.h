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

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset2.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>

namespace AZ
{
    namespace RPI
    {
        //! The "builder" pattern class that creates a ShaderVariantAsset2.
        class ShaderVariantAssetCreator2 final
            : public AssetCreator<ShaderVariantAsset2>
        {
        public:
            //! Begins construction of the shader variant asset.
            //! @param assetId The "initial" assetId that the resulting ShaderVariantAsset will get.
            //!        "initial" was quoted because in the end the asset processor will assign another assetId
            //!        because on the UUID of the source asset (a *.shadervariantlist file) and the product subid
            //!        that gets assign when returning the Job Response.
            //!        It is still useful, because when creating the Root Variant for the ShaderAsset this assetId should
            //!        match the value that will be assigned by the asset processor because the Root Variant is serialized
            //!        as a Data::Asset<ShaderVariantAsset> inside the ShaderAsset.
            void Begin(const AZ::Data::AssetId& assetId, const ShaderVariantId& shaderVariantId, RPI::ShaderVariantStableId stableId, bool isFullyBaked);

            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<ShaderVariantAsset2>& result);

            /////////////////////////////////////////////////////////////////////
            // Methods for all shader variant types

            //! Set the timestamp value of ShaderAsset that matches this ShaderVariantAsset.
            //! This is needed to synchronize between the ShaderAsset and ShaderVariantAsset when hot-reloading shaders.
            void SetBuildTimestamp(AZStd::sys_time_t buildTimestamp);

            //! Assigns a shaderStageFunction, which contains the byte code, to the slot dictated by the shader stage.
            void SetShaderFunction(RHI::ShaderStage shaderStage, RHI::Ptr<RHI::ShaderStageFunction> shaderStageFunction);

        };
    } // namespace RPI
} // namespace AZ
