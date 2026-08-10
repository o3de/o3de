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
    return {"useAlphaToCoverage"}
end

-- Shader tags of passes that write to a color target. Alpha-to-coverage derives sample
-- coverage from the alpha of render target 0, so it is only meaningful on these shaders.
-- Enabling it on depth/shadow-only shaders (which have no color target) is undefined.
ColorPassTags = {"forward", "forward_customZ", "main", "main_customZ"}

function Process(context)
    local useA2C = false
    if context:HasMaterialProperty("useAlphaToCoverage") then
        useA2C = context:GetMaterialPropertyValue_bool("useAlphaToCoverage")
    end

    -- Clear the override everywhere first, then enable it only on the color passes.
    local lastShader = context:GetShaderCount() - 1
    for i = 0, lastShader do
        context:GetShader(i):GetRenderStatesOverride():ClearAlphaToCoverageEnabled()
    end

    if useA2C then
        for _, tag in ipairs(ColorPassTags) do
            if context:HasShaderWithTag(tag) then
                context:GetShaderByTag(tag):GetRenderStatesOverride():SetAlphaToCoverageEnabled(true)
            end
        end
    end
end
