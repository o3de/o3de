----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
TagPoint = {
    type = "TagPoint",

    Editor = {
        Icon = "TagPoint.bmp",
    },
}

-------------------------------------------------------
function TagPoint:OnSpawn()
    self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
end

-------------------------------------------------------
function TagPoint:OnInit()
    CryAction.RegisterWithAI(self.id, AIOBJECT_WAYPOINT);
end

