----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
RigidBody = {
  type = "RigidBody",
  MapVisMask = 0,

    Properties = {
        soclasses_SmartObjectClass = "",
        bAutoGenAIHidePts = 0,

        objModel = "Objects/box.cgf",
        Density = 5000,
        Mass = 1,
        bResting = 1, -- If rigid body is originally in resting state.
        bVisible = 1, -- If rigid body is originally visible.
        bRigidBodyActive = 1, -- If rigid body is originally created OR will be created only on OnActivate.
        bActivateOnRocketDamage = 0, -- Activate when a rocket hit the entity.
        Impulse = {X=0,Y=0,Z=0}, -- Impulse to apply at event.
        max_time_step = 0.01,
        sleep_speed = 0.04,
        damping = 0,
        water_damping = 1.5,
        water_resistance = 0,    
    },
    temp_vec={x=0,y=0,z=0},
    PhysParams = { mass=0,density=0 },
    
    updateTime = 500,
    gravityUpdate = 0,
    
        Editor={
        Icon = "physicsobject.bmp",
        IconOnTop=1,
    },
}

-------------------------------------------------------
function RigidBody:OnInit()
    self.ModelName = "";
    self.Mass = 0;
    self:OnReset();
    
--    System.Log( "here1" );
end

-------------------------------------------------------
function RigidBody:OnReset()
    --self:NetPresent(nil);
    if (self.ModelName ~= self.Properties.objModel or self.Mass ~= self.Properties.Mass) then
        self.Mass = self.Properties.Mass;
        self.ModelName = self.Properties.objModel;
        self:LoadObject( 0,self.ModelName );

    end
    
    local Properties = self.Properties;

    if (Properties.bVisible == 0) then
        self:DrawSlot( 0,0 );
    else
        self:DrawSlot( 0,1 );
    end

    local physType;
    if (self.Properties.bRigidBodyActive == 1) then            
        physType = PE_RIGID;
        self.PhysParams.density = Properties.Density;
        self.PhysParams.mass = Properties.Mass;
    else
        physType = PE_STATIC;
    end

    self:Physicalize( 0,physType,self.PhysParams );
    self:SetPhysicParams(PHYSICPARAM_SIMULATION, self.Properties );
    self:SetPhysicParams(PHYSICPARAM_BUOYANCY, self.Properties );
    
    if (self.Properties.bResting == 0) then
        self:AwakePhysics(1);
    else
        self:AwakePhysics(0);
    end
    
    -- Mark AI hideable flag.
    if (self.Properties.bAutoGenAIHidePts == 1) then
        self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 0); -- set
    else
        self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 2); -- remove
    end

end

-------------------------------------------------------
function RigidBody:OnPropertyChange()
    self:OnReset();
end

-------------------------------------------------------
function RigidBody:OnContact( player )
    self:Event_OnTouch( player );
end

-------------------------------------------------------
function RigidBody:OnDamage( hit )
    --System.LogToConsole( "On Damage" );

    if( hit.ipart ) then
        self:AddImpulse( hit.ipart, hit.pos, hit.dir, hit.impact_force_mul );
--    else    
--        self:AddImpulse( -1, hit.pos, hit.dir, hit.impact_force_mul );
    end    

    if(self.Properties.bActivateOnRocketDamage)then
        if(hit.explosion)then
            self:AwakePhysics(1);
        end
    end
end

-------------------------------------------------------
function RigidBody:OnShutDown()
end

-------------------------------------------------------
function RigidBody:OnTimer()
--    System.Log("RigidBody OnTimer");
--    self:SetTimer( 0,1000 );
end

function RigidBody:OnUpdate()

    --FIXME: all this timing stuff will be replaced by OnTimer once it will work again
    self.gravityUpdate = self.gravityUpdate + _frametime;
    
    if (self.gravityUpdate < 0.5) then
        return;
    end
    
    self.gravityUpdate = 0.0;
    EntityUpdateGravity(self);
end

-------------------------------------------------------
-- Input events
-------------------------------------------------------
function RigidBody:Event_AddImpulse(sender)
    self.temp_vec.x=self.Properties.Impulse.X;
    self.temp_vec.y=self.Properties.Impulse.Y;
    self.temp_vec.z=self.Properties.Impulse.Z;
    self:AddImpulse(0,nil,self.temp_vec,1);
end

-------------------------------------------------------
function RigidBody:Event_Activate(sender)    

    --create rigid body 
    self:CreateRigidBody( self.Properties.Density,self.Properties.Mass,0 );

    self:Activate(1);
    self:AwakePhysics(1);
end

-------------------------------------------------------
function RigidBody:Event_Show(sender)
    self:DrawSlot( 0,1 );
end

-------------------------------------------------------
function RigidBody:Event_Hide(sender)
    self:DrawSlot( 0,0 );
end

-------------------------------------------------------
-- Output events
-------------------------------------------------------
function RigidBody:Event_OnTouch(sender)
    BroadcastEvent( self,"OnTouch" );
end

RigidBody.FlowEvents =
{
    Inputs =
    {
        Activate = { RigidBody.Event_Activate, "bool" },
        AddImpulse = { RigidBody.Event_AddImpulse, "bool" },
        Hide = { RigidBody.Event_Hide, "bool" },
        Show = { RigidBody.Event_Show, "bool" },
        OnTouch = { RigidBody.Event_OnTouch, "bool" },
    },
    Outputs =
    {
        Activate = "bool",
        AddImpulse = "bool",
        Hide = "bool",
        Show = "bool",
        OnTouch = "bool",
    },
}
