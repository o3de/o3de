--------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

function GetMaterialPropertyDependencies()
    return {"general.opacity_mode"}
end
 
OpacityMode_Opaque = 0
OpacityMode_Cutout = 1
OpacityMode_Blended = 2
OpacityMode_TintedTransparent = 3

function ConfigureAlphaBlending(shaderItem) 
    shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
    shaderItem:GetRenderStatesOverride():SetDepthWriteMask(DepthWriteMask_Zero)
    shaderItem:GetRenderStatesOverride():SetBlendEnabled(0, true)
    shaderItem:GetRenderStatesOverride():SetBlendSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendDest(0, BlendFactor_AlphaSourceInverse)
    shaderItem:GetRenderStatesOverride():SetBlendOp(0, BlendOp_Add)
end

function ConfigureDualSourceBlending(shaderItem)
    -- This blend multiplies the dest against color source 1, then adds color source 0.
    shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
    shaderItem:GetRenderStatesOverride():SetDepthWriteMask(DepthWriteMask_Zero)
    shaderItem:GetRenderStatesOverride():SetBlendEnabled(0, true)
    shaderItem:GetRenderStatesOverride():SetBlendSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendDest(0, BlendFactor_ColorSource1)
    shaderItem:GetRenderStatesOverride():SetBlendOp(0, BlendOp_Add)
end

function ResetAlphaBlending(shaderItem)
    shaderItem:GetRenderStatesOverride():ClearDepthEnabled()
    shaderItem:GetRenderStatesOverride():ClearDepthWriteMask()
    shaderItem:GetRenderStatesOverride():ClearBlendEnabled(0)
    shaderItem:GetRenderStatesOverride():ClearBlendSource(0)
    shaderItem:GetRenderStatesOverride():ClearBlendDest(0)
    shaderItem:GetRenderStatesOverride():ClearBlendOp(0)
end

function Process(context)
    local opacityMode = OpacityMode_Opaque
    if context:HasMaterialProperty("general.opacity_mode") then
        opacityMode = context:GetMaterialPropertyValue_enum("general.opacity_mode")
    end

    local forwardPassEDS = context:GetShaderByTag("ForwardPass_EDS")
    if forwardPassEDS then
        if opacityMode == OpacityMode_Blended then
            ConfigureAlphaBlending(forwardPassEDS)
            forwardPassEDS:SetDrawListTagOverride("transparent")
        elseif opacityMode == OpacityMode_TintedTransparent then
            ConfigureDualSourceBlending(forwardPassEDS)
            forwardPassEDS:SetDrawListTagOverride("transparent")
        else
            ResetAlphaBlending(forwardPassEDS)
            forwardPassEDS:SetDrawListTagOverride("") -- reset to default draw list
        end
    end
end