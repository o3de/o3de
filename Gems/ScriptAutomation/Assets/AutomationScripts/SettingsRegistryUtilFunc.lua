----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
----------------------------------------------------------------------------------------------------

function GetRequiredStringValue(valueKey, prettyName)
    value = g_SettingsRegistry:GetString(valueKey)
    if (not value:has_value()) then
        Print('FrameTime script missing ' .. tostring(prettyName) .. ' settings registry entry, ending script early')
        return false, nil
    end
    return true, value:value()
end

function GetOptionalUIntValue(valueKey, defaultValue)
    return g_SettingsRegistry:GetUInt(valueKey):value_or(defaultValue)
end
function GetOptionalStringValue(valueKey, defaultValue)
    return g_SettingsRegistry:GetString(valueKey):value_or(defaultValue)
end
