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
    return { "blend.mode" }
end

BlendMode_Opaque = 0
BlendMode_Cutout = 1
BlendMode_Blended = 2
BlendMode_Additive = 3
BlendMode_Subtractive = 4
BlendMode_Modulate = 5

function ConfigureOpaqueBlending(shaderItem)
    shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
    shaderItem:GetRenderStatesOverride():SetDepthWriteMask(DepthWriteMask_Zero)
    shaderItem:GetRenderStatesOverride():SetBlendEnabled(0, false)
end

function ConfigureCutoutBlending(shaderItem)
    shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
    shaderItem:GetRenderStatesOverride():SetDepthComparisonFunc(6)
    shaderItem:GetRenderStatesOverride():SetBlendEnabled(0, true)
    shaderItem:GetRenderStatesOverride():SetBlendSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendDest(0, BlendFactor_AlphaSourceInverse)
    shaderItem:GetRenderStatesOverride():SetBlendOp(0, BlendOp_Add)
end

function ConfigureBlendedBlending(shaderItem)
    shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
    shaderItem:GetRenderStatesOverride():SetDepthComparisonFunc(6)
    shaderItem:GetRenderStatesOverride():SetBlendEnabled(0, true)
    shaderItem:GetRenderStatesOverride():SetBlendSource(0, BlendFactor_AlphaSource)
    shaderItem:GetRenderStatesOverride():SetBlendDest(0, BlendFactor_AlphaSourceInverse)
    shaderItem:GetRenderStatesOverride():SetBlendOp(0, BlendOp_Add)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaDest(0, BlendFactor_AlphaSourceInverse)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaOp(0, BlendOp_Add)
end

function ConfigureAdditiveBlending(shaderItem)
    shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
    shaderItem:GetRenderStatesOverride():SetDepthComparisonFunc(6)
    shaderItem:GetRenderStatesOverride():SetBlendEnabled(0, true)
    shaderItem:GetRenderStatesOverride():SetBlendSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendDest(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendOp(0, BlendOp_Add)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaDest(0, BlendFactor_AlphaSourceInverse)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaOp(0, BlendOp_Add)
end

function ConfigureSubtractiveBlending(shaderItem)
    shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
    shaderItem:GetRenderStatesOverride():SetDepthComparisonFunc(6)
    shaderItem:GetRenderStatesOverride():SetBlendEnabled(0, true)
    shaderItem:GetRenderStatesOverride():SetBlendSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendDest(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendOp(0, BlendOp_Subtract)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaDest(0, BlendFactor_AlphaSourceInverse)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaOp(0, BlendOp_Subtract)
end

function ConfigureModulateBlending(shaderItem)
    shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
    shaderItem:GetRenderStatesOverride():SetDepthComparisonFunc(6)
    shaderItem:GetRenderStatesOverride():SetBlendEnabled(0, true)
    shaderItem:GetRenderStatesOverride():SetBlendSource(0, BlendFactor_Zero)
    shaderItem:GetRenderStatesOverride():SetBlendDest(0, BlendFactor_ColorSource)
    shaderItem:GetRenderStatesOverride():SetBlendOp(0, BlendOp_Add)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaSource(0, BlendFactor_One)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaDest(0, BlendFactor_AlphaSourceInverse)
    shaderItem:GetRenderStatesOverride():SetBlendAlphaOp(0, BlendOp_Add)
end

function ResetAlphaBlending(shaderItem)
    shaderItem:GetRenderStatesOverride():ClearDepthEnabled()
    shaderItem:GetRenderStatesOverride():ClearDepthWriteMask()
    shaderItem:GetRenderStatesOverride():ClearBlendEnabled(0)
    shaderItem:GetRenderStatesOverride():ClearBlendSource(0)
    shaderItem:GetRenderStatesOverride():ClearBlendDest(0)
    shaderItem:GetRenderStatesOverride():ClearBlendOp(0)
    shaderItem:GetRenderStatesOverride():ClearBlendAlphaSource(0)
    shaderItem:GetRenderStatesOverride():ClearBlendAlphaDest(0)
    shaderItem:GetRenderStatesOverride():ClearBlendAlphaOp(0)
end

function Process(context)
    local blend = context:GetMaterialPropertyValue_enum("blend.mode")

    local forwardPass = context:GetShaderByTag("ForwardPass")

    if blend == BlendMode_Opaque then
        ConfigureOpaqueBlending(forwardPass)
        forwardPass:SetDrawListTagOverride("transparent")
    elseif (blend == BlendMode_Cutout) then
        ConfigureCutoutBlending(forwardPass)
        forwardPass:SetDrawListTagOverride("transparent")
    elseif (blend == BlendMode_Blended) then
        ConfigureBlendedBlending(forwardPass)
        forwardPass:SetDrawListTagOverride("transparent")
    elseif (blend == BlendMode_Additive) then
        ConfigureAdditiveBlending(forwardPass)
        forwardPass:SetDrawListTagOverride("transparent")
    elseif (blend == BlendMode_Subtractive) then
        ConfigureSubtractiveBlending(forwardPass)
        forwardPass:SetDrawListTagOverride("transparent")
    elseif (blend == BlendMode_Modulate) then
        ConfigureModulateBlending(forwardPass)
        forwardPass:SetDrawListTagOverride("transparent")
    else
        ResetAlphaBlending(forwardPass)
        forwardPass:SetDrawListTagOverride("") -- reset to default draw list
    end
end

function ProcessEditor(context)
    local blendMode = context:GetMaterialPropertyValue_enum("blend.mode")

    local mainVisibility
    if blendMode == BlendMode_Cutout then
        mainVisibility = MaterialPropertyVisibility_Enabled
    else
        mainVisibility = MaterialPropertyVisibility_Hidden
    end
    context:SetMaterialPropertyVisibility("blend.factor", mainVisibility)

end
