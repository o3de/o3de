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
    return {"opacity.mode", "opacity.alphaSource", "opacity.textureMap"}
end
 
OpacityMode_Opaque = 0
OpacityMode_Cutout = 1
OpacityMode_Blended = 2
OpacityMode_TintedTransparent = 3

AlphaSource_Packed = 0
AlphaSource_Split = 1
AlphaSource_None = 2

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
    local opacityMode = context:GetMaterialPropertyValue_enum("opacity.mode")

    local forwardPassEDS = context:GetShaderByTag("ForwardPass_EDS")

    if(opacityMode == OpacityMode_Blended) then
        ConfigureAlphaBlending(forwardPassEDS)
        forwardPassEDS:SetDrawListTagOverride("transparent")
    elseif(opacityMode == OpacityMode_TintedTransparent) then
        ConfigureDualSourceBlending(forwardPassEDS)
        forwardPassEDS:SetDrawListTagOverride("transparent")
    else
        ResetAlphaBlending(forwardPassEDS)
        forwardPassEDS:SetDrawListTagOverride("") -- reset to default draw list
    end
end

function ProcessEditor(context)
    local opacityMode = context:GetMaterialPropertyValue_enum("opacity.mode")
    
    local mainVisibility
    if(opacityMode == OpacityMode_Opaque) then
        mainVisibility = MaterialPropertyVisibility_Hidden
    else
        mainVisibility = MaterialPropertyVisibility_Enabled
    end
    
    context:SetMaterialPropertyVisibility("opacity.alphaSource", mainVisibility)
    context:SetMaterialPropertyVisibility("opacity.textureMap", mainVisibility)
    context:SetMaterialPropertyVisibility("opacity.textureMapUv", mainVisibility)
    context:SetMaterialPropertyVisibility("opacity.factor", mainVisibility)

    if(opacityMode == OpacityMode_Blended or opacityMode == OpacityMode_TintedTransparent) then
        context:SetMaterialPropertyVisibility("opacity.alphaAffectsSpecular", MaterialPropertyVisibility_Enabled)
    else
        context:SetMaterialPropertyVisibility("opacity.alphaAffectsSpecular", MaterialPropertyVisibility_Hidden)
    end

    if(mainVisibility == MaterialPropertyVisibility_Enabled) then
        local alphaSource = context:GetMaterialPropertyValue_enum("opacity.alphaSource")

        if(alphaSource ~= AlphaSource_Split) then
            context:SetMaterialPropertyVisibility("opacity.textureMap", MaterialPropertyVisibility_Hidden)
            context:SetMaterialPropertyVisibility("opacity.textureMapUv", MaterialPropertyVisibility_Hidden)
        else
            local textureMap = context:GetMaterialPropertyValue_Image("opacity.textureMap")

            if(nil == textureMap) then
                context:SetMaterialPropertyVisibility("opacity.textureMapUv", MaterialPropertyVisibility_Disabled)
                context:SetMaterialPropertyVisibility("opacity.factor", MaterialPropertyVisibility_Disabled)
            end
        end
    end
end
