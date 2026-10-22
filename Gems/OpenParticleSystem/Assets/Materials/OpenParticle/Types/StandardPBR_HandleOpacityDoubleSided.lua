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
    return { "general.doubleSided" }
end

ForwardPassIndex = 0
ForwardPassEdsIndex = 1

-- Helper function to safely get render states and apply configuration
-- Helper function to validate if a shader item is valid
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

function Process(context)
    local doubleSided = context:GetMaterialPropertyValue_bool("general.doubleSided")
    local lastShader = context:GetShaderCount() - 1;

    if lastShader < 0 then
        -- No shaders available, skip processing
        return
    end

    if doubleSided then
        for i = 0, lastShader do
            local shader = context:GetShader(i)
            if IsShaderItemValid(shader) then
                SafeConfigureRenderStates(shader, function(renderStates)
                    renderStates:SetCullMode(CullMode_None)
                end)
            end
        end
    else
        for i = 0, lastShader do
            local shader = context:GetShader(i)
            if IsShaderItemValid(shader) then
                SafeConfigureRenderStates(shader, function(renderStates)
                    renderStates:ClearCullMode()
                end)
            end
        end
    end
end
