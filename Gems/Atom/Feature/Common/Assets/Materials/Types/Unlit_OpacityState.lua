--------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
----------------------------------------------------------------------------------------------------

function GetMaterialPropertyDependencies()
    return {"mode", "alphaSource", "textureMap", "useTexture"}
end

OpacityMode_Opaque = 0
OpacityMode_Cutout = 1
OpacityMode_Blended = 2

AlphaSource_Packed = 0
AlphaSource_Split = 1
AlphaSource_None = 2

function GetShaderOptionDependencies()
    return {"o_opacity_useTexture"}
end

function Process(context)
    local opacityMode = context:GetMaterialPropertyValue_enum("mode")
    local alphaSource = context:GetMaterialPropertyValue_enum("alphaSource")
    local opacityTexture = context:GetMaterialPropertyValue_Image("textureMap")
    local useOpacityTexture = context:GetMaterialPropertyValue_bool("useTexture")

    local enableOpacityTexture = opacityMode ~= OpacityMode_Opaque
        and alphaSource == AlphaSource_Split
        and opacityTexture ~= nil
        and useOpacityTexture

    context:SetShaderOptionValue_bool("o_opacity_useTexture", enableOpacityTexture)

    local isBlended = opacityMode == OpacityMode_Blended

    if context:HasShaderWithTag("forward") then
        local shader = context:GetShaderByTag("forward")
        if shader then
            shader:SetEnabled(not isBlended)
        end
    end

    if context:HasShaderWithTag("depth") then
        local shader = context:GetShaderByTag("depth")
        if shader then
            shader:SetEnabled(not isBlended)
        end
    end

    if context:HasShaderWithTag("transparent") then
        local shader = context:GetShaderByTag("transparent")
        if shader then
            shader:SetEnabled(isBlended)
        end
    end
end

function ProcessEditor(context)
    local opacityMode = context:GetMaterialPropertyValue_enum("mode")
    local alphaSource = context:GetMaterialPropertyValue_enum("alphaSource")
    local opacityTexture = context:GetMaterialPropertyValue_Image("textureMap")
    local useOpacityTexture = context:GetMaterialPropertyValue_bool("useTexture")

    local showOpacitySettings = opacityMode ~= OpacityMode_Opaque
    local mainVisibility = showOpacitySettings and MaterialPropertyVisibility_Enabled or MaterialPropertyVisibility_Hidden

    context:SetMaterialPropertyVisibility("alphaSource", mainVisibility)
    context:SetMaterialPropertyVisibility("textureMap", MaterialPropertyVisibility_Hidden)
    context:SetMaterialPropertyVisibility("useTexture", MaterialPropertyVisibility_Hidden)
    context:SetMaterialPropertyVisibility("textureMapUv", MaterialPropertyVisibility_Hidden)
    context:SetMaterialPropertyVisibility("factor", MaterialPropertyVisibility_Hidden)

    if not showOpacitySettings then
        return
    end

    if opacityMode == OpacityMode_Cutout and alphaSource == AlphaSource_None then
        return
    end

    context:SetMaterialPropertyVisibility("factor", MaterialPropertyVisibility_Enabled)

    if alphaSource ~= AlphaSource_Split then
        return
    end

    context:SetMaterialPropertyVisibility("textureMap", MaterialPropertyVisibility_Enabled)

    if opacityTexture == nil then
        return
    end

    context:SetMaterialPropertyVisibility("useTexture", MaterialPropertyVisibility_Enabled)

    if useOpacityTexture then
        context:SetMaterialPropertyVisibility("textureMapUv", MaterialPropertyVisibility_Enabled)
    else
        context:SetMaterialPropertyVisibility("textureMapUv", MaterialPropertyVisibility_Disabled)
    end
end