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
GravityBox =
{
    Properties =
    {
        bActive = 1,
        BoxMin = { x=-10,y=-10,z=-10 },
        BoxMax = { x=10,y=10,z=10 },
        bUniform = 1,
        Gravity = { x=0,y=0,z=0 },
        FalloffInner = 0,
    },

    Editor =
    {
        Icon = "GravitySphere.bmp",
    },
    _PhysTable = { Area={}, },
}

-------------------------------------------------------
function GravityBox:OnLoad(table)
  self.bActive = table.bActive
    if (self.bActive == 1) then
        self:PhysicalizeThis();
    else
        self:DestroyPhysics();
    end
end

-------------------------------------------------------
function GravityBox:OnSave(table)
  table.bActive = self.bActive
end

------------------------------------------------------------------------------------------------------
function GravityBox:OnInit()
    self.bActive = self.Properties.bActive;
    if (self.bActive == 1) then
        self:PhysicalizeThis();
    end
end

------------------------------------------------------------------------------------------------------
-- OnPropertyChange called only by the editor.
------------------------------------------------------------------------------------------------------
function GravityBox:OnPropertyChange()
    if (self.bActive ~= self.Properties.bActive) then
        self.bActive = self.Properties.bActive;
        if (self.bActive == 1) then
            self:PhysicalizeThis();
        else
            self:DestroyPhysics();
        end
    end
end

------------------------------------------------------------------------------------------------------
-- OnReset called only by the editor.
------------------------------------------------------------------------------------------------------
function GravityBox:OnReset()
end

------------------------------------------------------------------------------------------------------
function GravityBox:PhysicalizeThis()
    if (self.bActive == 1) then
        local Properties = self.Properties;
        local Area = self._PhysTable.Area;
        Area.type = AREA_BOX;
        Area.box_min = Properties.BoxMin;
        Area.box_max = Properties.BoxMax;
        Area.uniform = Properties.bUniform;
        Area.falloffInner = Properties.FalloffInner;
        Area.gravity = Properties.Gravity;
        self:Physicalize( 0,PE_AREA,self._PhysTable );
    end
end

------------------------------------------------------------------------------------------------------
function GravityBox:Event_Activate()
    if (self.bActive ~= 1) then
        self.bActive = 1;
        self:PhysicalizeThis();
    end
end

------------------------------------------------------------------------------------------------------
function GravityBox:Event_Deactivate()
    if (self.bActive ~= 0) then
        self.bActive = 0;
        self:PhysicalizeThis();
    end
end

GravityBox.FlowEvents =
{
    Inputs =
    {
        Deactivate = { GravityBox.Event_Deactivate, "bool" },
        Activate = { GravityBox.Event_Activate, "bool" },
    },
    Outputs =
    {
        Deactivate = "bool",
        Activate = "bool",
    },
}
