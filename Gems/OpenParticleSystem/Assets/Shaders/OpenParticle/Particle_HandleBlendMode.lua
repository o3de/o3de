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
    -- Support both old blend.* naming and new opacity.* naming
    return { "blend.mode", "opacity.mode" }
end

-- Blend mode enum values (must match the enumValues in the materialtype)
local BlendMode_Opaque      = 0
local BlendMode_Cutout      = 1
local BlendMode_Blended     = 2
local BlendMode_Additive    = 3
local BlendMode_Subtractive = 4
local BlendMode_Modulate    = 5

-- Opacity mode enum values from OpacityPropertyGroup.json
local OpacityMode_Opaque        = 0
local OpacityMode_Cutout        = 1
local OpacityMode_Blended       = 2
local OpacityMode_TintedTransparent = 3

-- Render state constants
local DepthComparisonFunc_GreaterEqual = 6  -- RHI::ComparisonFunc::GreaterEqual

--------------------------------------------------------------------------------
-- Blend state configuration helpers
--------------------------------------------------------------------------------

local function IsShaderItemValid(shaderItem)
    if not shaderItem then
        return false
    end
    -- Try to call a safe method to validate the shader item
    local success = pcall(function()
        shaderItem:SetEnabled(true)
    end)
    return success
end

local function SafeConfigureRenderStates(shaderItem, configureFn)
    if not IsShaderItemValid(shaderItem) then
        return
    end
    local success, renderStates = pcall(function()
        return shaderItem:GetRenderStatesOverride()
    end)
    if success and renderStates then
        configureFn(renderStates)
    end
end

local function ConfigureOpaqueBlending(shaderItem)
    SafeConfigureRenderStates(shaderItem, function(renderStates)
        renderStates:SetDepthEnabled(true)
        renderStates:SetDepthWriteMask(DepthWriteMask_All)
        renderStates:SetBlendEnabled(0, false)
    end)
end

local function ConfigureCutoutBlending(shaderItem)
    SafeConfigureRenderStates(shaderItem, function(renderStates)
        renderStates:SetDepthEnabled(true)
        renderStates:SetDepthComparisonFunc(DepthComparisonFunc_GreaterEqual)
        renderStates:SetBlendEnabled(0, true)
        renderStates:SetBlendSource(0, BlendFactor_One)
        renderStates:SetBlendDest(0, BlendFactor_AlphaSourceInverse)
        renderStates:SetBlendOp(0, BlendOp_Add)
    end)
end

local function ConfigureBlendedBlending(shaderItem)
    SafeConfigureRenderStates(shaderItem, function(renderStates)
        renderStates:SetDepthEnabled(true)
        renderStates:SetDepthComparisonFunc(DepthComparisonFunc_GreaterEqual)
        renderStates:SetBlendEnabled(0, true)
        renderStates:SetBlendSource(0, BlendFactor_AlphaSource)
        renderStates:SetBlendDest(0, BlendFactor_AlphaSourceInverse)
        renderStates:SetBlendOp(0, BlendOp_Add)
        renderStates:SetBlendAlphaSource(0, BlendFactor_One)
        renderStates:SetBlendAlphaDest(0, BlendFactor_AlphaSourceInverse)
        renderStates:SetBlendAlphaOp(0, BlendOp_Add)
    end)
end

local function ConfigureAdditiveBlending(shaderItem)
    SafeConfigureRenderStates(shaderItem, function(renderStates)
        renderStates:SetDepthEnabled(true)
        renderStates:SetDepthComparisonFunc(DepthComparisonFunc_GreaterEqual)
        renderStates:SetBlendEnabled(0, true)
        renderStates:SetBlendSource(0, BlendFactor_One)
        renderStates:SetBlendDest(0, BlendFactor_One)
        renderStates:SetBlendOp(0, BlendOp_Add)
        renderStates:SetBlendAlphaSource(0, BlendFactor_One)
        renderStates:SetBlendAlphaDest(0, BlendFactor_AlphaSourceInverse)
        renderStates:SetBlendAlphaOp(0, BlendOp_Add)
    end)
end

local function ConfigureSubtractiveBlending(shaderItem)
    SafeConfigureRenderStates(shaderItem, function(renderStates)
        renderStates:SetDepthEnabled(true)
        renderStates:SetDepthComparisonFunc(DepthComparisonFunc_GreaterEqual)
        renderStates:SetBlendEnabled(0, true)
        renderStates:SetBlendSource(0, BlendFactor_One)
        renderStates:SetBlendDest(0, BlendFactor_One)
        renderStates:SetBlendOp(0, BlendOp_Subtract)
        renderStates:SetBlendAlphaSource(0, BlendFactor_One)
        renderStates:SetBlendAlphaDest(0, BlendFactor_AlphaSourceInverse)
        renderStates:SetBlendAlphaOp(0, BlendOp_Subtract)
    end)
end

