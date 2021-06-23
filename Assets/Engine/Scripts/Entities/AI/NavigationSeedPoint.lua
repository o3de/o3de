----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
NavigationSeedPoint = {
  type = "NavigationSeedPoint",

    Editor = {
        Icon = "Seed.bmp",
    },
}

-------------------------------------------------------
function NavigationSeedPoint:OnInit()
    CryAction.RegisterWithAI(self.id, AIOBJECT_NAV_SEED);
end