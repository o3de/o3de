----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local DeleteElements = 
{
    Properties = 
    {
        ParentElement = {default = EntityId()},
    },
}

function DeleteElements:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function DeleteElements:OnDeactivate()
    self.buttonHandler:Disconnect()
end

function DeleteElements:OnButtonClick()
    local children = UiElementBus.Event.GetChildren(self.Properties.ParentElement)
    if children then
        for i = 1,#children do
            UiElementBus.Event.DestroyElement(children[i])
        end
    end
end

return DeleteElements
