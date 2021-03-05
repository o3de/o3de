----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------
-- #Script.ReloadScript("scripts/default/entities/player/basicplayer.lua")

DeadBody =
{
    type = "DeadBody",
    temp_ModelName = "",

    DeadBodyParams =
    {
        max_time_step = 0.025,
        sleep_speed = 0.025,
        damping = 0.3,
        freefall_damping = 0.1,

        lying_mode_ncolls = 4,
        lying_sleep_speed = 0.065,
        lying_damping = 1.5,
    },

    PhysParams =
    {
        mass = 80,
        height = 1.8,
        eyeheight = 1.7,
        sphereheight = 1.2,
        radius = 0.45,
        stiffness_scale = 0,
        lod = 1,
        retain_joint_vel = 0,
    },

    Properties =
    {
        soclasses_SmartObjectClass = "",
        bResting = 1,
        object_Model = "objects/characters/human/sdk_player/sdk_player.cdf",
        lying_damping = 1.5,
        bCollidesWithPlayers = 0,
        bPushableByPlayers = 0,
        Mass = 80,
        Stiffness = 0,
        MaxTimeStep = 0.025,
        bExtraStiff = 0,
        bNoFriendlyFire = 0,
        PoseAnim = "",

        Buoyancy=
        {
            water_density = 1000,
            water_damping = 0,
            water_resistance = 1000,
        },

        TacticalInfo =
        {
            bScannable = 0,
            LookupName = "",
        },
    },

    Editor =
    {
        Icon = "DeadBody.bmp",
        IconOnTop = 1,
    },
}

-------------------------------------------------------
function DeadBody:OnLoad(table)
    self.temp_ModelName = table.temp_ModelName
    self.PhysParams = table.PhysParams
    self.DeadBodyParams = table.DeadBodyParams
end

-------------------------------------------------------
function DeadBody:OnSave(table)
    table.temp_ModelName = self.temp_ModelName
    table.PhysParams = self.PhysParams
    table.DeadBodyParams = self.DeadBodyParams
end


-----------------------------------------------------------------------------------------------------------
function DeadBody:OnReset()
    --self.temp_ModelName ="";
    --self:OnPropertyChange();

    self.bScannable = self.Properties.TacticalInfo.bScannable;

  -- no problem in repeatedly registering or unregistering
  if (self.bScannable==1) then
        Game.AddTacticalEntity(self.id, eTacticalEntity_Story);
    else
      Game.RemoveTacticalEntity(self.id, eTacticalEntity_Story);
  end

    self:LoadCharacter(0,self.Properties.object_Model);
    --self:StartAnimation( 0,"cidle" );
    self:PhysicalizeThis();
end


-----------------------------------------------------------------------------------------------------------
function DeadBody:Server_OnInit()
    if (not CryAction.IsServer()) then
        DeadBody.OnPropertyChange( self );
    end
end


-----------------------------------------------------------------------------------------------------------
function DeadBody:Client_OnInit()
    DeadBody.OnPropertyChange( self );

    self:SetUpdatePolicy( ENTITY_UPDATE_PHYSICS_VISIBLE );
    --self:Activate(1);

--    self:RenderShadow( 1 ); -- enable rendering of player shadow
--  self:SetShaderFloat("HeatIntensity",0,0,0);
end


-----------------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------------
function DeadBody:Server_OnDamageDead( hit )
--printf("server on Damage DEAD %.2f %.2f",hit.impact_force_mul_final,hit.impact_force_mul);
  --System.Log("DeadBody hit part "..hit.ipart);
    if( hit.ipart ) then
        self:AddImpulse( hit.ipart, hit.pos, hit.dir, hit.impact_force_mul );
    else
        self:AddImpulse( -1, hit.pos, hit.dir, hit.impact_force_mul );
    end
end

-----------------------------------------------------------------------------------------------------------

function DeadBody:OnHit()
    BroadcastEvent( self,"Hit" );
end

-----------------------------------------------------------------------------------------------------------

function DeadBody:OnPropertyChange()
    --System.LogToConsole("prev:"..self.temp_ModelName.." new:"..self.Properties.object_Model);

    self.PhysParams.mass = self.Properties.Mass;

    if (self.Properties.object_Model ~= self.temp_ModelName) then
        self.temp_ModelName = self.Properties.object_Model;
        self:LoadCharacter(0,self.Properties.object_Model);
    end
    self:PhysicalizeThis();
