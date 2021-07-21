----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
-- Just here so the entity system won't spam
CActorWrapper =
{
        Editor={
        Icon="SpawnPoint.bmp",
    },
    Properties =
    {
        Prototype = ""
    },
}

function CActorWrapper:OnSpawn()
    CryAction.CreateGameObjectForEntity(self.id);
    CryAction.ActivateExtensionForGameObject(self.id, "CActorWrapper", true);
end

function CActorWrapper:OnDestroy()
    CryAction.ActivateExtensionForGameObject(self.id, "CActorWrapper", false);
end
