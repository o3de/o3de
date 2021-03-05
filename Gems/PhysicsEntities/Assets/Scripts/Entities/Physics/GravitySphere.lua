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
-- Zero Gravity entity

GravitySphere =
{
    Properties =
    {
        bActive = 1,
        Radius = 10,
        bUniform = 1,
        bEllipsoidal = 1,
        FalloffInner = 0,
        Gravity = { x=0,y=0,z=0 },
        Damping = 0,
    },

    _PhysTable = { Area={}, },

    Editor =
    {
        Icon = "GravitySphere.bmp",
        IconOnTop = 0,
    },
}

-------------------------------------------------------
function GravitySphere:OnLoad(table)
  self.bActive = table.bActive
    self:PhysicalizeThis();
end

-------------------------------------------------------
function GravitySphere:OnSave(table)
  table.bActive = self.bActive
end

------------------------------------------------------------------------------------------------------
function GravitySphere:OnInit()
    self:OnReset();
end

------------------------------------------------------------------------------------------------------
-- OnPropertyChange called only by the editor.
------------------------------------------------------------------------------------------------------
function GravitySphere:OnPropertyChange()
    self:DestroyPhysics();
    self.bActive = self.Properties.bActive;
    if (self.bActive ~= 0) then
        self:PhysicalizeThis();
    end
end

------------------------------------------------------------------------------------------------------
-- OnReset called only by the editor.
------------------------------------------------------------------------------------------------------
function GravitySphere:OnReset()
    self.bActive = self.Properties.bActive;
    self:PhysicalizeThis();
end

------------------------------------------------------------------------------------------------------
function GravitySphere:PhysicalizeThis()
    --Log("GravitySphere:PhysicalizeThis");
    if (self.bActive == 1) then
        local Properties = self.Properties;
        local Area = self._PhysTable.Area;
        if (Properties.bEllipsoidal ~= 0) then
            Area.type = AREA_SPHERE;
        else
            Area.type = AREA_BOX;
            Area.box_min = {x = -Properties.Radius, y = -Properties.Radius, z = -Properties.Radius};
            Area.box_max = {x = Properties.Radius, y = Properties.Radius, z = Properties.Radius};
        end
        Area.radius = Properties.Radius;
        Area.uniform = Properties.bUniform;
        Area.falloffInner = Properties.FalloffInner;
        Area.gravity = Properties.Gravity;
        Area.damping = Properties.Damping;

        self:Physicalize( 0,PE_AREA,self._PhysTable );

        self:SetPhysicParams(PHYSICPARAM_FOREIGNDATA,{foreignData = ZEROG_AREA_ID});
    else
        self:DestroyPhysics();
    end
end

------------------------------------------------------------------------------------------------------
function GravitySphere:Event_Activate()
    if (self.bActive ~= 1) then
        self.bActive = 1;
        self:PhysicalizeThis();
        BroadcastEvent(self, "Activate");
    end
end


------------------------------------------------------------------------------------------------------
function GravitySphere:Event_Deactivate()
    if (self.bActive ~= 0) then
        self.bActive = 0;
        self:PhysicalizeThis();
        BroadcastEvent(self, "Deactivate");
    end
end

GravitySphere.FlowEvents =
{
    Inputs =
    {
        Deactivate = { GravitySphere.Event_Deactivate, "bool" },
        Activate = { GravitySphere.Event_Activate, "bool" },
    },
    Outputs =
    {
        Deactivate = "bool",
        Activate = "bool",
    },
}