end

--------------------------------------------------------------------------------------------------------
function DeadBody:PhysicalizeThis()
    local Properties = self.Properties;
    local status = 1;

    self.PhysParams.retain_joint_vel = 0;
    if (Properties.PoseAnim ~= "") then
        self:StartAnimation( 0, Properties.PoseAnim, 0,0,1,1,1,0,1 );
        self.PhysParams.retain_joint_vel = 1; --this is just to retain the positions of non-physical bones
    end

    local bPushableByPlayers = Properties.bPushableByPlayers;
    local bCollidesWithPlayers = Properties.bCollidesWithPlayers;

    if (status == 1) then
        bPushableByPlayers = 0;
        bCollidesWithPlayers = 0;
    end

    self.PhysParams.mass = Properties.Mass;
    self.PhysParams.stiffness_scale = Properties.Stiffness*(1-Properties.bExtraStiff*2);

    self:Physicalize( 0, PE_ARTICULATED, self.PhysParams );

    self:SetPhysicParams(PHYSICPARAM_SIMULATION, self.Properties );
    self:SetPhysicParams(PHYSICPARAM_BUOYANCY, self.Properties.Buoyancy );

    if (Properties.lying_damping) then
        self.DeadBodyParams.lying_damping = Properties.lying_damping;
    end
    self.DeadBodyParams.max_time_step = Properties.MaxTimeStep;
    self:SetPhysicParams(PHYSICPARAM_SIMULATION, self.DeadBodyParams);
    self:SetPhysicParams(PHYSICPARAM_ARTICULATED, self.DeadBodyParams);

    local flagstab = { flags_mask=geom_colltype_player, flags=geom_colltype_player*bCollidesWithPlayers };
    if (status == 1) then
        flagstab.flags_mask = geom_colltype_explosion + geom_colltype_ray + geom_colltype_foliage_proxy + geom_colltype_player;
    end
    self:SetPhysicParams(PHYSICPARAM_PART_FLAGS, flagstab);
    flagstab.flags_mask = pef_pushable_by_players;
    flagstab.flags = pef_pushable_by_players*bPushableByPlayers;
    self:SetPhysicParams(PHYSICPARAM_FLAGS, flagstab);
    if (Properties.bResting == 1) then
        self:AwakePhysics(0);
    else
        self:AwakePhysics(1);
    end
    self:EnableProceduralFacialAnimation(false);
    self:PlayFacialAnimation("death_pose_0"..random(1,5), true);
    -- small hack: we need one update to sync physics and animation, so here it is after physicalization
    -- (our OnUpdate function should just deactivate immediately.)
    --self:Activate(1);
end

--function DeadBody:OnUpdate()
    --self:Activate(0);
--end

function DeadBody:Event_Hide()
    self:Hide(1);
end;

------------------------------------------------------------------------------------------------------
function DeadBody:Event_UnHide()
    self:Hide(0);
end;

--------------------------------------------------------------------------------------------------------
function DeadBody:Event_Awake()
    self:AwakePhysics(1);
end

--------------------------------------------------------------------------------------------------------
function DeadBody:Event_Hit(sender)
    BroadcastEvent(self, "Hit");
end

--------------------------------------------------------------------------------------------------------
DeadBody.Server =
{
    OnInit = DeadBody.Server_OnInit,
--    OnShutDown = function( self ) end,
    OnDamage = DeadBody.Server_OnDamageDead,
    OnHit = DeadBody.OnHit,
    OnUpdate=DeadBody.OnUpdate,
}

--------------------------------------------------------------------------------------------------------
DeadBody.Client =
{
    OnInit = DeadBody.Client_OnInit,
--    OnShutDown = DeadBody.Client_OnShutDown,
    OnDamage=DeadBody.Client_OnDamage,
    OnUpdate=DeadBody.OnUpdate,
}

----------------------------------------------------------------------------------------------------
function DeadBody:HasBeenScanned()
    self:ActivateOutput("Scanned", true);
end


--------------------------------------------------------------------

DeadBody.FlowEvents =
{
    Inputs =
    {
        Awake = { DeadBody.Event_Awake, "bool" },
        Hide = { DeadBody.Event_Hide, "bool" },
        UnHide = { DeadBody.Event_UnHide, "bool" },
    },
    Outputs =
    {
        Awake = "bool",
        Hit = "bool",
        Scanned = "bool",
    },
}