local function ConfigureModulateBlending(shaderItem)
    SafeConfigureRenderStates(shaderItem, function(renderStates)
        renderStates:SetDepthEnabled(true)
        renderStates:SetDepthComparisonFunc(DepthComparisonFunc_GreaterEqual)
        renderStates:SetBlendEnabled(0, true)
        renderStates:SetBlendSource(0, BlendFactor_Zero)
        renderStates:SetBlendDest(0, BlendFactor_ColorSource)
        renderStates:SetBlendOp(0, BlendOp_Add)
        renderStates:SetBlendAlphaSource(0, BlendFactor_One)
        renderStates:SetBlendAlphaDest(0, BlendFactor_AlphaSourceInverse)
        renderStates:SetBlendAlphaOp(0, BlendOp_Add)
    end)
end

local function ResetAlphaBlending(shaderItem)
    SafeConfigureRenderStates(shaderItem, function(renderStates)
        renderStates:ClearDepthEnabled()
        renderStates:ClearDepthWriteMask()
        renderStates:ClearBlendEnabled(0)
        renderStates:ClearBlendSource(0)
        renderStates:ClearBlendDest(0)
        renderStates:ClearBlendOp(0)
        renderStates:ClearBlendAlphaSource(0)
        renderStates:ClearBlendAlphaDest(0)
        renderStates:ClearBlendAlphaOp(0)
    end)
end

-- Table-driven blend mode dispatch: maps BlendMode_* to { configureFn, drawListTag }
local BlendConfig = {
    [BlendMode_Opaque]      = { ConfigureOpaqueBlending,      "forward"     },
    [BlendMode_Cutout]      = { ConfigureCutoutBlending,      "forward"     },
    [BlendMode_Blended]     = { ConfigureBlendedBlending,     "transparent" },
    [BlendMode_Additive]    = { ConfigureAdditiveBlending,    "transparent" },
    [BlendMode_Subtractive] = { ConfigureSubtractiveBlending, "transparent" },
    [BlendMode_Modulate]    = { ConfigureModulateBlending,    "transparent" },
}

-- Helper function to get blend mode from either property name
local function GetBlendMode(context)
    -- Try blend.mode first (old particle naming)
    local success, value = pcall(function()
        return context:GetMaterialPropertyValue_enum("blend.mode")
    end)
    if success then
        return value
    end

    -- Fall back to opacity.mode (new standard naming)
    success, value = pcall(function()
        local opacityMode = context:GetMaterialPropertyValue_enum("opacity.mode")
        -- Map opacity modes to blend modes
        if opacityMode == OpacityMode_Opaque then
            return BlendMode_Opaque
        elseif opacityMode == OpacityMode_Cutout then
            return BlendMode_Cutout
        elseif opacityMode == OpacityMode_Blended then
            return BlendMode_Blended
        elseif opacityMode == OpacityMode_TintedTransparent then
            return BlendMode_Blended  -- Treat TintedTransparent as Blended
        end
        return BlendMode_Opaque  -- Default to Opaque
    end)
    if success then
        return value
    end

    -- Default to Opaque if neither property exists
    return BlendMode_Opaque
end

--------------------------------------------------------------------------------
-- Material functor entry points
--------------------------------------------------------------------------------

function Process(context)
    local blend = GetBlendMode(context)
    local forwardPass = context:GetShaderByTag("ForwardPass")

    if IsShaderItemValid(forwardPass) then
        local config = BlendConfig[blend]
        if config then
            config[1](forwardPass)
            forwardPass:SetDrawListTagOverride(config[2])
        else
            ResetAlphaBlending(forwardPass)
            forwardPass:SetDrawListTagOverride("")
        end
    end
end

function ProcessEditor(context)
    local blendMode = GetBlendMode(context)

    -- Show alpha cutoff factor only for Cutout mode
    local factorVisibility = (blendMode == BlendMode_Cutout)
        and MaterialPropertyVisibility_Enabled
        or MaterialPropertyVisibility_Hidden

    -- Try both property names for factor
    local success = pcall(function()
        context:SetMaterialPropertyVisibility("blend.factor", factorVisibility)
    end)
    if not success then
        pcall(function()
            context:SetMaterialPropertyVisibility("opacity.factor", factorVisibility)
        end)
    end

    -- Show opacity factor for transparent blend modes
    local isTransparent = (blendMode == BlendMode_Blended)
        or (blendMode == BlendMode_Additive)
        or (blendMode == BlendMode_Subtractive)
        or (blendMode == BlendMode_Modulate)
    local opacityVisibility = isTransparent
        and MaterialPropertyVisibility_Enabled
        or MaterialPropertyVisibility_Hidden

    -- Try both property names for opacityFactor
    success = pcall(function()
        context:SetMaterialPropertyVisibility("blend.opacityFactor", opacityVisibility)
    end)
    if not success then
        pcall(function()
            context:SetMaterialPropertyVisibility("opacity.factor", opacityVisibility)
        end)
    end
end
