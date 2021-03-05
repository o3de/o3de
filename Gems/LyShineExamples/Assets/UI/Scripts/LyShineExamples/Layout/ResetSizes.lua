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

local ResetSizes =
{
    Properties =
    {
        ContainerElement = {default = EntityId()},
    },
}

function ResetSizes:OnActivate()
    self.tickHandler = TickBus.Connect(self)
end

function ResetSizes:OnTick()
    self.tickHandler:Disconnect()

    -- Initialize table that will hold the base offsets for every element in the content
    self.offsets = {}
    -- Save the base offsets for every element
    self:SaveElementSizeRecursive(self.Properties.ContainerElement)

    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function ResetSizes:SaveElementSizeRecursive(element)
    -- Save the offsets for that element
    self.offsets[element] = UiTransform2dBus.Event.GetOffsets(element)

    -- Iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        self:SaveElementSizeRecursive(children[i])
    end
end

function ResetSizes:OnDeactivate()
    self.buttonHandler:Disconnect()
end

function ResetSizes:OnButtonClick()
    -- Reset the offsets for every element
    for element, offsets in pairs(self.offsets) do
        local isHorizontalFit = UiLayoutFitterBus.Event.GetHorizontalFit(element)
        local isVerticalFit = UiLayoutFitterBus.Event.GetVerticalFit(element)
    
        local newOffsets = UiTransform2dBus.Event.GetOffsets(element)
        if (isHorizontalFit == false) then
            newOffsets.left = offsets.left
            newOffsets.right = offsets.right
        end
        
        if (isVerticalFit == false) then
            newOffsets.top = offsets.top
            newOffsets.bottom = offsets.bottom
        end        
    
        UiTransform2dBus.Event.SetOffsets(element, newOffsets)
    end
end

return ResetSizes