----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
local M = {}

function M.ActivateMySpawner3(id)
    SpawnerComponentRequestBus.Event.Spawn(id)
end

return M