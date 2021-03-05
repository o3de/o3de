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

-- require with:
-- local inputMultiHandler = require('scripts.utils.components.inpututils')

-- self.inputHandlers = inputMultiHandler.ConnectMultiHandlers{
--     [InputEventNotificationId("jump")] = {
--         OnPressed = function(floatValue) self:JumpPressed(floatValue) end,
--         OnHeld = function(floatValue) self:JumpHeld(floatValue) end,
--         OnReleased = function(floatValue) self:JumpReleased(floatValue) end,
--     },
-- }

-- disconnect from like this:
-- self.inputHandlers:Disconnect()
local inputMultiHandlers = require('scripts.utils.components.MultiHandlers')
return {
    ConnectMultiHandlers = inputMultiHandlers(InputEventNotificationBus, InputEventNotificationId),
}