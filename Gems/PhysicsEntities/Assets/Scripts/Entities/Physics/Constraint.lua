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
Constraint = 
{
    Properties=
    {
        radius = 0.03,
        damping = 0,
        bNoSelfCollisions = 1,
        bUseEntityFrame=1,
        max_pull_force=0,
        max_bend_torque=0,
        bConstrainToLine=0,
        bConstrainToPlane=0,
        bConstrainFully=1,
        bNoRotation=0,
        Limits = {
          x_min = 0,
          x_max = 0,
          yz_min = 0,
          yz_max = 0,
        }
    },
    numUpdates = 0,

    Editor=
    {
        Icon="Magnet.bmp",
        ShowBounds = 1,
    },
}

function Constraint:OnReset()
    --Log("%s:OnReset() idEnt: %s idConstr: %s broken: %s", self:GetName(), tostring(self.idEnt),
    --    tostring(self.idConstr), tostring(self.broken))

    --if (self.idConstr) then
        --System.GetEntity(self.idEnt):SetPhysicParams(PHYSICPARAM_REMOVE_CONSTRAINT, { id=self.idConstr })
    --end

    self.idEnt = nil
    self.idConstr = nil
    self.broken = nil
    self.numUpdates = 0
    self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
end

function Constraint:Apply()
    --Log("%s:Apply() idEnt: %s idConstr: %s broken: %s", self:GetName(), tostring(self.idEnt),
    --    tostring(self.idConstr), tostring(self.broken))

    local ConstraintParams = {pivot={},frame0={},frame1={}};
    if self.idEnt then
    elseif (not self.broken) then
        local ents = Physics.SamplePhysEnvironment(self:GetPos(), self.Properties.radius);
        if (ents[1] and ents[6]) then  
            CopyVector(ConstraintParams.pivot, self:GetPos());
            if self.Properties.bUseEntityFrame==1 then
                CopyVector(ConstraintParams.frame1, ents[1]:GetAngles());
            else
                CopyVector(ConstraintParams.frame1, self:GetAngles());
            end    
            CopyVector(ConstraintParams.frame0, ConstraintParams.frame1);
            ConstraintParams.entity_part_id_1 = ents[2];
            ConstraintParams.phys_entity_id = ents[6];
            ConstraintParams.entity_part_id_2 = ents[5];
            ConstraintParams.xmin = self.Properties.Limits.x_min;
            ConstraintParams.xmax = self.Properties.Limits.x_max;
            ConstraintParams.yzmin = self.Properties.Limits.yz_min;
            ConstraintParams.yzmax = self.Properties.Limits.yz_max;
            ConstraintParams.ignore_buddy = self.Properties.bNoSelfCollisions;
            ConstraintParams.damping = self.Properties.damping;
            ConstraintParams.sensor_size = self.Properties.radius;
            ConstraintParams.max_pull_force = self.Properties.max_pull_force;
            ConstraintParams.max_bend_torque = self.Properties.max_bend_torque;
            ConstraintParams.line = self.Properties.bConstrainToLine;
            ConstraintParams.plane = self.Properties.bConstrainToPlane;    
            ConstraintParams.no_rotation = self.Properties.bNoRotation;
            ConstraintParams.full = self.Properties.bConstrainFully;
            self.idConstr = ents[1]:SetPhysicParams(PHYSICPARAM_CONSTRAINT, ConstraintParams);
            self.idEnt = ents[1].id;
        else
            self:SetTimer(1,1);    
        end        
    end
end

function Constraint:OnTimer()
    if (self.numUpdates < 10) then 
        self:Apply();
        self.numUpdates = self.numUpdates+1;
    end
end

function Constraint:OnLevelLoaded()
    --Log("%s:OnLevelLoaded() idEnt: %s idConstr: %s broken: %s", self:GetName(), tostring(self.idEnt),
    --    tostring(self.idConstr), tostring(self.broken))

    if ((not self.broken) and (not self.idConstr)) then
        self.numUpdates = 0
        self.idEnt = nil
        self.idConstr = nil
        self:Apply()
    end
end

function Constraint:OnPropertyChange()
    self:OnReset()
end

function Constraint:OnSpawn()
    self:OnReset()
end

function Constraint:Event_ConstraintBroken(sender)
    --Log("%s:Event_ConstraintBroken() idEnt: %s idConstr: %s broken: %s", self:GetName(), tostring(self.idEnt),
    --    tostring(self.idConstr), tostring(self.broken))
    
    if self.idEnt then
        System.GetEntity(self.idEnt):SetPhysicParams(PHYSICPARAM_REMOVE_CONSTRAINT, { id=self.idConstr });
        self.idEnt = nil
        self.idConstr = nil
    end    

    self.broken = true
    self:KillTimer(1)

    BroadcastEvent(self, "ConstraintBroken")
end

function Constraint:OnSave( props )
    --Log("%s:OnSave() idEnt: %s idConstr: %s broken: %s", self:GetName(), tostring(self.idEnt),
    --    tostring(self.idConstr), tostring(self.broken))
    
    props.broken = self.broken
    props.idEnt = self.idEnt
    props.idConstr = self.idConstr
end

function Constraint:OnLoad( props )
    self.broken = props.broken
    self.idEnt = props.idEnt
    self.idConstr = props.idConstr
    
    --Log("%s:OnLoad() idEnt: %s idConstr: %s broken: %s", self:GetName(), tostring(self.idEnt),
    --    tostring(self.idConstr), tostring(self.broken))
end

function Constraint:OnDestroy()
end

Constraint.FlowEvents =
{
    Inputs =
    {
        ConstraintBroken = { Constraint.Event_ConstraintBroken, "bool" },
    },
    Outputs =
    {
        ConstraintBroken = "bool",
    },
}
