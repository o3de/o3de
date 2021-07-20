----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
Script.ReloadScript("scripts/Utils/EntityUtils.lua")

-- Basic entity
LivingEntity = {
    Properties = {
        soclasses_SmartObjectClass = "",
        bMissionCritical = 0,
        bCanTriggerAreas = 1,
        DmgFactorWhenCollidingAI = 1,
                
        object_Model = "objects/default/primitive_capsule.cgf",
        Physics = {
            bPhysicalize = 1, -- True if object should be physicalized at all.
            bPushableByPlayers = 1,
        },
        Living = {
            height = 0,    -- vertical offset of collision geometry center
            vector_size = {0.4, 0.4,0.9}, -- collision cylinder dimensions
            height_eye = 1.8, -- vertical offset of camera
            height_pivot = 0.1, -- offset from central ground position that is considered entity center
            head_radius = 0.3,    -- radius of the 'head' geometry (used for camera offset)
            height_head = 1.7,    -- center.z of the head geometry
            groundContactEps = 0.004,    --the amount that the living needs to move upwards before ground contact is lost. defaults to which ever is greater 0.004, or 0.01*geometryHeight
            bUseCapsule = 1,--switches between capsule and cylinder collider geometry
            
            inertia = 1,    -- inertia koefficient, the more it is, the less inertia is, 0 means no inertia
            inertiaAccel = 1, -- inertia on acceleration
            air_control = 1, -- air control koefficient 0..1, 1 - special value (total control of movement)
            air_resistance = 0.1,    -- standard air resistance 
            gravity = 9.8, -- gravity vector
            mass = 100,    -- mass (in kg)
            min_slide_angle = 60, -- if surface slope is more than this angle, player starts sliding (angle is in radians)
            max_climb_angle = 60, -- player cannot climb surface which slope is steeper than this angle
            max_jump_angle = 45, -- player is not allowed to jump towards ground if this angle is exceeded
            min_fall_angle = 65,    -- player starts falling when slope is steeper than this
            max_vel_ground = 10, -- player cannot stand of surfaces that are moving faster than this
            timeImpulseRecover = 0.3, -- forcefully turns on inertia for that duration after receiving an impulse
            nod_speed = 1,    -- vertical camera shake speed after landings
            bActive = 1,-- 0 disables all simulation for the character, apart from moving along the requested velocity
            collision_types = 271, -- (271 = ent_static | ent_terrain | ent_living | ent_rigid | ent_sleeping_rigid) entity types to check collisions against
        

        },
        MultiplayerOptions = {
            bNetworked        = 0,
        },
        
        bExcludeCover=0,
    },
    
    Client = {},
    Server = {},
    
    -- Temp.
    _Flags = {},
    
        Editor={
        Icon = "physicsobject.bmp",
        IconOnTop=1,
      },
            
}


------------------------------------------------------------------------------------------------------
function LivingEntity:OnSpawn()
    if (self.Properties.MultiplayerOptions.bNetworked == 0) then
        self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
    end

    self:SetFromProperties();
end


------------------------------------------------------------------------------------------------------
function LivingEntity:SetFromProperties()
    local Properties = self.Properties;
    
    if (Properties.object_Model == "") then
        do return end;
    end
    
    self.freezable = (tonumber(Properties.bFreezable) ~= 0);
    
    self:SetupModel();

    -- Mark AI hideable flag.
    if (Properties.bAutoGenAIHidePts == 1) then
        self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 0); -- set
    else
        self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 2); -- remove
    end
    
    if (self.Properties.bCanTriggerAreas == 1) then
        self:SetFlags(ENTITY_FLAG_TRIGGER_AREAS, 0); -- set
    else
        self:SetFlags(ENTITY_FLAG_TRIGGER_AREAS, 2); -- remove
    end
end

------------------------------------------------------------------------------------------------------
function LivingEntity:SetupModel()
    local Properties = self.Properties;
    self:LoadObject(0,Properties.object_Model);

    if (Properties.Physics.bPhysicalize == 1) then
        self:PhysicalizeThis();
    end
end


------------------------------------------------------------------------------------------------------
function LivingEntity:OnLoad(table)  
    self.health = table.health;
    self.dead = table.dead;
end

function LivingEntity:OnSave(table)  
    table.health = self.health;
    table.dead = self.dead;
end


