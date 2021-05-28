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
