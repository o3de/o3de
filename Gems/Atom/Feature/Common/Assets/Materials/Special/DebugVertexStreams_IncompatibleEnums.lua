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


DebugVertexStream_Normals = 0
DebugVertexStream_Tangents = 1
DebugVertexStream_Bitangents = 2
DebugVertexStream_Uvs = 3
DebugVertexStream_TangentW = 4

function GetMaterialPropertyDependencies()
    return { "general.debugVertexStream", "general.uvIndex"}
end

function ProcessEditor(context)
    local vertexStream = context:GetMaterialPropertyValue_enum("general.debugVertexStream");

    -- Depending on which vertex stream is being visualized, only certain settings are applicable
    if(vertexStream == DebugVertexStream_Normals) then
        -- Normals can be viewed as colors using either method        
        context:SetMaterialPropertyVisibility("general.colorDisplayMode", MaterialPropertyVisibility_Enabled)

        -- Uv set and tangent/bitangent options do not impact normals
        context:SetMaterialPropertyVisibility("general.tangentOptions", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("general.bitangentOptions", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("general.uvIndex", MaterialPropertyVisibility_Hidden)
    elseif(vertexStream == DebugVertexStream_Tangents) then
        -- Tangents can be viewed using either color display mode, and are affected by tangent options and uv index
        context:SetMaterialPropertyVisibility("general.tangentOptions", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("general.colorDisplayMode", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("general.uvIndex", MaterialPropertyVisibility_Enabled)

        context:SetMaterialPropertyVisibility("general.bitangentOptions", MaterialPropertyVisibility_Hidden)
    elseif(vertexStream == DebugVertexStream_Bitangents) then
        -- Bitangents can be viewed using either color display mode, and are affected by tangent options and uv index
        context:SetMaterialPropertyVisibility("general.bitangentOptions", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("general.colorDisplayMode", MaterialPropertyVisibility_Enabled)
        context:SetMaterialPropertyVisibility("general.uvIndex", MaterialPropertyVisibility_Enabled)
        
        context:SetMaterialPropertyVisibility("general.tangentOptions", MaterialPropertyVisibility_Hidden)
    elseif(vertexStream == DebugVertexStream_Uvs) then
        -- Uvs are only impacted by uv index
        context:SetMaterialPropertyVisibility("general.uvIndex", MaterialPropertyVisibility_Enabled)

        context:SetMaterialPropertyVisibility("general.tangentOptions", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("general.bitangentOptions", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("general.colorDisplayMode", MaterialPropertyVisibility_Hidden)
    elseif(vertexStream == DebugVertexStream_TangentW) then
        -- Tangent.w can only use per-vertex data, and only for the first uv set, and is only displayed one way,
        -- so it is not impacted by any settings
        context:SetMaterialPropertyVisibility("general.tangentOptions", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("general.bitangentOptions", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("general.colorDisplayMode", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("general.uvIndex", MaterialPropertyVisibility_Hidden)
    end

    -- For the second uv set, only the surface gradient technique is supported for generating tangents and bitangents
    local uvSet = context:GetMaterialPropertyValue_enum("general.uvIndex");
    if(uvSet > 0) then
        context:SetMaterialPropertyVisibility("general.tangentOptions", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("general.bitangentOptions", MaterialPropertyVisibility_Hidden)
    end
end