------------------------------------------------------------------------------------------------------
function LivingEntity:IsRigidBody()
    local Properties = self.Properties;
    local Mass = Properties.Mass;
    local Density = Properties.Density;
    if (Mass == 0 or Density == 0 or Properties.bPhysicalize ~= 1) then 
          return false;
    end
    return true;
end

------------------------------------------------------------------------------------------------------
function LivingEntity:PhysicalizeThis()
    Entity.Physicalize(self,0, PE_LIVING, self.Properties);
end

------------------------------------------------------------------------------------------------------
-- OnPropertyChange called only by the editor.
------------------------------------------------------------------------------------------------------
function LivingEntity:OnPropertyChange()
  -- if the properties are changed, we force a reset in the __usable
  if (self.__usable) then
    if (self.__origUsable ~= self.Properties.bUsable or self.__origPickable ~= self.Properties.bPickable) then
      self.__usable = nil;
    end
  end
    self:SetFromProperties();
end

------------------------------------------------------------------------------------------------------
-- OnReset called only by the editor.
------------------------------------------------------------------------------------------------------
function LivingEntity:OnReset()
    self:ResetOnUsed()
    
    local PhysProps = self.Properties.Physics;
    if (PhysProps.bPhysicalize == 1) then
        self:PhysicalizeThis();
        self:AwakePhysics(0);
    end
end

------------------------------------------------------------------------------------------------------
function LivingEntity:Event_Remove()
    self:DrawSlot(0,0);
    self:DestroyPhysics();
    self:ActivateOutput( "Remove", true );
end


------------------------------------------------------------------------------------------------------
function LivingEntity:Event_Hide()
    self:Hide(1);
    self:ActivateOutput( "Hide", true );
    if CurrentCinematicName then
        Log("%.3f %s %s : Event_Hide", _time, CurrentCinematicName, self:GetName() );
    end
end

------------------------------------------------------------------------------------------------------
function LivingEntity:Event_UnHide()
    self:Hide(0);
    self:ActivateOutput( "UnHide", true );
    if CurrentCinematicName then
        Log("%.3f %s %s : Event_UnHide", _time, CurrentCinematicName, self:GetName() );
    end
end

------------------------------------------------------------------------------------------------------
function LivingEntity:Event_Ragdollize()  
    self:RagDollize(0);
    self:ActivateOutput( "Ragdollized", true );
    if (self.Event_RagdollizeDerived) then
        self:Event_RagdollizeDerived();
    end
end

------------------------------------------------------------------------------------------------------
function LivingEntity.Client:OnPhysicsBreak( vPos,nPartId,nOtherPartId )
    self:ActivateOutput("Break",nPartId+1 );
end


function LivingEntity:IsUsable(user)
    local ret = nil
    -- From EntityUtils.lua
    if not self.__usable then self.__usable = self.Properties.bUsable end
    
    local mp = System.IsMultiplayer();
    if(mp and mp~=0) then
        return 0;
    end

    if (self.__usable == 1) then
        ret = 2
    else
        local PhysProps = self.Properties.Physics;
        if (self:IsRigidBody() == true and user and user.CanGrabObject) then
            ret = user:CanGrabObject(self)
        end
    end
        
    return ret or 0
end

LivingEntity.FlowEvents =
{
    Inputs =
    {
        Used = { LivingEntity.Event_Used, "bool" },
        EnableUsable = { LivingEntity.Event_EnableUsable, "bool" },
        DisableUsable = { LivingEntity.Event_DisableUsable, "bool" },

        Hide = { LivingEntity.Event_Hide, "bool" },
        UnHide = { LivingEntity.Event_UnHide, "bool" },
        Remove = { LivingEntity.Event_Remove, "bool" },
        Ragdollize = { LivingEntity.Event_Ragdollize, "bool" },
    },
    Outputs =
    {
        Used = "bool",
        EnableUsable = "bool",
        DisableUsable = "bool",
        Activate = "bool",
        Hide = "bool",
        UnHide = "bool",
        Remove = "bool",
        Ragdollized = "bool",        
        Break = "int",
    },
}


MakeUsable(LivingEntity);
MakePickable(LivingEntity);
MakeTargetableByAI(LivingEntity);
MakeKillable(LivingEntity);
AddHeavyObjectProperty(LivingEntity);
AddInteractLargeObjectProperty(LivingEntity);
SetupCollisionFiltering(LivingEntity);
