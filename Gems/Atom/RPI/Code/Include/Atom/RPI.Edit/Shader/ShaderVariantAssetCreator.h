/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>

namespace AZ
{
    namespace RPI
    {
        //! The "builder" pattern class that creates a ShaderVariantAsset.
        class ShaderVariantAssetCreator final
            : public AssetCreator<ShaderVariantAsset>
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
            void Begin(const AZ::Data::AssetId& assetId, const ShaderVariantId& shaderVariantId, RPI::ShaderVariantStableId stableId, const ShaderOptionGroupLayout* shaderOptionGroupLayout);

            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<ShaderVariantAsset>& result);

            /////////////////////////////////////////////////////////////////////
            // Methods for all shader variant types

            //! Set the timestamp value of ShaderAsset that matches this ShaderVariantAsset.
            //! This is needed to synchronize between the ShaderAsset and ShaderVariantAsset when hot-reloading shaders.
            void SetShaderAssetBuildTimestamp(AZStd::sys_time_t shaderAssetBuildTimestamp);

            //! Assigns a shaderStageFunction, which contains the byte code, to the slot dictated by the shader stage.
            void SetShaderFunction(RHI::ShaderStage shaderStage, RHI::Ptr<RHI::ShaderStageFunction> shaderStageFunction);
            
            /////////////////////////////////////////////////////////////////////
            // Methods for PipelineStateType::Draw variants.
            
            //! Assigns the contract for inputs required by the shader.
            void SetInputContract(const ShaderInputContract& contract);
            
            //! Assigns the contract for outputs required by the shader.
            void SetOutputContract(const ShaderOutputContract& contract);
            
            //! Assigns the render states for the draw pipeline. Ignored for non-draw pipelines.
            void SetRenderStates(const RHI::RenderStates& renderStates);

        };
    } // namespace RPI
} // namespace AZ
