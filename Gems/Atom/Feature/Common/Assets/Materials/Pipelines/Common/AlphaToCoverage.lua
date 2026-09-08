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

function Process(context)
    local useA2C = false
    if context:HasMaterialProperty("useAlphaToCoverage") then
        useA2C = context:GetMaterialPropertyValue_bool("useAlphaToCoverage")
    end

    local lastShader = context:GetShaderCount() - 1
    if useA2C then
        for i = 0, lastShader do
            context:GetShader(i):GetRenderStatesOverride():SetAlphaToCoverageEnabled(true)
        end
    else
        for i = 0, lastShader do
            context:GetShader(i):GetRenderStatesOverride():ClearAlphaToCoverageEnabled()
        end
    end
end
