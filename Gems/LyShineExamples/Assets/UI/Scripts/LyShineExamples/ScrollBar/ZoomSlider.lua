----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local ZoomSlider = 
{
    Properties = 
    {
        ScrollBox = {default = EntityId()},
        Content = {default = EntityId()},
        MaxZoomMultiplier = {default = 2, min = 1, description = "How much the map can be zoomed in at its maximum zoom (multiplier)."},
        MinZoomMultiplier = {default = 0.5, min = 0.1, max = 1, description = "How much the map can be zoomed out at its minimum zoom (multiplier)."},
    },
}

function ZoomSlider:OnActivate()
    -- Connect to the slider notification bus
    self.sliderHandler = UiSliderNotificationBus.Connect(self, self.entityId)
    -- Initialize first zoom
    self.currentZoom = 1.0
end

function ZoomSlider:OnDeactivate()
    -- Deactivate from the slider notification bus only if we managed to connect
    if (self.sliderHandler ~= nil) then
        self.sliderHandler:Disconnect()
    end
end

-- Multiply an offset by a number
function MultiplyOffset(offset, multiplier)
    local newOffset = UiOffsets()
    newOffset.bottom = offset.bottom * multiplier
    newOffset.left = offset.left * multiplier
    newOffset.right = offset.right * multiplier
    newOffset.top = offset.top * multiplier
    return newOffset
end

-- Multiply a Vector2 by a number
function MultiplyVec2(vector, multiplier)
    local newVector = Vector2(0, 0)
    newVector.x = vector.x * multiplier
    newVector.y = vector.y * multiplier
    return newVector
end

function ZoomSlider:OnSliderValueChanging(percent)
    -- Recompute the base offsets and scrollOffsets based on our current zoom (offsets change both the position and the scale)
    local baseOffsets = UiTransform2dBus.Event.GetOffsets(self.Properties.Content)
    baseOffsets = MultiplyOffset(baseOffsets, 1 / self.currentZoom) -- current zoom will never be 0 due to property min
     local baseScrollOffsets = UiScrollBoxBus.Event.GetScrollOffset(self.Properties.ScrollBox)
    baseScrollOffsets = MultiplyVec2(baseScrollOffsets, 1 / self.currentZoom)
    
    -- Then recompute min and max offsets based on new base offsets
    local minOffsets = MultiplyOffset(baseOffsets, self.Properties.MinZoomMultiplier)
    local maxOffsets = MultiplyOffset(baseOffsets, self.Properties.MaxZoomMultiplier)
    local minScrollOffsets = MultiplyVec2(baseScrollOffsets, self.Properties.MinZoomMultiplier)
    local maxScrollOffsets = MultiplyVec2(baseScrollOffsets, self.Properties.MaxZoomMultiplier)
    
    -- Calculate new offsets based on percentage of slider ( x = (max - min) * percentage + min )
    -- Content offsets
    -- (max - min)
     local newOffset = UiOffsets()
     newOffset.bottom = maxOffsets.bottom - minOffsets.bottom
     newOffset.left = maxOffsets.left - minOffsets.left
     newOffset.right = maxOffsets.right - minOffsets.right
     newOffset.top = maxOffsets.top - minOffsets.top
     -- * percentage
     newOffset = MultiplyOffset(newOffset, percent / 100)
     -- + min
     newOffset.bottom = newOffset.bottom + minOffsets.bottom
     newOffset.left = newOffset.left + minOffsets.left
     newOffset.right = newOffset.right + minOffsets.right
     newOffset.top = newOffset.top + minOffsets.top

    -- Scroll offsets
    -- (max - min)
     local newScrollOffset = Vector2(0, 0)
     newScrollOffset.x = maxScrollOffsets.x - minScrollOffsets.x
     newScrollOffset.y = maxScrollOffsets.y - minScrollOffsets.y
     -- * percentage
     newScrollOffset = MultiplyVec2(newScrollOffset, percent / 100)
     -- + min
     newScrollOffset.x = newScrollOffset.x + minScrollOffsets.x
     newScrollOffset.y = newScrollOffset.y + minScrollOffsets.y
     
     -- Set the map offsets to the newly calculated offsets
     UiTransform2dBus.Event.SetOffsets(self.Properties.Content, newOffset)
    -- Set the scrollbox offsets to the newly calculated scroll offsets
     UiScrollBoxBus.Event.SetScrollOffset(self.Properties.ScrollBox, newScrollOffset)

    -- Update the zoom level
    self.currentZoom = (self.Properties.MaxZoomMultiplier - self.Properties.MinZoomMultiplier) * percent / 100 + self.Properties.MinZoomMultiplier
end

return ZoomSlider
