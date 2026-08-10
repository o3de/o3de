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
    return {"mode", "alphaSource", "textureMap", "factor", "edgeSoftness", "alphaToCoverage"}
end

function GetShaderOptionDependencies()
    return {"o_alphaToCoverage", "o_alphaToCoverageUseSharpness"}
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

    -- Anti-aliased cutout edges are rendered with alpha-to-coverage (A2C). The two edge
    -- controls are mutually exclusive:
    --  - alphaToCoverage checkbox ON:  the alphaToCoverageSharpness ramp defines the edge
    --    band; edgeSoftness is ignored (o_alphaToCoverageUseSharpness = true).
    --  - alphaToCoverage checkbox OFF: edgeSoftness > 0 implicitly enables A2C and defines
    --    the band; sharpness is ignored.
    -- The o_alphaToCoverage shader option and the A2C render state (applied by
    -- Materials/Pipelines/Common/AlphaToCoverage.lua via the useAlphaToCoverage internal
    -- property) are both driven from this single value so they can never disagree.
    -- A mismatch makes the forward pass output alpha -1 while the hardware derives
    -- coverage from it, masking out every sample and leaving depth-only pixels that
    -- render as black.
    local useCutout = (opacityMode == OpacityMode_Cutout)
    local useA2C = (useCutout and (alphaToCoverage or edgeSoftness > 0.0))

    context:SetShaderOptionValue_bool("o_alphaToCoverage", useA2C)
    context:SetShaderOptionValue_bool("o_alphaToCoverageUseSharpness", alphaToCoverage)
    context:SetInternalMaterialPropertyValue_bool("hasPerPixelClip", useCutout)
    context:SetInternalMaterialPropertyValue_bool("isTransparent", opacityMode == OpacityMode_Blended)
    context:SetInternalMaterialPropertyValue_bool("isTintedTransparent", opacityMode == OpacityMode_TintedTransparent)
    context:SetInternalMaterialPropertyValue_bool("useAlphaToCoverage", useA2C)
end

function ProcessEditor(context)
    local opacityMode = context:GetMaterialPropertyValue_enum("mode")

    local alphaToCoverage = false
    if context:HasMaterialProperty("alphaToCoverage") then
        alphaToCoverage = context:GetMaterialPropertyValue_bool("alphaToCoverage")
    end
    
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
        -- The two edge controls are mutually exclusive:
        --  - Alpha to Coverage ON:  sharpness ramp is active, edgeSoftness greyed out (and
        --    ignored by the shader via o_alphaToCoverageUseSharpness).
        --  - Alpha to Coverage OFF: edgeSoftness is active, sharpness greyed out.
        if(alphaToCoverage) then
            context:SetMaterialPropertyVisibility("edgeSoftness", MaterialPropertyVisibility_Disabled)
            context:SetMaterialPropertyVisibility("alphaToCoverageSharpness", MaterialPropertyVisibility_Enabled)
        else
            context:SetMaterialPropertyVisibility("edgeSoftness", MaterialPropertyVisibility_Enabled)
            context:SetMaterialPropertyVisibility("alphaToCoverageSharpness", MaterialPropertyVisibility_Disabled)
        end
        context:SetMaterialPropertyVisibility("alphaToCoverage", MaterialPropertyVisibility_Enabled)
    else
        context:SetMaterialPropertyVisibility("edgeSoftness", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("alphaToCoverage", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("alphaToCoverageSharpness", MaterialPropertyVisibility_Hidden)
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
