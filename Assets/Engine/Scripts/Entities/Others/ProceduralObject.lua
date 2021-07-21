----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------
--
-- Description :        Procedural object entity
--
-- Created by Marco C. May 2013
--
----------------------------------------------------------------------------

ProceduralObject = {

type = "ProceduralObject",

    Properties = {

        filePrefabLibrary= "",

        ObjectVariation={
            sPrefabVariation = "",
        },

        nMaxSpawn = 0,
    },

    -- editor information
    Editor = {
        Icon = "proceduralobject.bmp",
        ShowBounds = 1,
    },

    PrefabSourceName = "",

    --Client = {},
    --Server = {},
}

function ProceduralObject:OnInit()
    --System.Log( "OnInit proc object" );
    self:OnReset();
end

function ProceduralObject:OnPropertyChange()
    --System.Log( "OnPropertyChange" );
    self:OnReset();
end

function ProceduralObject:OnDestroy( sender )
    PrefabManager.Delete(self.id);
end

function ProceduralObject:OnMove()
    --System.Log( "OnMove proc object" );
    PrefabManager.Move(self.id);
end

function ProceduralObject:OnSpawn()

    if (CryAction.IsClient()) then
        --System.Log( "CLIENT" );
        self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
    end

    if (CryAction.IsServer()) then
        --System.Log( "SERVER" );
        self:SetFlags(ENTITY_FLAG_SERVER_ONLY,0);
    end
end

function ProceduralObject:Spawn(seed)
    --System.Log( "Spawning proc object" );

    --System.Log( "PrefabName="..self.PrefabSourceName );

    PrefabManager.Delete(self.id);
    local props=self.Properties;

    if(not EmptyString(props.filePrefabLibrary)) then
        --System.Log( "Opening file:"..props.filePrefabLibrary);
        PrefabManager.LoadLibrary(props.filePrefabLibrary);

        if(not EmptyString(props.ObjectVariation.sPrefabVariation)) then
            PrefabManager.Spawn(self.id,props.filePrefabLibrary,props.ObjectVariation.sPrefabVariation,seed,props.nMaxSpawn);
        end
    end

    --System.Log( "PrefabName="..self.PrefabSourceName );
end

function ProceduralObject:OnHidden()
    PrefabManager.Hide(self.id,true);
end

function ProceduralObject:OnUnHidden()
    PrefabManager.Hide(self.id,false);
end

function ProceduralObject:OnReset( sender )

    --System.Log( "OnReset" );
    if (System.IsEditor()) then
        -- change prefabs only if we are editing (and the user pressed reload script),
        -- do not change every time we go in and out of game mode...
        -- unless we are in game mode generation
        local clientgamemode=0; --System.GetCVar("g_GameModeGenerate");

        if (System.IsEditing() or clientgamemode==1) then
            PrefabManager.Delete(self.id);
            self:Spawn(0);
        end
    else
        PrefabManager.Delete(self.id);
    end

    --System.Log( "OnReset done" );
end

