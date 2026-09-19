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
    return {"mode", "alphaSource", "textureMap", "edgeSoftness", "alphaToCoverage"}
end
 
OpacityMode_Opaque = 0
OpacityMode_Cutout = 1
OpacityMode_Blended = 2
OpacityMode_TintedTransparent = 3

AlphaSource_Packed = 0
AlphaSource_Split = 1
AlphaSource_None = 2

function Process(context)

    local opacityMode = OpacityMode_Opaque
    if context:HasMaterialProperty("mode") then
        opacityMode = context:GetMaterialPropertyValue_enum("mode")
    end

    local edgeSoftness = 0.0
    if context:HasMaterialProperty("edgeSoftness") then
        edgeSoftness = context:GetMaterialPropertyValue_float("edgeSoftness")
    end

    local alphaToCoverage = false
    if context:HasMaterialProperty("alphaToCoverage") then
        alphaToCoverage = context:GetMaterialPropertyValue_bool("alphaToCoverage")
    end

    -- When edgeSoftness > 0 and mode is Cutout, switch to the transparent (blended) pipeline
    -- so the smoothed alpha can blend with the background for anti-aliased cutout edges.
    -- hasPerPixelClip is false in this case because clipping is handled by the shader's
    -- smoothstep + clip logic, not the per-pixel clip flag that switches to customZ shaders.
    local useSoftEdge = (opacityMode == OpacityMode_Cutout and edgeSoftness > 0.0)

    local useA2C = (opacityMode == OpacityMode_Cutout and alphaToCoverage and not useSoftEdge)

    context:SetInternalMaterialPropertyValue_bool("hasPerPixelClip", opacityMode == OpacityMode_Cutout and not useSoftEdge)
    context:SetInternalMaterialPropertyValue_bool("isTransparent", opacityMode == OpacityMode_Blended or useSoftEdge)
    context:SetInternalMaterialPropertyValue_bool("isTintedTransparent", opacityMode == OpacityMode_TintedTransparent)
    context:SetInternalMaterialPropertyValue_bool("useAlphaToCoverage", useA2C)
end

function ProcessEditor(context)
    local opacityMode = context:GetMaterialPropertyValue_enum("mode")
    
    local mainVisibility
    if(opacityMode == OpacityMode_Opaque) then
        mainVisibility = MaterialPropertyVisibility_Hidden
    else
        mainVisibility = MaterialPropertyVisibility_Enabled
    end
    
    context:SetMaterialPropertyVisibility("alphaSource", mainVisibility)
    context:SetMaterialPropertyVisibility("textureMap", mainVisibility)
    context:SetMaterialPropertyVisibility("textureMapUv", mainVisibility)
    context:SetMaterialPropertyVisibility("factor", mainVisibility)

    if(opacityMode == OpacityMode_Cutout) then
        context:SetMaterialPropertyVisibility("edgeSoftness", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("alphaToCoverage", MaterialPropertyVisibility_Enabled)
    else
        context:SetMaterialPropertyVisibility("edgeSoftness", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("alphaToCoverage", MaterialPropertyVisibility_Hidden)
    end

    if(opacityMode == OpacityMode_Blended or opacityMode == OpacityMode_TintedTransparent) then
        context:SetMaterialPropertyVisibility("alphaAffectsSpecular", MaterialPropertyVisibility_Enabled)
    else
        context:SetMaterialPropertyVisibility("alphaAffectsSpecular", MaterialPropertyVisibility_Hidden)
    end

    if(mainVisibility == MaterialPropertyVisibility_Enabled) then
        local alphaSource = context:GetMaterialPropertyValue_enum("alphaSource")

        if (opacityMode == OpacityMode_Cutout and alphaSource == AlphaSource_None) then
            context:SetMaterialPropertyVisibility("factor", MaterialPropertyVisibility_Hidden)
        end

        if(alphaSource ~= AlphaSource_Split) then
            context:SetMaterialPropertyVisibility("textureMap", MaterialPropertyVisibility_Hidden)
            context:SetMaterialPropertyVisibility("textureMapUv", MaterialPropertyVisibility_Hidden)
        else
            local textureMap = context:GetMaterialPropertyValue_Image("textureMap")

            if(nil == textureMap) then
                context:SetMaterialPropertyVisibility("textureMapUv", MaterialPropertyVisibility_Disabled)
                context:SetMaterialPropertyVisibility("factor", MaterialPropertyVisibility_Disabled)
            end
        end
    end
end
